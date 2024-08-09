#ifndef LAKOT_FILEMANAGER_H
#define LAKOT_FILEMANAGER_H

#include <filesystem>

namespace lakot {

class FileManager
{
public:
    virtual ~FileManager();
    FileManager();

    std::string createPath(const std::filesystem::path& pPath, const std::string& pFileOrDirectory);

    bool isFileExist(const std::filesystem::path& pPath);
    bool isFileExist(const std::filesystem::path& pPath, const std::string& pFile);

    bool hasParentPath(const std::filesystem::path& pPath) const;

    std::string getFileContent(const std::filesystem::path& pPath);

    std::string getParentDirectory(const std::filesystem::path& pPath);
    std::string getFileNameFromPath(const std::filesystem::path& pPath);
    std::string getDirectoryFromPath(const std::filesystem::path& pPath);
};

}

#endif
