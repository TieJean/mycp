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
    struct stat srcStat, dstState;
    // check if directory reference: https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
    if (stat(FLAGS_src.c_str(), &srcStat)) {
        LOG(FATAL) << "cannot open src dir: " << FLAGS_src;
    }
    if (!S_ISDIR(srcStat.st_mode)) {
        LOG(ERROR) << "expected a src dir, but got a file: " << FLAGS_src;
    } 
    if (stat(FLAGS_dst.c_str(), &dstState)) {
        if (mkdir(FLAGS_dst.c_str(), 0777)) {
            LOG(FATAL) << "failed to create dst dir: " << FLAGS_dst;
        }
    } else if (!S_ISDIR(dstState.st_mode)) {
        LOG(FATAL) << "expected a dst dir, but didn't get a dir: " << FLAGS_dst;
    }

    

    mycp::shutdown();

}