#ifndef LAKOT_SERVER_WIREWRITER_H
#define LAKOT_SERVER_WIREWRITER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <google/protobuf/wire_format_lite.h>

namespace lakot
{

class WireWriter
{
public:
    WireWriter() = delete;

    static void appendVarint(std::vector<char>& pOut, uint64_t pValue)
    {
        while (pValue >= 0x80)
        {
            pOut.push_back(static_cast<char>(pValue | 0x80));
            pValue >>= 7;
        }

        pOut.push_back(static_cast<char>(pValue));
    }

    static void appendLengthDelimitedHeader(std::vector<char>& pOut, int pFieldNumber, size_t pLength)
    {
        appendVarint(pOut, google::protobuf::internal::WireFormatLite::MakeTag(
            pFieldNumber, google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
        appendVarint(pOut, pLength);
    }

    static size_t getLengthDelimitedHeaderSize(int pFieldNumber, size_t pLength)
    {
        return getVarintSize(google::protobuf::internal::WireFormatLite::MakeTag(
                   pFieldNumber, google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED))
            + getVarintSize(pLength);
    }

    static void appendVarintField(std::vector<char>& pOut, int pFieldNumber, uint64_t pValue)
    {
        appendVarint(pOut, google::protobuf::internal::WireFormatLite::MakeTag(
            pFieldNumber, google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT));
        appendVarint(pOut, pValue);
    }

    static size_t getVarintSize(uint64_t pValue)
    {
        size_t tSize = 1;

        while (pValue >= 0x80)
        {
            pValue >>= 7;
            ++tSize;
        }

        return tSize;
    }

    static void appendBytes(std::vector<char>& pOut, const char* pData, size_t pSize)
    {
        pOut.insert(pOut.end(), pData, pData + pSize);
    }
};

}

#endif
