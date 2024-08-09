#include "FileManager.h"

#include <fstream>

using namespace lakot;

FileManager::~FileManager() {}

FileManager::FileManager() {}

std::string FileManager::createPath(const std::filesystem::path& pPath, const std::string& pFileOrDirectory)
{
    if (pPath.string().ends_with('/'))
    {
        return pPath.string() + pFileOrDirectory;
    }

    return pPath.string() + '/' + pFileOrDirectory;
}

bool FileManager::isFileExist(const std::filesystem::path& pPath)
{
    return std::filesystem::exists(pPath);
}

bool FileManager::isFileExist(const std::filesystem::path& pPath, const std::string& pFile)
{
    return std::filesystem::exists(createPath(pPath, pFile));
}

bool FileManager::hasParentPath(const std::filesystem::path& pPath) const
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
