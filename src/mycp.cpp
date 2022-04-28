#include <glog/logging.h>
#include "mycp.h"

// reference: https://github.com/littledan/linux-aio
namespace mycp {

io_context_t ctx;
unordered_map<iocb*, Copier*> Copier::iocbs2Copiers;

void init(const unsigned nEvents) {
    if (io_setup(nEvents, &ctx) != 0) {
        LOG(FATAL) << "io_setup failed";
    }
}
void shutdown() {
    io_destroy(ctx);
}

}