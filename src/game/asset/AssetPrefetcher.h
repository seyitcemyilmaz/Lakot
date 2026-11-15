#ifndef LAKOT_ASSET_PREFETCHER_H
#define LAKOT_ASSET_PREFETCHER_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../graphics/model/Texture.h"

struct aiScene;

namespace Assimp
{
class Importer;
}

namespace lakot
{

class GarbageCollector;

enum class AssetKindType
{
    eModel,
    eSkinnedModel,
    eMotionSource
};

struct ParsedAsset
{
    AssetKindType kind{AssetKindType::eModel};
    std::string path;
    std::shared_ptr<Assimp::Importer> importer;
    const aiScene* scene{nullptr};
    std::unordered_map<std::string, DecodedImage> images;
    std::string error;
};

class AssetPrefetcher
{
public:
    explicit AssetPrefetcher(GarbageCollector& pGarbageCollector);
    ~AssetPrefetcher();

    AssetPrefetcher(const AssetPrefetcher&) = delete;
    AssetPrefetcher& operator=(const AssetPrefetcher&) = delete;

    bool request(AssetKindType pKind, const std::string& pPath, unsigned int pImportFlags);

    bool isPending(AssetKindType pKind, const std::string& pPath) const;

    std::optional<ParsedAsset> take(AssetKindType pKind, const std::string& pPath);
    std::optional<ParsedAsset> takeAny();

private:
    struct State;

    static std::string makeKey(AssetKindType pKind, const std::string& pPath);
    static ParsedAsset parse(AssetKindType pKind, const std::string& pPath, unsigned int pImportFlags);

    GarbageCollector& mGarbageCollector;
    std::shared_ptr<State> mState;
    std::unordered_set<std::string> mRequested;
};

}

#endif
