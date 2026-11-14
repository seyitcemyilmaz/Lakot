#include "AssetPrefetcher.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "GarbageCollector.h"

#include "ModelTextureSourceLoader.h"

using namespace lakot;

struct AssetPrefetcher::State
{
    std::mutex mutex;
    std::condition_variable readyChanged;
    std::unordered_map<std::string, ParsedAsset> ready;
    std::atomic<bool> isCancelled{false};
};

AssetPrefetcher::AssetPrefetcher(GarbageCollector& pGarbageCollector)
    : mGarbageCollector(pGarbageCollector)
    , mState(std::make_shared<State>())
{

}

AssetPrefetcher::~AssetPrefetcher()
{
    mState->isCancelled = true;
}

std::string AssetPrefetcher::makeKey(AssetKindType pKind, const std::string& pPath)
{
    return std::to_string(static_cast<int>(pKind)) + "|" + pPath;
}

ParsedAsset AssetPrefetcher::parse(AssetKindType pKind, const std::string& pPath, unsigned int pImportFlags)
{
    ParsedAsset tParsed;
    tParsed.kind = pKind;
    tParsed.path = pPath;
    tParsed.importer = std::make_shared<Assimp::Importer>();
    tParsed.scene = tParsed.importer->ReadFile(pPath, pImportFlags);

    if (!tParsed.scene)
    {
        tParsed.error = "cannot read " + pPath + ": " + tParsed.importer->GetErrorString();
        return tParsed;
    }

    for (const ModelTextureSource& tSource : ModelTextureSourceLoader::list(*tParsed.scene, pPath))
    {
        std::string tError;
        DecodedImage tImage;

        if (tSource.embedded)
        {
            if (tSource.embedded->mHeight != 0)
            {
                continue;
            }

            tImage = Texture::decodeMemory(reinterpret_cast<const unsigned char*>(tSource.embedded->pcData), tSource.embedded->mWidth, tError);
        }
        else
        {
            tImage = Texture::decodeFile(tSource.key, tError);
        }

        if (!tImage.pixels.empty())
        {
            tParsed.images.emplace(tSource.key, std::move(tImage));
        }
    }

    return tParsed;
}

bool AssetPrefetcher::request(AssetKindType pKind, const std::string& pPath, unsigned int pImportFlags)
{
    std::string tKey = makeKey(pKind, pPath);

    if (!mRequested.insert(tKey).second)
    {
        return false;
    }

    std::shared_ptr<State> tState = mState;

    mGarbageCollector.addAsynchronousTask([tState, tKey, pKind, pPath, pImportFlags]()
    {
        if (tState->isCancelled)
        {
            return;
        }

        ParsedAsset tParsed;

        try
        {
            tParsed = parse(pKind, pPath, pImportFlags);
        }
        catch (const std::exception& tException)
        {
            tParsed = ParsedAsset();
            tParsed.kind = pKind;
            tParsed.path = pPath;
            tParsed.error = std::string("cannot read ") + pPath + ": " + tException.what();
        }

        {
            std::lock_guard<std::mutex> tLock(tState->mutex);
            tState->ready.emplace(tKey, std::move(tParsed));
        }

        tState->readyChanged.notify_all();
    });

    return true;
}

std::optional<ParsedAsset> AssetPrefetcher::take(AssetKindType pKind, const std::string& pPath)
{
    std::string tKey = makeKey(pKind, pPath);

    std::unique_lock<std::mutex> tLock(mState->mutex);

    if (mRequested.count(tKey) != 0)
    {
        mState->readyChanged.wait(tLock, [this, &tKey]() { return mState->ready.count(tKey) != 0; });
    }

    auto tIterator = mState->ready.find(tKey);

    if (tIterator == mState->ready.end())
    {
        return std::nullopt;
    }

    std::optional<ParsedAsset> tResult(std::move(tIterator->second));
    mState->ready.erase(tIterator);
    mRequested.erase(tKey);
    return tResult;
}

std::optional<ParsedAsset> AssetPrefetcher::takeAny()
{
    std::lock_guard<std::mutex> tLock(mState->mutex);

    if (mState->ready.empty())
    {
        return std::nullopt;
    }

    auto tIterator = mState->ready.begin();
    std::optional<ParsedAsset> tResult(std::move(tIterator->second));
    mRequested.erase(tIterator->first);
    mState->ready.erase(tIterator);
    return tResult;
}
