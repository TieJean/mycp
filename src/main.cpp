#include <gflags/gflags.h>
#include <glog/logging.h>

#include "mycp.h"

DEFINE_string(src, "", "\nsource directory\n");
DEFINE_string(dst, "", "\ndestination directory\n");

using namespace mycp;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_src.size() == 0 || FLAGS_dst.size() == 0) {
        LOG(FATAL) << "missing src or dest";
    }

    AIOParam params;
    params.nMaxCopierEvents  = 128;
    params.nMaxRCopierEvents = 16192;
    params.timeout.tv_sec = 1;
    params.timeout.tv_nsec = 0;
    mycp::init(params.nMaxRCopierEvents);

    struct timespec timeout;
    timeout.tv_sec = 1;
    timeout.tv_nsec = 0;
    params.timeout = timeout;
    RecursiveCopier rCopier(FLAGS_src, FLAGS_dst, params);
    rCopier.recursiveCopy();

    mycp::shutdown();

}