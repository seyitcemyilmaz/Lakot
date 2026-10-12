#ifndef LAKOT_CLASSCATALOG_H
#define LAKOT_CLASSCATALOG_H

#include <cstdint>
#include <string>
#include <vector>

namespace lakot
{

struct ClassPath
{
    uint32_t id = 0;
    std::string key;
    std::string name;
};

struct ClassDefinition
{
    uint32_t id = 0;
    std::string key;
    std::string name;
    std::vector<std::string> mainStats;
    uint32_t startStrength = 1;
    uint32_t startDexterity = 1;
    uint32_t startIntelligence = 1;
    uint32_t startVitality = 1;
    std::vector<ClassPath> paths;

    const ClassPath* findPath(uint32_t pPathId) const;
};

class ClassCatalog
{
public:
    bool load(const std::string& pDataDirectory, std::string& pError);

    const ClassDefinition* find(uint32_t pClassId) const;
    const std::vector<ClassDefinition>& getAll() const;
    uint32_t getStatPointsPerLevel() const;
    uint32_t getStatCap() const;
    uint32_t getPathUnlockLevel() const;

private:
    std::vector<ClassDefinition> mClasses;
    uint32_t mStatPointsPerLevel = 0;
    uint32_t mStatCap = 0;
    uint32_t mPathUnlockLevel = 0;
};

}

#endif
