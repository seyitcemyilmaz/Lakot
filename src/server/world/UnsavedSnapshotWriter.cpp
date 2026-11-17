#include "UnsavedSnapshotWriter.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <system_error>

using namespace lakot;

namespace
{
    const char* const kDirectoryName = "unsaved";

    std::string toJson(const CharacterRepository::CharacterSave& pSave)
    {
        std::ostringstream tOut;
        tOut.precision(9);

        const CharacterStats& tStats = pSave.stats;

        tOut << "{\n"
             << "  \"characterId\": " << pSave.characterId << ",\n"
             << "  \"mapId\": " << pSave.mapId << ",\n"
             << "  \"x\": " << pSave.x << ", \"y\": " << pSave.y << ", \"z\": " << pSave.z << ", \"yaw\": " << pSave.yaw << ",\n"
             << "  \"stats\": {\"level\": " << tStats.level << ", \"experience\": " << tStats.experience
             << ", \"health\": " << tStats.health << ", \"mana\": " << tStats.mana
             << ", \"strength\": " << tStats.strength << ", \"dexterity\": " << tStats.dexterity
             << ", \"intelligence\": " << tStats.intelligence << ", \"vitality\": " << tStats.vitality
             << ", \"gold\": " << tStats.gold << ", \"classId\": " << tStats.classId
             << ", \"pathId\": " << tStats.pathId << ", \"statPoints\": " << tStats.statPoints
             << ", \"starterGiven\": " << (tStats.starterGiven ? "true" : "false") << "},\n"
             << "  \"items\": [";

        for (size_t tIndex = 0; tIndex < pSave.items.size(); ++tIndex)
        {
            const ItemInstance& tItem = pSave.items[tIndex];

            tOut << (tIndex == 0 ? "\n    " : ",\n    ")
                 << "{\"templateId\": " << tItem.templateId << ", \"count\": " << tItem.count
                 << ", \"location\": " << static_cast<uint32_t>(tItem.location) << ", \"slot\": " << tItem.slot << "}";
        }

        tOut << (pSave.items.empty() ? "]\n}\n" : "\n  ]\n}\n");

        return tOut.str();
    }
}

std::optional<std::filesystem::path> UnsavedSnapshotWriter::write(const CharacterRepository::CharacterSave& pSave)
{
    std::error_code tError;
    std::filesystem::create_directories(kDirectoryName, tError);

    if (tError)
    {
        std::osyncstream(std::cerr) << "[SaveQueue] could not create unsaved directory: " << tError.message() << std::endl;
        return std::nullopt;
    }

    const auto tSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::filesystem::path tPath = std::filesystem::path(kDirectoryName) / (std::to_string(pSave.characterId) + "-" + std::to_string(tSeconds) + ".json");

    std::ofstream tFile(tPath, std::ios::binary | std::ios::trunc);
    tFile << toJson(pSave);
    tFile.flush();

    if (!tFile)
    {
        std::osyncstream(std::cerr) << "[SaveQueue] Could not write file: " << tPath.string() << std::endl;
        return std::nullopt;
    }

    return tPath;
}

size_t UnsavedSnapshotWriter::countFiles()
{
    std::error_code tError;
    std::filesystem::directory_iterator tIterator(kDirectoryName, tError);

    if (tError)
    {
        return 0;
    }

    size_t tCount = 0;

    for (const auto& tEntry : tIterator)
    {
        if (tEntry.path().extension() == ".json")
        {
            ++tCount;
        }
    }

    return tCount;
}
