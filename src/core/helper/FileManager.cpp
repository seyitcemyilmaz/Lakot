#include "FileManager.h"

FileManager* FileManager::mInstance = nullptr;

FileManager* FileManager::getInstance() {
	if (!mInstance) {
		mInstance = new FileManager();
	}

	return mInstance;
}

FileManager::FileManager() { }

bool FileManager::hasParentPath(std::filesystem::path pPath) {
	return pPath.has_parent_path();
}

std::string FileManager::getParentDirectory(std::filesystem::path pPath) {
	return pPath.parent_path().string();
}

bool FileManager::isFileExist(std::filesystem::path pPath) {
	return std::filesystem::exists(pPath);
}

bool FileManager::isFileExist(std::filesystem::path pPath, std::string pFile) {
	return std::filesystem::exists(createPath(pPath, pFile));
}

std::string FileManager::getFileNameFromPath(std::filesystem::path pPath) {
	std::string tPath = pPath.string();

	if (tPath.find('/') != std::string::npos) {
		tPath = tPath.substr(tPath.find_last_of('/') + 1);
	}

	if (tPath.find('\\') != std::string::npos) {
		tPath = tPath.substr(tPath.find_last_of('\\') + 1);
	}

	return tPath;
}

std::string FileManager::getDirectoryFromPath(std::filesystem::path pPath) {
	return pPath.string().substr(0, pPath.string().find_last_of(LAKOT_FILE_PATH_SEPARATOR));
}

std::string FileManager::createPath(std::filesystem::path pPath, std::string pFileOrDirectory) {
	if (pPath.string().ends_with(LAKOT_FILE_PATH_SEPARATOR)) {
		return pPath.string() + pFileOrDirectory;
	}

	return pPath.string() + LAKOT_FILE_PATH_SEPARATOR + pFileOrDirectory;
}
















