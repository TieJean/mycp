#include <glog/logging.h>
#include <error.h>
#include "mycp.h"

// reference: https://github.com/littledan/linux-aio
namespace mycp {

io_context_t ctx;

void init(const unsigned nEvents) {
    if (io_setup(nEvents, &ctx) != 0) {
        LOG(FATAL) << "io_setup failed";
    }
}
void shutdown() {
    io_destroy(ctx);
}

unordered_map<iocb*, Copier*> Copier::iocbs2Copiers;

Copier::~Copier() {
    while (!iocbBusyList.empty()) {
        RecursiveCopier::handleCallbackWorker(false, this->params);
    }
    for (iocb* iocbPtr : iocbFreeList) {
        iocbs2Copiers.erase(iocbPtr);
        if (iocbPtr->u.c.buf != nullptr) { 
            delete[] (char*) iocbPtr->u.c.buf; 
            iocbPtr->u.c.buf = nullptr;
        }
        if (iocbPtr != nullptr) { 
            delete iocbPtr; 
            iocbPtr = nullptr;
        }
    }
    // this shouldn't happen
    for (iocb* iocbPtr : iocbBusyList) {
        LOG(INFO) << "For some reason, your iocbBusyList is not empty when deconstructor is called...";
        iocbs2Copiers.erase(iocbPtr);
        if (iocbPtr->u.c.buf != nullptr) { 
            delete[] (char*) iocbPtr->u.c.buf; 
            iocbPtr->u.c.buf = nullptr;
        }
        if (iocbPtr != nullptr) { 
            delete iocbPtr; 
            iocbPtr = nullptr;
        }
    }

    close(this->fdSrc);
    close(this->fdDst); 
}

void Copier::readCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    if (res2 != 0) { LOG(ERROR) << "error in readCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested read size doesn't match with responded read size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    if (iocbs2Copiers.find(iocbPtr) == iocbs2Copiers.end()) {
        return;
    }
    int fd = iocbs2Copiers[iocbPtr]->fdDst;
    if (fcntl(fd, F_GETFD) == -1) {
        fd = open(iocbs2Copiers[iocbPtr]->dstPathStr.c_str(), O_WRONLY);
    }
    posix_fadvise(iocbs2Copiers[iocbPtr]->fdSrc, iocbPtr->u.c.offset, iocbs2Copiers[iocbPtr]->blksize,  POSIX_FADV_DONTNEED);
    io_prep_pwrite(iocbPtr, fd, iocbPtr->u.c.buf, iocbPtr->u.c.nbytes, iocbPtr->u.c.offset);
    io_set_callback(iocbPtr, Copier::writeCallback);
    int nr = io_submit(ctx, 1, &iocbPtr);
    if ( nr != 1 ) {
        // cout << "fcntl=" << fcntl(fd, F_GETFD) << endl;
        // perror("readCallback");
        // cout << "fd=" << fd << ", aio_fildes=" << iocbPtr->aio_fildes << ", aio_lio_opcode=" << iocbPtr->aio_lio_opcode << endl;
        LOG(FATAL) << "io_submit failed in readCallback\n" 
                   << "Requsted: 1; Responded: " << nr;
    }
}

void Copier::writeCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    if (res2 != 0) { LOG(ERROR) << "error in writeCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested write size doesn't match with responded write size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    if (0) {
        cout << "iocbPtr: " << iocbPtr << endl;
        cout << "iocbs2Copiers[iocbPtr]->iocbFreeList: " << &iocbs2Copiers[iocbPtr]->iocbFreeList << endl;
        cout << "iocbs2Copiers[iocbPtr]->iocbFreeList.size(): " << iocbs2Copiers[iocbPtr]->iocbFreeList.size() << endl;
    }
    if (iocbs2Copiers.find(iocbPtr) == iocbs2Copiers.end()) {
        return;
    }
    if (iocbs2Copiers[iocbPtr]->iocbFreeList.size() > 32) {
        return;
    }
    iocbs2Copiers[iocbPtr]->iocbFreeList.emplace_back(iocbPtr);
    posix_fadvise(iocbs2Copiers[iocbPtr]->fdDst, iocbPtr->u.c.offset, iocbs2Copiers[iocbPtr]->blksize,  POSIX_FADV_DONTNEED);

    if (iocbs2Copiers[iocbPtr]->offset < iocbs2Copiers[iocbPtr]->filesize) {
        iocbs2Copiers[iocbPtr]->copy();
    }
}
}