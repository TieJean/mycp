#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <stdio.h>
#include "mycp.h"

using std::string;
using std::vector;
using std::unordered_map;
namespace fs = boost::filesystem;

namespace mycp {

class Evaluator {
    
public:
    uint64_t KB = (1 << 10); // 1 KB
    uint64_t MB = (1 << 20); // 1 MB
    uint64_t GB = (1 << 30); // 1 GB
    string dataDir;
    uint64_t cacheSize;
    bool isVerbose = false;

    Evaluator() {}
    Evaluator(const string& dataDir) : dataDir(dataDir) {
        this->cacheSize = sysconf(_SC_LEVEL2_CACHE_SIZE);
        cout << "cache size: " << this->cacheSize  << endl;
    }
    ~Evaluator() {}

    void flushL1DCache();

    void CreateBTreeSmallFiles();
    void CreateBTreeLargeFiles();
    void CreateDTreeSmallFiles();
    void CreateDTreeLargeFiles();
    void CreateBTreeHybrid();
    void CreateDTreeHybrid();
    void CreateDirectoryOnly();

private:
    void createFilesAndDirs(const string& pathStr, const unordered_map<string, uint64_t>& fileStrToSizeList, const vector<string>& dirStrList) {
        struct stat pathStat;
        for (const auto& fileStrToSize : fileStrToSizeList) {
            createFileBySize(pathStr + fileStrToSize.first, fileStrToSize.second);
        }
        for (const auto& dirStr : dirStrList) {
            createDirectory(pathStr + dirStr);
        }
    }

    void createFileBySize(const string& pathStr, const uint64_t filesize) {
        struct stat pathStat;
        if (stat(pathStr.c_str(), &pathStat) == 0) {
            if (S_ISDIR(pathStat.st_mode)) {
                LOG(FATAL) << "[Evaluator::createFileBySize] You cannot override a directory by a file!";
            } else {
                if (!S_ISREG(pathStat.st_mode)) {
                    LOG(FATAL) << "[Evaluator::createFileBySize] Undefined code path!";
                }
                fs::resize_file(fs::path(pathStr), 0);
                // reference: https://stackoverflow.com/questions/7775027/how-to-create-file-of-x-size
                FILE *fp = fopen(pathStr.c_str(), "w");
                if (fp == NULL) {
                    LOG(FATAL) << "[Evaluator::createFileBySize] Failed to open file: " << pathStr;
                }
                fseek(fp, filesize-1 , SEEK_SET);
                fputc('\0', fp);
                fclose(fp);
            }
        } else {
            FILE *fp = fopen(pathStr.c_str(), "w");
            if (fp == NULL) {
                cout << "pathStr: " << pathStr << endl;
                LOG(FATAL) << "[Evaluator::createFileBySize] Failed to open file: " << pathStr;
            }
            fseek(fp, filesize-1 , SEEK_SET);
            fputc('\0', fp);
            fclose(fp);
        }
    }

    void createDirectory(const string& pathStr) {
        fs::path path(pathStr);
        struct stat pathStat;
        if (!stat(pathStr.c_str(), &pathStat)) {
            if (S_ISREG(pathStat.st_mode)) {
                LOG(FATAL) << "[Evaluator::createDirectory] You cannot override a file by a directory!";
            } else {
                if (!S_ISDIR(pathStat.st_mode)) {
                    LOG(FATAL) << "[Evaluator::createDirectory] Undefined code path!";
                }
            }
        } else {
            if (!fs::create_directory(path)) {
                LOG(FATAL) << "[Evaluator::createDirectory] Failed to create directory: " << pathStr;
            }
        }
    }

};

}