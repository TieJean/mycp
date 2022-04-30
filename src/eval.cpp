#include "eval.h"

namespace mycp {

void Evaluator::flushL1DCache() {
    size_t sz = (cacheSize + KB) * sizeof(char);
    char* buf;
    buf = (char*) malloc(sz);
    if (buf == NULL) {
        LOG(ERROR) << "failed to malloc when flushing cache" << endl;
    }
    memset(buf, 1, sz);
    free(buf);
}

void Evaluator::CreateBTreeSmallFiles() {
    // root
    string testDir = dataDir + "BTreeFewSmallFilesTest/";
    createDirectory(testDir);
    testDir +=  "src/";
    createDirectory(testDir);

    size_t nFile = 5;
    size_t nDirLayer1  = 5;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = KB;
    }
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        dirStrLayer1List.push_back(std::to_string(dirIdx));
    }
    createFilesAndDirs(testDir, fileStrToSizeList, dirStrLayer1List);

    // layer2
    dirStrLayer1List.clear();
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        string currTestRelDir = std::to_string(dirIdx) + "/";
        fileStrToSizeList.clear();
        for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
            string currTestRelPath = currTestRelDir + std::to_string(fileIdx) + ".txt";
            fileStrToSizeList[currTestRelPath] = KB;
        }
        createFilesAndDirs(testDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateBTreeLargeFiles() {

}
void Evaluator::CreateDTreeSmallFiles() {

}
void Evaluator::CreateDTreeLargeFiles() {

}
void Evaluator::CreateBTreeHybrid() {

}
void Evaluator::CreateDTreeHybrid() {

}
void Evaluator::CreateDirectoryOnly() {

}

}