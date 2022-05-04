#include <glog/logging.h>
#include <error.h>
#include "mycp.h"

// reference: https://github.com/littledan/linux-aio
namespace mycp {

io_context_t ctx;

Copier* Copier::iocbs2Copiers[65536];
size_t Copier::count;
uint64_t primes[16] = {	4507, 3323, 5437, 	4457, 24373, 65729, 66947, 72221, 
                       14767, 40351, 41771, 	63587, 	2473, 	4139, 7177, 10499};
uint64_t masks[16] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
                      0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};

size_t iocbPtr2Idx(struct iocb *iocbPtr) {
    uint64_t ret = 0;
    for (size_t i = 0; i < 16; ++i) {
        ret += (((((uint64_t)iocbPtr + ret) & masks[i]) >> i) * primes[i]);
    }
    return ret % 65536;
}

void init(const unsigned nEvents) {
    if (io_setup(nEvents, &ctx) < 0) {
        LOG(FATAL) << "io_setup failed";
    }
    for (size_t i = 0; i < 65536; ++i) {
        Copier::iocbs2Copiers[i] = NULL;
    }
    Copier::count = 0;
}
void shutdown() {
    io_destroy(ctx);
}

Copier::~Copier() {
    while (!iocbBusyList.empty()) {
        RecursiveCopier::handleCallbackWorker(false, this->params);
    }
    for (iocb* iocbPtr : iocbFreeList) {
        size_t iocbPtrIdx = iocbPtr2Idx(iocbPtr); 
        iocbs2Copiers[iocbPtrIdx] = NULL;
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
        size_t iocbPtrIdx = iocbPtr2Idx(iocbPtr); 
        iocbs2Copiers[iocbPtrIdx] = NULL;
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
    size_t iocbPtrIdx = iocbPtr2Idx(iocbPtr);
    int fd = iocbs2Copiers[iocbPtrIdx]->fdDst;
    io_prep_pwrite(iocbPtr, fd, iocbPtr->u.c.buf, iocbPtr->u.c.nbytes, iocbPtr->u.c.offset);
    io_set_callback(iocbPtr, Copier::writeCallback);
    int nr = io_submit(ctx, 1, &iocbPtr);
    if ( nr != 1 ) {
        cout << "fcntl=" << fcntl(fd, F_GETFD) << endl;
        perror("readCallback");
        cout << "fd=" << fd << ", aio_fildes=" << iocbPtr->aio_fildes << ", aio_lio_opcode=" << iocbPtr->aio_lio_opcode << endl;
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
    size_t iocbPtrIdx = iocbPtr2Idx(iocbPtr);
    if (0) {
        cout << "iocbPtr: " << iocbPtr << endl;
        cout << "iocbs2Copiers[iocbPtr]->iocbFreeList: " << &iocbs2Copiers[iocbPtrIdx]->iocbFreeList << endl;
        cout << "iocbs2Copiers[iocbPtr]->iocbFreeList.size(): " << iocbs2Copiers[iocbPtrIdx]->iocbFreeList.size() << endl;
    }
    if (iocbs2Copiers[iocbPtrIdx]->iocbFreeList.size() > 64) {
        return;
    }
    iocbs2Copiers[iocbPtrIdx]->iocbFreeList.emplace_back(iocbPtr);

    if (iocbs2Copiers[iocbPtrIdx]->offset < iocbs2Copiers[iocbPtrIdx]->filesize) {
        iocbs2Copiers[iocbPtrIdx]->copy();
    }
}
}