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

void Evaluator::CreateSingleLargeFile() {
    auto testDirs = createTestRoot("SingleLargeFileTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    createFileBySize(testSrcDir + "test.txt", 4 * GB);
}

void Evaluator::CreateBTreeHybrid() {
    auto testDirs = createTestRoot("BTreeHybridTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFile = 10;
    size_t nLargeFile = 5;
    size_t nDirLayer1  = 10;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = 8 * KB;
    }
    for (size_t fileIdx = nFile; fileIdx < nFile + nLargeFile; ++fileIdx) {
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
        for (size_t fileIdx = 0; fileIdx < nFile; ++fileIdx) {
            string currTestRelPath = currTestRelDir + std::to_string(fileIdx) + ".txt";
            fileStrToSizeList[currTestRelPath] = KB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateBTreeSmallFiles() {
    // root
    auto testDirs = createTestRoot("BTreeSmallFilesTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFileLayer1 = 10;
    size_t nDirLayer1  = 10;
    size_t nFileLayer2 = 50;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFileLayer1; ++fileIdx) {
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
        for (size_t fileIdx = 0; fileIdx < nFileLayer2; ++fileIdx) {
            string currTestRelPath = currTestRelDir + std::to_string(fileIdx) + ".txt";
            fileStrToSizeList[currTestRelPath] = 4 * KB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateBTreeMediumFiles() {
    // root
    auto testDirs = createTestRoot("BTreeMediumFilesTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFileLayer1 = 10;
    size_t nDirLayer1  = 10;
    size_t nFileLayer2 = 50;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFileLayer1; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = 4 * MB;
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
            fileStrToSizeList[currTestRelPath] = 2 * MB;
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
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = GB;
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
            fileStrToSizeList[currTestRelPath] = 10 * MB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateDTreeMediumFiles() {
    // root
    cout << "DTreeMediumFilesTest" << endl;
    auto testDirs = createTestRoot("DTreeMediumFilesTest");
    string testSrcDir  = std::get<0>(testDirs);
    string testDstcDir = std::get<1>(testDirs);

    size_t nFileLayer1 = 10;
    size_t nDirLayer1  = 2;
    size_t nFileLayer2 = 250;
    unordered_map<string, uint64_t> fileStrToSizeList;
    vector<string> dirStrLayer1List;
    // layer1
    for (size_t fileIdx = 0; fileIdx < nFileLayer1; ++fileIdx) {
        fileStrToSizeList[std::to_string(fileIdx) + ".txt"] = 4 * MB;
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
            fileStrToSizeList[currTestRelPath] = 2 * MB;
        }
        createFilesAndDirs(testSrcDir, fileStrToSizeList, dirStrLayer1List);
    }
}

void Evaluator::CreateDirectoryOnly() {

}

}