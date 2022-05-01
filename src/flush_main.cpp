#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <glog/logging.h>

using std::endl;

int main() {
    size_t sz = (sysconf(_SC_LEVEL2_CACHE_SIZE) + (1 << 10)) * sizeof(char);
    char* buf;
    buf = (char*) malloc(sz);
    if (buf == NULL) {
        LOG(ERROR) << "failed to malloc when flushing cache" << endl;
    }
    memset(buf, 1, sz);
    free(buf);
}