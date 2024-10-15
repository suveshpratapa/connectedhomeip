/*
 *    Copyright (c) 2020-2023 Project CHIP Authors
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    This file is based on Project CHIP TLVReader.h and was modified by
 *    Apple Inc. Such modifications are licensed under Apple Inc.'s MFi Sample
 *    Code License Agreement, which is contained in the LICENSE file distributed
 *    with the Apple Matter Extensions, and only to those who accept that
 *    license. For the avoidance of doubt, this does not modify the License.
 *
 *    Copyright (c) 2023 Apple Inc. All Rights Reserved.
 */

#pragma once

#include <apple/support/OwnedSpan.h>
#include <lib/core/CHIPError.h>
#include <lib/core/Optional.h>
#include <lib/support/BitFlags.h>
#include <lib/support/Span.h>

#include <type_traits>

namespace Apple {
namespace TLV8 {

/**
 * TLV8 tags are 8 bit integers (0-255) and are always contextual.
 */
typedef uint8_t Tag;

/**
 * Provides a memory efficient parser for data encoded in Apple TLV8 format.
 *
 * The interface of this class mimics the CHIP TLVReader as far as possible,
 * however some APIs are not available on this class. In particular:
 *
 * - TLV8 elements are untyped so GetType() is not supported. Consumers are
 *   expected to infer the type of an element from its tag.
 *
 * - Elements longer than 255 bytes are represented as a sequence of elements
 *   with the same tag; they are coalesced into a single logical element by
 *   the reader automatically.
 *
 * - Parsing of nested TLVs requires the use of OpenContainer() along with
 *   a child TLVReader instance; EnterContainer() is not supported. TLV8 also
 *   does not support anonymous containers which are conventionally used to
 *   wrap a top-level structure in Matter TLV. In TLV8 the Decode() method of
 *   a structure will directly read fields from the provided reader without
 *   any nesting. DecodeTLV() can be used to call such a method with a
 *   nested reader initialized using OpenContainer().
 *
 * - TLVReader instances cannot be copied or initialized from another reader.
 *   (In the case of a nested reader this would lead to the copy erroneously
 *   sharing and invalidating the state of the parent reader.)
 *
 * All numeric values (integer and floating point) are parsed as little endian.
 */
class TLVReader
{
public:
    TLVReader()                  = default;
    TLVReader(const TLVReader &) = delete;
    TLVReader & operator=(const TLVReader &) = delete;

    void Init(const chip::ByteSpan & data) { Init(data.data(), data.size()); }

    template <size_t N>
    void Init(const uint8_t (&data)[N])
    {
        Init(data, N);
    }

    void Init(const uint8_t * data, size_t dataLen);

    Tag GetTag() const { return mTag; }

    size_t GetLength() const { return mTagLength; }

    CHIP_ERROR Next();
    CHIP_ERROR Next(Tag expectedTag);

    // Initializes containerReader to read the data of the current tag. This
    // method can only be called once for a particular tag, and a matching call
    // to CloseContainer() must be made before any other methods are called on
    // this reader. An OpenContainer() / CloseContainer() sequence has the
    // effect of "consuming" the data of the current tag, so the same data is
    // not available for subsequent calls to Get() and its variants.
    CHIP_ERROR OpenContainer(TLVReader & containerReader);
    CHIP_ERROR CloseContainer(TLVReader & containerReader);

    CHIP_ERROR GetBytes(uint8_t * buf, size_t bufSize) { return GetBytes(buf, bufSize, false); }
    CHIP_ERROR GetString(char * buf, size_t bufSize) { return GetBytes(reinterpret_cast<uint8_t *>(buf), bufSize, true); }

    CHIP_ERROR DupBytes(uint8_t *& buf, uint32_t & dataLen) { return DupBytes(buf, &dataLen, false); }
    CHIP_ERROR DupString(char *& buf) { return DupBytes(reinterpret_cast<uint8_t *&>(buf), nullptr, true); };

    CHIP_ERROR GetBytes(OwnedByteSpan & span);
    inline CHIP_ERROR GetBytes(chip::Optional<OwnedByteSpan> & span) { return GetBytes(span.Emplace()); }

    CHIP_ERROR GetString(OwnedCharSpan & span);
    inline CHIP_ERROR GetString(chip::Optional<OwnedCharSpan> & span) { return GetString(span.Emplace()); }

    CHIP_ERROR Get(float & v);
    CHIP_ERROR Get(double & v);

    template <typename T> // bool, int_*t, uint*_t
    std::enable_if_t<std::is_integral<T>::value, CHIP_ERROR> Get(T & v);

    template <typename T>
    std::enable_if_t<std::is_enum<T>::value, CHIP_ERROR> Get(T & v)
    {
        std::underlying_type_t<T> raw;
        ReturnErrorOnFailure(Get(raw));
        v = static_cast<T>(raw);
        return CHIP_NO_ERROR;
    }

    template <typename T>
    CHIP_ERROR Get(chip::BitFlags<T> & v)
    {
        std::underlying_type_t<T> raw;
        ReturnErrorOnFailure(Get(raw));
        v.SetRaw(raw);
        return CHIP_NO_ERROR;
    }

    template <typename T>
    inline CHIP_ERROR Get(chip::Optional<T> & val)
    {
        return Get(val.Emplace());
    }

    // Decode the current tag as a nested TLV
    template <typename T>
    inline CHIP_ERROR DecodeTLV(T & tlv)
    {
        return DecodeTLV(&tlv, DecodeFn(tlv));
    }

    // Decode the current tag as an optional nested TLV
    template <typename T>
    inline CHIP_ERROR DecodeTLV(chip::Optional<T> & tlv)
    {
        return DecodeTLV(tlv.Emplace());
    }

    // Decode a top-level TLV from the specified data
    template <typename T>
    static inline CHIP_ERROR DecodeTLV(T & tlv, uint8_t const * data, size_t dataLen)
    {
        return DecodeTLV(&tlv, DecodeFn(tlv), data, dataLen);
    }

    // Decode a top-level TLV from the specified span
    template <typename T>
    static inline CHIP_ERROR DecodeTLV(T & tlv, const chip::ByteSpan & span)
    {
        return DecodeTLV(&tlv, DecodeFn(tlv), span.data(), span.size());
    }

private:
    bool mIsRoot               = true;           // discriminator for mSource
    uint8_t mTag               = 0;              // current tag
    uint8_t mFragmentRemaining = StateIncorrect; // bytes remaining in fragment, or an ErrorState if != 0 and tagRemaining == 0
    uint8_t mMarkFragmentRemaining;              // Mark() copy of mFragmentRemaining
    size_t mTagLength    = 0;                    // total content length of current tag (all fragments)
    size_t mTagRemaining = 0;                    // bytes remaining in current tag
    size_t mMarkTagRemaining;                    // Mark() copy of mTagRemaining

    union
    {
        struct
        {
            uint8_t const * buf;
            uint8_t const * end;
            uint8_t const * mark;
        } root;
        struct
        {
            TLVReader * parent;
        } child;
    } mSource = { .root = { nullptr, nullptr } };

    enum ErrorState : uint8_t
    {
        StateNoError   = 0,
        StateIncorrect = 0xff, // CHIP_ERROR_INCORRECT_STATE
        StateUnderrun  = 0xfe, // CHIP_ERROR_TLV_UNDERRUN
    };

    void Init(TLVReader & parent);
    void Reset(enum ErrorState state);
    enum ErrorState ErrorState();

    void MarkSource();
    void Mark();
    void Rewind();

    size_t Remaining();
    bool Read(uint8_t * buf, size_t count);
    bool ReadTagData(uint8_t * buf, size_t count);

    using decode_ptr = CHIP_ERROR (*)(void *, TLVReader &);
    CHIP_ERROR DecodeTLV(void * tlv, decode_ptr decodeFn);
    static CHIP_ERROR DecodeTLV(void * tlv, decode_ptr decodeFn, uint8_t const * data, size_t dataLen);

    template <typename T>
    static constexpr decode_ptr DecodeFn(T const & tlv)
    {
        return [](void * tlv, TLVReader & reader) { return static_cast<T *>(tlv)->Decode(reader); };
    }

    CHIP_ERROR GetBytes(uint8_t * buf, size_t bufSize, bool terminate);
    CHIP_ERROR DupBytes(uint8_t *& data, uint32_t * dataLen, bool terminate);

    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
};

} // namespace TLV8
} // namespace Apple
