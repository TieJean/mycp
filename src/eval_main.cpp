#include <gflags/gflags.h>
#include <glog/logging.h>

#include "eval.h"

DEFINE_string(data_dir, "/home/taijing/project/data/", "\nsource directory\n");

using namespace mycp;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    Evaluator eval(FLAGS_data_dir);
    eval.isVerbose = true;
    eval.CreateBTreeSmallFiles();
}