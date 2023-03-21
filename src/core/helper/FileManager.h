#ifndef LAKOT_FILEMANAGER_H
#define LAKOT_FILEMANAGER_H

#include "platform.h"

class FileManager {
private:
    static FileManager* mInstance;

    FileManager();
public:
    static FileManager* getInstance();

    bool isFileExist(std::filesystem::path pPath);
    bool isFileExist(std::filesystem::path pPath, std::string pFile);

    bool hasParentPath(std::filesystem::path pPath);

    std::string getParentDirectory(std::filesystem::path pPath);
    std::string getFileNameFromPath(std::filesystem::path pPath);
    std::string getDirectoryFromPath(std::filesystem::path pPath);

    std::string createPath(std::filesystem::path pPath, std::string pFileOrDirectory);
};

#endif
