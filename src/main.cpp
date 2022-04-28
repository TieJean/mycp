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

    mycp::init(16);

    RecursiveCopier rCopier(FLAGS_src, FLAGS_dst);
    rCopier.recursiveCopy();

    mycp::shutdown();

}