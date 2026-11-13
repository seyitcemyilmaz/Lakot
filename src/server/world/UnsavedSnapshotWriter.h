#ifndef LAKOT_SERVER_UNSAVEDSNAPSHOTWRITER_H
#define LAKOT_SERVER_UNSAVEDSNAPSHOTWRITER_H

#include <cstddef>
#include <filesystem>
#include <optional>

#include "../database/repository/CharacterRepository.h"

namespace lakot
{

class UnsavedSnapshotWriter
{
public:
    UnsavedSnapshotWriter() = delete;
    ~UnsavedSnapshotWriter() = delete;

    static std::optional<std::filesystem::path> write(const CharacterRepository::CharacterSave& pSave);
    static size_t countFiles();
};

}

#endif
