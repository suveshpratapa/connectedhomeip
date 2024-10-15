/*
 *    Copyright (c) 2023 Apple Inc. All rights reserved.
 *
 *    Apple Matter Extensions is licensed under Apple Inc.'s MFi Sample Code
 *    License Agreement, which is contained in the LICENSE file distributed with
 *    the Apple Matter Extensions, and only to those who accept that license.
 */

#include <apple/support/TLV8.h>
#include <lib/core/CHIPEncoding.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>

#include <type_traits>

namespace Apple {
namespace TLV8 {

namespace Encoding = chip::Encoding::LittleEndian;

void TLVReader::Init(const uint8_t * data, size_t dataLen)
{
    mIsRoot          = true;
    mSource.root.buf = data;
    mSource.root.end = data + dataLen;
    Reset(StateIncorrect);
}

void TLVReader::Init(TLVReader & parent)
{
    mIsRoot              = false;
    mSource.child.parent = &parent;
    Reset(StateIncorrect);
}

void TLVReader::Reset(enum ErrorState state)
{
    mTag               = 0;
    mTagLength         = 0;
    mTagRemaining      = 0;
    mFragmentRemaining = state;
}

enum TLVReader::ErrorState TLVReader::ErrorState()
{
    return static_cast<enum ErrorState>(mTagRemaining ? 0 : mFragmentRemaining);
}

CHIP_ERROR TLVReader::Next()
{
    VerifyOrReturnError(ErrorState() != StateUnderrun, CHIP_ERROR_TLV_UNDERRUN);
    VerifyOrDie(ReadTagData(nullptr, mTagRemaining));
    VerifyOrReturnError(Remaining(), CHIP_END_OF_TLV, Reset(StateNoError));

    // Read tag and length and then mark the start of the tag data in the source
    uint8_t tl[2];
    VerifyOrReturnError(Read(tl, 2), CHIP_ERROR_TLV_UNDERRUN, Reset(StateUnderrun));
    MarkSource();

    // Skip over the data and scan through continuation fragments (TLV items with the same tag)
    // to determine the total tag data length. Underrun errors are also caught during this pass.
    mTag                   = tl[0];
    mMarkFragmentRemaining = tl[1];
    mTagLength             = tl[1];
    while (tl[1])
    {
        VerifyOrReturnError(Read(nullptr, tl[1]), CHIP_ERROR_TLV_UNDERRUN, Reset(StateUnderrun));

        auto remaining = Remaining();
        if (!remaining)
            break;

        bool haveLength = (remaining >= 2);
        VerifyOrDie(Read(tl, haveLength ? 2 : 1));
        if (tl[0] != mTag)
            break;

        VerifyOrReturnError(haveLength, CHIP_ERROR_TLV_UNDERRUN, Reset(StateUnderrun));
        mTagLength += tl[1];
    }
    mMarkTagRemaining = mTagLength;

    // Length and bytes remaining are now populated and remembered, rewind so tag data can be read.
    Rewind();
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::Next(Tag expectedTag)
{
    ReturnErrorOnFailure(Next());
    VerifyOrReturnError(GetTag() == expectedTag, CHIP_ERROR_UNEXPECTED_TLV_ELEMENT);
    return CHIP_NO_ERROR;
}

void TLVReader::MarkSource()
{
    if (mIsRoot)
        mSource.root.mark = mSource.root.buf;
    else
        mSource.child.parent->Mark();
}

// Remember the position within the current tag recursively up the chain of readers
void TLVReader::Mark()
{
    mMarkTagRemaining      = mTagRemaining;
    mMarkFragmentRemaining = mFragmentRemaining;
    MarkSource();
}

// Return the reader to the position remembered with Mark()
void TLVReader::Rewind()
{
    mTagRemaining      = mMarkTagRemaining;
    mFragmentRemaining = mMarkFragmentRemaining;
    if (mIsRoot)
        mSource.root.buf = mSource.root.mark;
    else
        mSource.child.parent->Rewind();
}

// Bytes remaining to be Read()
size_t TLVReader::Remaining()
{
    return (mIsRoot) ? static_cast<size_t>(mSource.root.end - mSource.root.buf) : mSource.child.parent->mTagRemaining;
}

// Read source data into buf (or discard if buf == nullptr)
bool TLVReader::Read(uint8_t * buf, size_t count)
{
    VerifyOrReturnValue(count, true);
    VerifyOrReturnValue(mIsRoot, mSource.child.parent->ReadTagData(buf, count));
    VerifyOrReturnValue(mSource.root.buf + count <= mSource.root.end, false);
    if (buf)
        memcpy(buf, mSource.root.buf, count);
    mSource.root.buf += count;
    return true;
}

// Read tag contents into buf (or discard if buf == nullptr)
bool TLVReader::ReadTagData(uint8_t * buf, size_t count)
{
    VerifyOrReturnValue(count, true);
    VerifyOrReturnValue(mTagRemaining >= count, false);
    mTagRemaining -= count;
    while (count > mFragmentRemaining)
    {
        count -= mFragmentRemaining;
        if (buf && count >= 2)
        {
            // >= 2 bytes of spare buffer space, Read() data + next tag and length with one call
            VerifyOrDie(Read(buf, mFragmentRemaining + 2));
            buf += mFragmentRemaining;
            VerifyOrDie(buf[0] == mTag);
            VerifyOrDie((mFragmentRemaining = buf[1]) > 0);
        }
        else
        {
            // no buffer or not enough space, perform two separate reads
            VerifyOrDie(Read(buf, mFragmentRemaining));
            if (buf)
                buf += mFragmentRemaining;
            uint8_t tl[2];
            VerifyOrDie(Read(tl, 2));
            VerifyOrDie(tl[0] == mTag);
            VerifyOrDie((mFragmentRemaining = tl[1]) > 0);
        }
    }
    VerifyOrDie(Read(buf, count));
    mFragmentRemaining -= static_cast<uint8_t>(count);
    return true;
}

CHIP_ERROR TLVReader::OpenContainer(TLVReader & containerReader)
{
    VerifyOrReturnError(!ErrorState(), CHIP_ERROR_INCORRECT_STATE);
    containerReader.Init(*this);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::CloseContainer(TLVReader & containerReader)
{
    VerifyOrDie(!containerReader.mIsRoot && containerReader.mSource.child.parent == this);
    for (;;)
    {
        auto err = containerReader.Next();
        if (err == CHIP_END_OF_TLV)
            break;
        ReturnErrorOnFailure(err);
    }
    Reset(StateIncorrect);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::DecodeTLV(void * tlv, decode_ptr decodeFn)
{
    TLVReader nested;
    ReturnErrorOnFailure(OpenContainer(nested));
    ReturnErrorOnFailure(decodeFn(tlv, nested));
    ReturnErrorOnFailure(CloseContainer(nested));
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::DecodeTLV(void * tlv, decode_ptr decodeFn, uint8_t const * data, size_t dataLen)
{
    TLVReader reader;
    reader.Init(data, dataLen);
    ReturnErrorOnFailure(decodeFn(tlv, reader));
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::GetBytes(uint8_t * buf, size_t bufSize, bool terminate)
{
    VerifyOrReturnError(!ErrorState(), CHIP_ERROR_INCORRECT_STATE);
    auto length = GetLength();
    VerifyOrReturnError(length + (terminate ? 1 : 0) <= bufSize, CHIP_ERROR_BUFFER_TOO_SMALL);
    VerifyOrDie(ReadTagData(buf, length));
    if (terminate)
        buf[length] = 0;
    Rewind();
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::DupBytes(uint8_t *& data, uint32_t * dataLen, bool terminate)
{
    VerifyOrReturnError(!ErrorState(), CHIP_ERROR_INCORRECT_STATE);
    auto length = GetLength();

    auto bufSize = static_cast<uint32_t>(length + (terminate ? 1 : 0));
    VerifyOrReturnError(bufSize >= length, CHIP_ERROR_NO_MEMORY);
    auto buf = static_cast<uint8_t *>(chip::Platform::MemoryAlloc(bufSize));
    VerifyOrReturnError(buf, CHIP_ERROR_NO_MEMORY);

    VerifyOrDie(ReadTagData(buf, length));
    if (terminate)
        buf[length] = 0;
    if (dataLen)
        *dataLen = static_cast<uint32_t>(length);
    data = buf;
    Rewind();
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::GetBytes(OwnedByteSpan & span)
{
    uint8_t * data;
    uint32_t dataLen;
    ReturnErrorOnFailure(DupBytes(data, &dataLen, false));
    span.Adopt(data, dataLen);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::GetString(OwnedCharSpan & span)
{
    uint8_t * data;
    uint32_t dataLen;
    ReturnErrorOnFailure(DupBytes(data, &dataLen, true));
    span.Adopt(reinterpret_cast<char const *>(data), dataLen + 1);
    span.reduce_size(dataLen);
    return CHIP_NO_ERROR;
}

template <typename T, typename S>
static inline T signed_cast(S v)
{
    return static_cast<T>(static_cast<std::conditional_t<std::is_signed<T>::value, std::make_signed_t<S>, S>>(v));
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value, CHIP_ERROR> TLVReader::Get(T & v)
{
    auto constexpr N = sizeof(T);
    static_assert(N <= 8, "integer type must be <= 64 bits");
    auto length = GetLength();
    VerifyOrReturnError(length <= N, CHIP_ERROR_WRONG_TLV_TYPE);
    switch (length)
    {
    case 1:
        v = signed_cast<T>(ReadUInt8());
        break;
    case 2:
        v = signed_cast<T>(ReadUInt16());
        break;
    case 4:
        v = signed_cast<T>(ReadUInt32());
        break;
    case 8:
        v = signed_cast<T>(ReadUInt64());
        break;
    default:
        return CHIP_ERROR_WRONG_TLV_TYPE;
    }
    Rewind();
    return CHIP_NO_ERROR;
}

template CHIP_ERROR TLVReader::Get(bool & v);
template CHIP_ERROR TLVReader::Get(uint8_t & v);
template CHIP_ERROR TLVReader::Get(int8_t & v);
template CHIP_ERROR TLVReader::Get(uint16_t & v);
template CHIP_ERROR TLVReader::Get(int16_t & v);
template CHIP_ERROR TLVReader::Get(uint32_t & v);
template CHIP_ERROR TLVReader::Get(int32_t & v);
template CHIP_ERROR TLVReader::Get(uint64_t & v);
template CHIP_ERROR TLVReader::Get(int64_t & v);

CHIP_ERROR TLVReader::Get(float & v)
{
    VerifyOrReturnError(GetLength() == sizeof(v), CHIP_ERROR_WRONG_TLV_TYPE);
    auto bits = ReadUInt32();
    static_assert(sizeof(bits) == sizeof(v), "float must be 32 bits");
    memcpy(&v, &bits, sizeof(v));
    Rewind();
    return CHIP_NO_ERROR;
}

CHIP_ERROR TLVReader::Get(double & v)
{
    VerifyOrReturnError(GetLength() == sizeof(v), CHIP_ERROR_WRONG_TLV_TYPE);
    auto bits = ReadUInt64();
    static_assert(sizeof(bits) == sizeof(v), "double must be 64 bits");
    memcpy(&v, &bits, sizeof(v));
    Rewind();
    return CHIP_NO_ERROR;
}

uint8_t TLVReader::ReadUInt8()
{
    uint8_t v[1];
    VerifyOrDie(ReadTagData(v, sizeof(v)));
    return v[0];
}

uint16_t TLVReader::ReadUInt16()
{
    uint8_t v[2];
    VerifyOrDie(ReadTagData(v, sizeof(v)));
    return Encoding::Get16(v);
}

uint32_t TLVReader::ReadUInt32()
{
    uint8_t v[4];
    VerifyOrDie(ReadTagData(v, sizeof(v)));
    return Encoding::Get32(v);
}

uint64_t TLVReader::ReadUInt64()
{
    uint8_t v[8];
    VerifyOrDie(ReadTagData(v, sizeof(v)));
    return Encoding::Get64(v);
}

} // namespace TLV8
} // namespace Apple
