#include "MapImageFactory.h"

#include <algorithm>
#include <cmath>

using namespace lakot;

namespace
{
    const float kWaterDeep[3] = { 34.0f, 82.0f, 132.0f };
    const float kWaterShallow[3] = { 58.0f, 118.0f, 168.0f };
    const float kShoreBand = 0.35f;
}

MapImage MapImageFactory::create(const MapData& pMap, int pSize)
{
    MapImage tImage;
    tImage.size = pSize;
    tImage.rgba.resize(static_cast<size_t>(pSize) * pSize * 4);

    const float tWorldSize = pMap.getWorldSize();
    const float tHalf = tWorldSize * 0.5f;
    const float tPixelSize = tWorldSize / static_cast<float>(pSize);
    const float tWater = pMap.getWaterLevel();
    const int tLastVertex = pMap.getResolution() - 1;

    for (int tRow = 0; tRow < pSize; ++tRow)
    {
        for (int tColumn = 0; tColumn < pSize; ++tColumn)
        {
            float tX = -tHalf + (tColumn + 0.5f) * tPixelSize;
            float tZ = -tHalf + (tRow + 0.5f) * tPixelSize;

            float tHeight = pMap.getHeightAt(tX, tZ);

            float tGridX = std::clamp((tX + tHalf) / pMap.getGridSpacing(), 0.0f, static_cast<float>(tLastVertex));
            float tGridZ = std::clamp((tZ + tHalf) / pMap.getGridSpacing(), 0.0f, static_cast<float>(tLastVertex));
            int tVertexX = std::min(static_cast<int>(tGridX), tLastVertex - 1);
            int tVertexZ = std::min(static_cast<int>(tGridZ), tLastVertex - 1);
            float tFractionX = tGridX - tVertexX;
            float tFractionZ = tGridZ - tVertexZ;

            auto tColor00 = pMap.getVertexColor(tVertexX, tVertexZ);
            auto tColor10 = pMap.getVertexColor(tVertexX + 1, tVertexZ);
            auto tColor01 = pMap.getVertexColor(tVertexX, tVertexZ + 1);
            auto tColor11 = pMap.getVertexColor(tVertexX + 1, tVertexZ + 1);

            // Light from the north-west, like most printed maps.
            float tSlopeX = pMap.getHeightAt(tX + tPixelSize, tZ) - pMap.getHeightAt(tX - tPixelSize, tZ);
            float tSlopeZ = pMap.getHeightAt(tX, tZ + tPixelSize) - pMap.getHeightAt(tX, tZ - tPixelSize);
            float tShade = std::clamp(1.0f - (tSlopeX + tSlopeZ) * 0.35f / tPixelSize, 0.55f, 1.3f);

            float tWaterWeight = std::clamp((tWater + kShoreBand - tHeight) / (2.0f * kShoreBand), 0.0f, 1.0f);
            float tDepth = std::clamp((tWater - tHeight) / 6.0f, 0.0f, 1.0f);

            float tColor[3];

            for (int tChannel = 0; tChannel < 3; ++tChannel)
            {
                float tTop = tColor00[tChannel] + (tColor10[tChannel] - tColor00[tChannel]) * tFractionX;
                float tBottom = tColor01[tChannel] + (tColor11[tChannel] - tColor01[tChannel]) * tFractionX;
                float tGround = (tTop + (tBottom - tTop) * tFractionZ) * tShade;
                float tWaterColor = kWaterShallow[tChannel] + (kWaterDeep[tChannel] - kWaterShallow[tChannel]) * tDepth;

                tColor[tChannel] = tGround + (tWaterColor - tGround) * tWaterWeight;
            }

            size_t tIndex = (static_cast<size_t>(tRow) * pSize + tColumn) * 4;

            for (int tChannel = 0; tChannel < 3; ++tChannel)
            {
                tImage.rgba[tIndex + tChannel] = static_cast<uint8_t>(std::clamp(tColor[tChannel], 0.0f, 255.0f));
            }

            tImage.rgba[tIndex + 3] = 255;
        }
    }

    // Objects as seen from above: whatever sits highest is drawn last, so a
    // roof covers its walls and a canopy its trunk.
    std::vector<const MapObject*> tObjects;
    tObjects.reserve(pMap.getObjects().size());

    for (const MapObject& tObject : pMap.getObjects())
    {
        tObjects.push_back(&tObject);
    }

    std::stable_sort(tObjects.begin(), tObjects.end(), [](const MapObject* pA, const MapObject* pB)
    {
        return pA->elevation + pA->sizeY < pB->elevation + pB->sizeY;
    });

    for (const MapObject* tObject : tObjects)
    {
        int tLeft = static_cast<int>(std::floor((tObject->x - tObject->sizeX * 0.5f + tHalf) / tPixelSize));
        int tRight = static_cast<int>(std::ceil((tObject->x + tObject->sizeX * 0.5f + tHalf) / tPixelSize));
        int tTop = static_cast<int>(std::floor((tObject->z - tObject->sizeZ * 0.5f + tHalf) / tPixelSize));
        int tBottom = static_cast<int>(std::ceil((tObject->z + tObject->sizeZ * 0.5f + tHalf) / tPixelSize));

        tLeft = std::max(tLeft, 0);
        tTop = std::max(tTop, 0);
        tRight = std::min(std::max(tRight, tLeft + 1), pSize);
        tBottom = std::min(std::max(tBottom, tTop + 1), pSize);

        for (int tRow = tTop; tRow < tBottom; ++tRow)
        {
            for (int tColumn = tLeft; tColumn < tRight; ++tColumn)
            {
                size_t tIndex = (static_cast<size_t>(tRow) * pSize + tColumn) * 4;

                for (int tChannel = 0; tChannel < 3; ++tChannel)
                {
                    tImage.rgba[tIndex + tChannel] = static_cast<uint8_t>(std::clamp(tObject->color[tChannel] * 255.0f, 0.0f, 255.0f));
                }
            }
        }
    }

    return tImage;
}
