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
    bool isVerbose = false;

    Evaluator() {}
    Evaluator(const string& dataDir) : dataDir(dataDir) {
        cout << "KB: " << KB << endl;
        cout << "MB: " << MB << endl;
        cout << "GB: " << GB << endl;
    }
    ~Evaluator() {}

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
        if (stat(pathStr.c_str(), &pathStat)) {
            if (S_ISDIR(pathStat.st_mode)) {
                LOG(ERROR) << "[Evaluator::createFileBySize] You cannot override a directory by a file!";
            } else {
                if (!S_ISREG(pathStat.st_mode)) {
                    LOG(ERROR) << "[Evaluator::createFileBySize] Undefined code path!";
                }
                std::fstream ofile;
                ofile.open(pathStr, std::ios::trunc);
                if (!ofile.is_open()) {
                    LOG(ERROR) << "[Evaluator::truncateFile] Failed to open/create file: " << pathStr;
                }
                ofile.close();
                // reference: https://stackoverflow.com/questions/7775027/how-to-create-file-of-x-size
                FILE *fp = fopen(pathStr.c_str(), "w");
                if (fp == NULL) {
                    LOG(ERROR) << "[Evaluator::createFileBySize] Failed to open file: " << pathStr;
                }
                fseek(fp, filesize-1 , SEEK_SET);
                fputc('\0', fp);
                fclose(fp);
            }
        } else {
            FILE *fp = fopen(pathStr.c_str(), "w");
            if (fp == NULL) {
                LOG(ERROR) << "[Evaluator::createFileBySize] Failed to open file: " << pathStr;
            }
            fseek(fp, filesize-1 , SEEK_SET);
            fputc('\0', fp);
            fclose(fp);
        }
    }

    void createDirectory(const string& pathStr) {
        fs::path path(pathStr);
        struct stat pathStat;
        if (stat(pathStr.c_str(), &pathStat)) {
            if (S_ISREG(pathStat.st_mode)) {
                LOG(ERROR) << "[Evaluator::createDirectory] You cannot override a file by a directory!";
            } else {
                if (!S_ISDIR(pathStat.st_mode)) {
                    LOG(ERROR) << "[Evaluator::createDirectory] Undefined code path!";
                }
                return;
            }
        }
        if (!fs::create_directory(path)) {
            LOG(ERROR) << "[Evaluator::createDirectory] Failed to create directory: " << pathStr;
        }
    }

};

}