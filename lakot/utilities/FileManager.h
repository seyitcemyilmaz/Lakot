#ifndef LAKOT_FILEMANAGER_H
#define LAKOT_FILEMANAGER_H

#include <filesystem>

namespace lakot {

class FileManager
{
public:
    static std::string createPath(const std::filesystem::path& pPath, const std::string& pFileOrDirectory);

    static bool isFileExist(const std::filesystem::path& pPath);
    static bool isFileExist(const std::filesystem::path& pPath, const std::string& pFile);

    static bool hasParentPath(const std::filesystem::path& pPath);

    static std::string getFileContent(const std::filesystem::path& pPath);

    static std::string getParentDirectory(const std::filesystem::path& pPath);
    static std::string getFileNameFromPath(const std::filesystem::path& pPath);
    static std::string getDirectoryFromPath(const std::filesystem::path& pPath);

    static std::string getAssetPath();
    static void setAssetPath(const std::string& pAssetPath);

private:
    static std::string mAssetPath;
};

}

#endif
