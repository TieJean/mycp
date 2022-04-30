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

void Evaluator::CreateDebug() {
    auto testDirs = createTestRoot("debug");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    createFileBySize(testSrcDir + "test.txt", 8 * KB);
}

void Evaluator::CreateBTreeSmallFiles() {
    // root
    auto testDirs = createTestRoot("BTreeSmallFilesTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFile = 10;
    size_t nDirLayer1  = 10;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = 8 * KB;
    }
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        dirStrLayer1List.push_back(std::to_string(dirIdx));
    }
    createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);

    // layer2
    dirStrLayer1List.clear();
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        string currTestRelDir = std::to_string(dirIdx) + "/";
        fileStrToSizeList.clear();
        for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
            string currTestRelPath = currTestRelDir + std::to_string(fileIdx) + ".txt";
            fileStrToSizeList[currTestRelPath] = KB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateBTreeLargeFiles() {
    // root
    auto testDirs = createTestRoot("BTreeLargeFilesTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFileLayer1 = 5;
    size_t nFileLayer2 = 10;
    size_t nDirLayer1  = 10;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFileLayer1; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = MB;
    }
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        dirStrLayer1List.push_back(std::to_string(dirIdx));
    }
    createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);

    // layer2
    dirStrLayer1List.clear();
    for (size_t dirIdx = 0; dirIdx < nDirLayer1; ++dirIdx) {
        string currTestRelDir = std::to_string(dirIdx) + "/";
        fileStrToSizeList.clear();
        for (size_t fileIdx = 0; fileIdx < nFileLayer2; ++fileIdx) {
            string currTestRelPath = currTestRelDir + std::to_string(fileIdx) + ".txt";
            fileStrToSizeList[currTestRelPath] = MB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
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