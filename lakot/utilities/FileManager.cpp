#include "FileManager.h"

#include <fstream>
#include <string>

using namespace lakot;

std::string FileManager::mAssetPath = "";

std::string FileManager::createPath(const std::filesystem::path& pPath, const std::string& pFileOrDirectory)
{
    std::string tPath = pPath.string();
    unsigned int tPathLength = static_cast<unsigned int>(tPath.size());

    if (tPathLength > 0)
    {
        char tLastElement = tPath.at(tPathLength - 1);

        if (tLastElement == '/' || tLastElement == '\\')
        {
            return tPath + pFileOrDirectory;
        }
    }

    return tPath + '/' + pFileOrDirectory;
}

bool FileManager::isFileExist(const std::filesystem::path& pPath)
{
    return std::filesystem::exists(pPath);
}

bool FileManager::isFileExist(const std::filesystem::path& pPath, const std::string& pFile)
{
    return std::filesystem::exists(createPath(pPath, pFile));
}

bool FileManager::hasParentPath(const std::filesystem::path& pPath)
{
    return pPath.has_parent_path();
}

std::string FileManager::getFileContent(const std::filesystem::path& pPath)
{
    std::ifstream tFile(pPath);

    if (!tFile.is_open())
    {
        throw "File is not found.";
    }

    std::string tFileContent;
    std::getline(tFile, tFileContent, '\0');

    return tFileContent;
}

std::string FileManager::getParentDirectory(const std::filesystem::path& pPath)
{
    return pPath.parent_path().string();
}

std::string FileManager::getFileNameFromPath(const std::filesystem::path& pPath)
{
    std::string tPath = pPath.string();

    if (tPath.find('/') != std::string::npos)
    {
        tPath = tPath.substr(tPath.find_last_of('/') + 1);
    }

    if (tPath.find('\\') != std::string::npos)
    {
        tPath = tPath.substr(tPath.find_last_of('\\') + 1);
    }

    return tPath;
}

std::string FileManager::getDirectoryFromPath(const std::filesystem::path& pPath)
{
    return pPath.string().substr(0, pPath.string().find_last_of('/'));
}

std::string FileManager::getAssetPath()
{
    return mAssetPath;
}

void FileManager::setAssetPath(const std::string& pAssetPath)
{
    mAssetPath = pAssetPath;
}
