#include <glog/logging.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <libaio.h>
#include <stdint.h>
#include <string.h>
#include <filesystem>
#include <boost/filesystem.hpp>
#include <sys/sendfile.h>
#include<thread>

#include "trie.h"

using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::string;
using std::cout;
using std::endl;
using std::thread;
namespace fs = boost::filesystem;

namespace mycp {

struct AIOParam {
    size_t nMaxCopierEvents;
    size_t nMaxRCopierEvents;
    struct timespec timeout;
};

extern io_context_t ctx;
void init(const unsigned nEvents);
void shutdown();
size_t iocbPtr2Idx(struct iocb *iocbPtr);

class Copier {

static void readCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2);
static void writeCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2);

public:
    static Copier* iocbs2Copiers[65536];

    string srcPathStr;
    string dstPathStr;
    int fdSrc;
    int fdDst;
    AIOParam params;
    size_t filesize;
    blksize_t blksize;
    vector<iocb*> iocbFreeList;
    vector<iocb*> iocbBusyList;
    off_t offset;
    bool isVerbose = false;

    Copier() {}
    Copier(const string& pathSrc, const string& pathDst, const AIOParam& params) {
        // cout << "[Copier] start threadId=" << std::this_thread::get_id()  << endl;
        this->srcPathStr = pathSrc;
        this->dstPathStr = pathDst;
        this->fdSrc = open(pathSrc.c_str(), O_RDONLY);
        if (this->fdSrc < 0) {
            LOG(FATAL) << "failed to open source file: " << pathSrc;
        }
        this->fdDst = open(pathDst.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
        if (this->fdDst < 0) {
            LOG(FATAL) << "failed to open destination file: " << pathDst;
        }
        if (isVerbose) { cout << "fdDst=" << fdDst << ", pathDst=" << pathDst << endl; }
        struct stat stat;
        if (fstat(this->fdSrc, &stat) < 0) {
            LOG(FATAL) << "falied to fstat source file: " << pathSrc;
        }
        this->filesize = stat.st_size;
        this->blksize  = stat.st_blksize;
        this->params = params;
        this->iocbFreeList.reserve(params.nMaxCopierEvents);
        this->iocbBusyList.reserve(params.nMaxCopierEvents);
        if (isVerbose) {
            cout << endl;
            cout << "[Copier] pathSrc=" << pathSrc << ", pathDst=" << pathDst 
                << ", this=" << this << endl;
            cout << "[Copier] iocbFreeList=: " << &(this->iocbFreeList) << endl;
        }
        for (size_t i = 0; i < params.nMaxCopierEvents; ++i) {
            iocb* iocbPtr  = new iocb();
            iocbPtr->u.c.buf = new char[this->blksize];
            this->iocbFreeList.emplace_back(iocbPtr);
            size_t iocbPtrIdx = iocbPtr2Idx(iocbPtr);
            Copier::iocbs2Copiers[iocbPtrIdx] = this;
            if (isVerbose) { cout << "[Copier] iocbPtr=" << iocbPtr << endl; }
        }
        // cout << "[Copier] end threadId=" << std::this_thread::get_id()  << endl;
    }

    ~Copier();

    void copy() {
        while (!iocbFreeList.empty() && this->offset < this->filesize) {
            size_t rwSize = this->filesize - this->offset < this->blksize ? 
                            this->filesize - this->offset : this->blksize;
            iocb* iocbPtr = iocbFreeList.back();
            iocbFreeList.pop_back();
            iocbPtr->u.c.nbytes = rwSize;
            iocbPtr->u.c.offset = offset;
            io_prep_pread(iocbPtr, this->fdSrc, iocbPtr->u.c.buf, rwSize, offset);
            io_set_callback(iocbPtr, Copier::readCallback);
            iocbBusyList.emplace_back(iocbPtr);
            this->offset += rwSize;
        }
        if (isVerbose) {
            cout << "[Copier::copy] offset=" << this->offset << endl;
            cout << "[Copier::copy] iocbBusyList.size(): " << iocbBusyList.size() << ", iocbFreeList.size(): " << iocbFreeList.size() << endl;
        }
        if (iocbBusyList.size() == this->params.nMaxCopierEvents || this->offset >= this->filesize) {
            int nr = io_submit(ctx, this->iocbBusyList.size(), this->iocbBusyList.data());
            if (nr != this->iocbBusyList.size()) {
                LOG(INFO) << "Requested event nr doesn't match responded event nr\n"
                          << "Requested: " << this->iocbBusyList.size() << "Responded: " << nr;
            }
            if (isVerbose) {
                cout << "[Copier::copy] this->iocbFreeList.size(): " << this->iocbFreeList.size()
                     << ", this->iocbFreeList=" << &(this->iocbFreeList) << endl;
            }
            this->iocbBusyList.erase(this->iocbBusyList.begin(), this->iocbBusyList.begin() + nr);
        }
    }

private:
};

class RecursiveCopier {

public:
    string srcDir;
    string dstDir;
    AIOParam params;
    bool isVerbose = false;

    RecursiveCopier() {
        LOG(INFO) << "use default RecursiveCopier constructor is dangerous!\n"
                  << "pls use RecursiveCopier(const string& srcDir, const string& dstDir) instead";
    }

    RecursiveCopier(const string& srcDir, const string& dstDir, const AIOParam& params) {
        this->srcDir = srcDir;
        this->dstDir = dstDir;
        this->params = params;
        struct stat srcStat, dstStat;
        // check if directory reference: https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
        if (stat(this->srcDir.c_str(), &srcStat)) {
            LOG(FATAL) << "cannot open src dir: " << this->srcDir;
        }
        if (!S_ISDIR(srcStat.st_mode)) {
            LOG(ERROR) << "expected a src dir, but got a file: " << this->srcDir;
        } 
        if (stat(this->dstDir.c_str(), &dstStat)) {
            if (mkdir(this->dstDir.c_str(), 0777)) {
                LOG(FATAL) << "failed to create dst dir: " << this->dstDir;
            }
        } else if (!S_ISDIR(dstStat.st_mode)) {
            LOG(FATAL) << "expected a dst dir, but didn't get a dir: " << this->dstDir;
        }
    }

    ~RecursiveCopier() {}

    void recursiveCopy() {
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(this->srcDir)) {
            const fs::path& currSrcPath = entry.path();
            const fs::path& currDstPath = this->dstDir / fs::relative(currSrcPath, this->srcDir);
            struct stat srcStat, dstStat;
            if (stat(currSrcPath.c_str(), &srcStat)) {
                LOG(FATAL) << "[RecursiveCopier::recursiveCopy] cannot open src dir: " << this->srcDir;
            }
            if (S_ISREG(srcStat.st_mode)) {
                handleFile(currSrcPath, currDstPath, srcStat);
            } else if (S_ISDIR(srcStat.st_mode)) {
                handleDir(currSrcPath, currDstPath);
            } else {
                LOG(FATAL) << "[RecursiveCopier::recursiveCopy] undefined code path!";
            }
        }
    }

    void recursiveCopyMultiThread() {
        vector<thread> workers;
        for (const fs::directory_entry& entry : fs::directory_iterator(this->srcDir)) {
            const fs::path& currSrcPath = entry.path();
            const fs::path& currDstPath = this->dstDir / fs::relative(currSrcPath, this->srcDir);
            if (fs::is_directory(currSrcPath)) {
                handleDir(currSrcPath, currDstPath);
                thread worker(RecursiveCopier::recursiveCopyWorker, currSrcPath.string(), currDstPath.string(), this->params);
                workers.push_back(std::move(worker));
                // recursiveCopySingle(currSrcPath.string());
            } else {
                const fs::path& currDstPath = this->dstDir / fs::relative(currSrcPath, this->srcDir);
                struct stat srcStat;
                stat(currSrcPath.c_str(), &srcStat);
                handleFile(currSrcPath, currDstPath, srcStat);
            }
        }
        for (thread& worker : workers) {
            if (worker.joinable()) { worker.join(); }
        }
    }

    static void recursiveCopyWorker(const string& srcDirStr, const string& dstDirStr, const AIOParam& aioParams) {
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(srcDirStr)) {
            const fs::path& currSrcPath = entry.path();
            const fs::path& currDstPath = dstDirStr / fs::relative(currSrcPath, srcDirStr);
            struct stat srcStat, dstStat;
            if (stat(currSrcPath.c_str(), &srcStat)) {
                LOG(FATAL) << "[RecursiveCopier::recursiveCopy] cannot open src dir: " << srcDirStr;
            }
            if (S_ISREG(srcStat.st_mode)) {
                handleFileWorker(currSrcPath, currDstPath, aioParams);
            } else if (S_ISDIR(srcStat.st_mode)) {
                handleDirWorker(currSrcPath, currDstPath);
            } else {
                LOG(FATAL) << "[RecursiveCopier::recursiveCopy] undefined code path!";
            }
        }
    }

    static void handleFileWorker(const fs::path& srcPath, const fs::path& dstPath, const AIOParam& aioParams) {
        struct stat srcStat;
        int ret = stat(srcPath.c_str(), &srcStat);
        if (srcStat.st_size <= srcStat.st_blksize) {
        // if (srcStat.st_size <= (1 << 30)) {
            int fdSrc, fdDst;
            fdSrc = open(srcPath.c_str(), O_RDONLY); // don't need to check this open
            if (access(dstPath.c_str(), F_OK)) {
                fdDst = open(dstPath.c_str(), O_CREAT | O_EXCL, srcStat.st_mode);
                if (fdDst < 0) {
                    LOG(FATAL) << "[RecursiveCopier::handleFile] failed to create dst file: "
                               << dstPath;
                }
                close(fdDst);
            }
            fdDst = open(dstPath.c_str(), O_TRUNC | O_WRONLY);
            if (fdDst < 0) {
                LOG(FATAL) << "[RecursiveCopier::handleFile] failed to open dst file: "
                            << dstPath;
            }
            int nbytes = sendfile(fdDst, fdSrc, 0, srcStat.st_size);
            if (nbytes < srcStat.st_size) {
                LOG(ERROR) << "[RecursiveCopier::handleFile] Requsted sendfile size doesn't match with responded size\n"
                           << "Requested: " << srcStat.st_size << "; Responded: " << nbytes;
            }
            close(fdSrc);
            close(fdDst);
        } else {
            // cout << "[handleFileWorker] threadId=" << std::this_thread::get_id() << endl;
            string srcPathStr = srcPath.string();
            string dstPathStr = dstPath.string();
            Copier copier(srcPathStr, dstPathStr, aioParams);
            copier.copy();
            RecursiveCopier::handleCallbackWorker(true, aioParams);
        }
    }

    static void handleDirWorker(const fs::path& srcPath, const fs::path& dstPath) {
        struct stat srcStat, dstStat;
        if (stat(dstPath.c_str(), &dstStat)) {
            if (!fs::create_directory(dstPath)) {
                LOG(FATAL) << "[RecursiveCopier::handleDir] failed creating the dst dir: " << dstPath; 
            }
        } else if (S_ISREG(dstStat.st_mode)) {
            LOG(FATAL) << "[RecursiveCopier::handleDir] we have naming conflicts in dstDir... "
                       << "expected empty or some directory, but got a file";
        }
    }

    static void handleCallbackWorker(const bool isTimeout, const AIOParam aioParams) {
        io_event events[aioParams.nMaxRCopierEvents]; // TODO FIXME
        int nrEvents;
        if (isTimeout) {
            nrEvents = io_getevents(ctx, 0, aioParams.nMaxRCopierEvents, events, NULL);
        } else  {
             nrEvents = io_getevents(ctx, 0, aioParams.nMaxRCopierEvents, events, NULL);
            //  nrEvents = io_getevents(ctx, 0, aioParams.nMaxRCopierEvents, events, &timeout);
        }
        for (size_t eventIdx = 0; eventIdx < nrEvents; eventIdx++) {
            io_callback_t callback = (io_callback_t)events[eventIdx].data; 
            callback(ctx, events[eventIdx].obj, events[eventIdx].res, events[eventIdx].res2);
        }
    }

private:
    void handleFile(const fs::path& srcPath, const fs::path& dstPath, const struct stat& srcStat) {
        if (isVerbose) {
            cout << "srcStat.st_blksize: " << srcStat.st_blksize << endl;
        }
        // small files: if the file is less than one blksize (inclusive)
        // reference: https://stackoverflow.com/questions/10543230/fastest-way-to-copy-data-from-one-file-to-another-in-c-c
        if (srcStat.st_size <= srcStat.st_blksize) {
        // if (srcStat.st_size <= 4096) {
            int fdSrc, fdDst;
            fdSrc = open(srcPath.c_str(), O_RDONLY); // don't need to check this open
            if (access(dstPath.c_str(), F_OK)) {
                fdDst = open(dstPath.c_str(), O_CREAT | O_EXCL, srcStat.st_mode);
                if (fdDst < 0) {
                    LOG(FATAL) << "[RecursiveCopier::handleFile] failed to create dst file: "
                               << dstPath;
                }
                close(fdDst);
            }
            fdDst = open(dstPath.c_str(), O_TRUNC | O_WRONLY);
            if (fdDst < 0) {
                LOG(FATAL) << "[RecursiveCopier::handleFile] failed to open dst file: "
                            << dstPath;
            }
            int nbytes = sendfile(fdDst, fdSrc, 0, srcStat.st_size);
            if (nbytes < srcStat.st_size) {
                LOG(ERROR) << "[RecursiveCopier::handleFile] Requsted sendfile size doesn't match with responded size\n"
                           << "Requested: " << srcStat.st_size << "; Responded: " << nbytes;
            }
            close(fdSrc);
            close(fdDst);
        } else {
            string srcPathStr = srcPath.string();
            string dstPathStr = dstPath.string();
            Copier copier(srcPathStr, dstPathStr, this->params);
            copier.copy();
            handleCallback(true);
        }
    }

    void handleDir(const fs::path& srcPath, const fs::path& dstPath) {
        struct stat srcStat, dstStat;
        if (stat(dstPath.c_str(), &dstStat)) {
            if (!fs::create_directory(dstPath)) {
                LOG(FATAL) << "[RecursiveCopier::handleDir] failed creating the dst dir: " << dstPath; 
            }
        } else if (S_ISREG(dstStat.st_mode)) {
            LOG(FATAL) << "[RecursiveCopier::handleDir] we have naming conflicts in dstDir... "
                       << "expected empty or some directory, but got a file";
        }
    }

    void handleCallback(bool isTimeout) {
        io_event events[params.nMaxRCopierEvents]; // TODO FIXME
        int nrEvents;
        if (isTimeout) {
            nrEvents = io_getevents(ctx, 0, params.nMaxRCopierEvents, events, NULL);
        } else  {
             nrEvents = io_getevents(ctx, 0, params.nMaxRCopierEvents, events, &params.timeout);
            //  nrEvents = io_getevents(ctx, 0, params.nMaxRCopierEvents, events, NULL);
        }
        if (isVerbose) {
            cout << "[RecursiveCopier::handleCallback] nrEvents=" << nrEvents << endl;
        }
        for (size_t eventIdx = 0; eventIdx < nrEvents; eventIdx++) {
            io_callback_t callback = (io_callback_t)events[eventIdx].data; 
            callback(ctx, events[eventIdx].obj, events[eventIdx].res, events[eventIdx].res2);
        }
    }

};

}