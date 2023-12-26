#ifndef LAKOT_FILEMANAGER_H
#define LAKOT_FILEMANAGER_H

#include "platform/Platform.h"

class FileManager {
private:
    static FileManager* mInstance;

    FileManager();

public:
    static FileManager* getInstance();

    bool isFileExist(const std::filesystem::path& pPath);
    bool isFileExist(const std::filesystem::path& pPath, const std::string& pFile);

    bool hasParentPath(const std::filesystem::path& pPath) const;

    std::string getParentDirectory(const std::filesystem::path& pPath);
    std::string getFileNameFromPath(const std::filesystem::path& pPath);
    std::string getDirectoryFromPath(const std::filesystem::path& pPath);

    std::string createPath(const std::filesystem::path& pPath, const std::string& pFileOrDirectory);

    std::string getFileContent(const std::filesystem::path& pPath);

    bool operator==(const FileManager& other) = delete;
};

#endif
