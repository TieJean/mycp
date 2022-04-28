#include <glog/logging.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <libaio.h>
#include <stdint.h>
#include <string.h>

using std::vector;
using std::unordered_map;
using std::string;

namespace mycp {

struct AIOParam {
    uint8_t nTotalEvents;
};

io_context_t ctx;
void init(const unsigned nEvents) {
    if (io_setup(nEvents, &ctx) != 0) {
        LOG(FATAL) << "io_setup failed";
    }
}
void shutdown() {
    io_destroy(ctx);
}

class Copier {

static unordered_map<iocb*, Copier*> iocbs2Copiers;;

static void readCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    if (res2 != 0) { LOG(ERROR) << "error in readCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested read size doesn't match with responded read size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    int fd = iocbs2Copiers[iocbPtr]->fdDst;
    io_prep_pwrite(iocbPtr, fd, iocbPtr->u.c.buf, iocbPtr->u.c.nbytes, iocbPtr->u.c.offset);
    io_set_callback(iocbPtr, Copier::writeCallback);
    if (io_submit(ctx, 1, &iocbPtr) != 1) {
        LOG(FATAL) << "io_submit failed in readCallback";
    }
}

static void writeCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    if (res2 != 0) { LOG(ERROR) << "error in writeCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested write size doesn't match with responded write size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    iocbs2Copiers[iocbPtr]->iocbFreeList.emplace_back(iocbPtr);

    if (iocbs2Copiers[iocbPtr]->offset < iocbs2Copiers[iocbPtr]->filesize) {
        iocbs2Copiers[iocbPtr]->copy();
    }
}

public:
    int fdSrc;
    int fdDst;
    AIOParam params;
    size_t filesize;
    blksize_t blksize;
    vector<iocb*> iocbFreeList;
    vector<iocb*> iocbBusyList;
    off_t offset;
    bool isVerbose;

    Copier() {}
    Copier(const string& pathSrc, const string& pathDst, const AIOParam& params) {
        this->fdSrc = open(pathSrc.c_str(), O_RDONLY);
        if (this->fdSrc < 0) {
            LOG(FATAL) << "failed to open source file: " << pathSrc;
        }
        this->fdDst = open(pathDst.c_str(), O_WRONLY | O_CREAT);
        if (this->fdDst < 0) {
            LOG(FATAL) << "failed to open destination file: " << pathDst;
        }
        struct stat stat;
        if (fstat(this->fdSrc, &stat) < 0) {
            LOG(FATAL) << "falied to fstat source file: " << pathSrc;
        }
        this->filesize = stat.st_size;
        this->blksize  = stat.st_blksize;
        this->params = params;
        iocbFreeList.reserve(params.nTotalEvents);
        iocbBusyList.reserve(params.nTotalEvents);

        for (size_t i = 0; i < params.nTotalEvents; ++i) {
            iocb* iocbPtr  = new iocb();
            Copier* cpPtr  = new Copier();
            iocbPtr->u.c.buf = new char[this->blksize];
            iocbFreeList.emplace_back(iocbPtr);
            iocbs2Copiers[iocbPtr] = cpPtr;
        }

    }

    ~Copier() {
        for (iocb* iocbPtr : iocbFreeList) {
            iocbs2Copiers.erase(iocbPtr);
            if (iocbPtr->u.c.buf != nullptr) { delete[] (char*) iocbPtr->u.c.buf; }
            if (iocbPtr != nullptr) { delete iocbPtr; }
        }
        // this shouldn't happen
        for (iocb* iocbPtr : iocbBusyList) {
            LOG(INFO) << "For some reason, your iocbBusyList is not empty when deconstructor is called...";
            iocbs2Copiers.erase(iocbPtr);
            if (iocbPtr->u.c.buf != nullptr) { delete[] (char*) iocbPtr->u.c.buf; }
            if (iocbPtr != nullptr) { delete iocbPtr; }
        }

        close(this->fdSrc);
        close(this->fdDst);
    }

    void copy() {
        while (!iocbFreeList.empty() && this->offset < this->filesize) {
            size_t rwSize = this->filesize - this->offset < this->blksize ? 
                            this->filesize - this->offset : this->blksize;
            iocb* iocbPtr = iocbFreeList[iocbFreeList.size()-1];
            iocbFreeList.pop_back();
            iocbPtr->u.c.nbytes = rwSize;
            iocbPtr->u.c.offset = offset;
            io_prep_pread(iocbPtr, this->fdSrc, iocbPtr->u.c.buf, rwSize, offset);
            io_set_callback(iocbPtr, readCallback);
            iocbBusyList.emplace_back(iocbPtr);
            this->offset += rwSize;
        }

        if (iocbFreeList.size() == this->params.nTotalEvents || this->offset >= this->filesize) {
            int nr = io_submit(ctx, this->iocbBusyList.size(), this->iocbBusyList.data());
            if (nr != this->iocbBusyList.size()) {
                LOG(INFO) << "Requested event nr doesn't match responded event nr\n"
                          << "Requested: " << this->iocbBusyList.size() << "Responded: " << nr;
            }
            this->iocbBusyList.erase(this->iocbBusyList.begin(), this->iocbBusyList.begin() + nr);
        }
    }

private:
};

}