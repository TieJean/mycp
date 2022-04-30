#include <glog/logging.h>
#include <errno.h>
#include "mycp.h"

// reference: https://github.com/littledan/linux-aio
namespace mycp {

io_context_t ctx;

void init(const unsigned nEvents) {
    if (io_setup(nEvents, &ctx) != 0) {
        LOG(FATAL) << "io_setup failed";
    }
}

void wait() {

}

void shutdown() {
    io_destroy(ctx);
}

unordered_map<iocb*, Copier*> Copier::iocbs2Copiers;
void Copier::readCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    if (res2 != 0) { LOG(ERROR) << "error in readCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested read size doesn't match with responded read size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    int fd = iocbs2Copiers[iocbPtr]->fdDst;
    cout << "[readCallback] fd=" << fd << endl;
    cout << "[readCallback] iocb offset=" << iocbPtr->u.c.offset 
         << ", nbytes=" << iocbPtr->u.c.nbytes << endl;
    cout << "[readCallback] iocbPtr->u.c.buf: " << iocbPtr->u.c.buf << endl;
    cout << "[readCallback] *iocbPtr->u.c.buf: " << *((size_t*)iocbPtr->u.c.buf) << endl;
    io_prep_pwrite(iocbPtr, fd, iocbPtr->u.c.buf, iocbPtr->u.c.nbytes, iocbPtr->u.c.offset);
    io_set_callback(iocbPtr, Copier::writeCallback);
    int nr = io_submit(ctx, 1, &iocbPtr);
    if ( nr != 1) {
        LOG(FATAL) << "io_submit failed in readCallback\n" 
                   << "Requsted: 1; Responded: " << nr;
    }
}

void Copier::writeCallback(io_context_t ctx, struct iocb *iocbPtr, long res, long res2) {
    cout << "[writeCallback] start" << endl;
    if (res2 != 0) { LOG(ERROR) << "error in writeCallback"; }
    if (res != iocbPtr->u.c.nbytes) {
        LOG(ERROR) << "Requested write size doesn't match with responded write size\n" 
                   << "Requested: " << iocbPtr->u.c.nbytes << "; Responded: " << res;
    }
    iocbs2Copiers[iocbPtr]->iocbFreeList.emplace_back(iocbPtr);

    if (iocbs2Copiers[iocbPtr]->offset < iocbs2Copiers[iocbPtr]->filesize) {
        iocbs2Copiers[iocbPtr]->copy();
    }
    cout << "[writeCallback] end" << endl;

}
}