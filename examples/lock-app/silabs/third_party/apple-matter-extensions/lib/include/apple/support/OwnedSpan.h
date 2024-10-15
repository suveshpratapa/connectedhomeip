/*
 *    Copyright (c) 2021 Project CHIP Authors
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
 *    This file is based on Project CHIP Span.h and was modified by
 *    Apple Inc. Such modifications are licensed under Apple Inc.'s MFi Sample
 *    Code License Agreement, which is contained in the LICENSE file distributed
 *    with the Apple Matter Extensions, and only to those who accept that
 *    license. For the avoidance of doubt, this does not modify the License.
 *
 *    Copyright (c) 2023 Apple Inc. All Rights Reserved.
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/Span.h>

namespace Apple {

/**
 * An object that holds a fixed-size sequence of objects and owns the associated memory.
 *
 * Designed to interoperate with non-owning CHIP Spans (including ByteSpan, CharSpan).
 */
template <class T>
class OwnedSpan
{
public:
    using pointer = T *;

    constexpr OwnedSpan() : mDataBuf(nullptr), mDataLen(0) {}

    constexpr pointer data() const { return mDataBuf; }
    constexpr size_t size() const { return mDataLen; }
    constexpr bool empty() const { return size() == 0; }
    constexpr pointer begin() const { return data(); }
    constexpr pointer end() const { return data() + size(); }

    operator chip::Span<T>() const { return chip::Span<T>(data(), size()); }

    chip::Span<T> SubSpan(size_t offset, size_t length) const
    {
        VerifyOrDie(offset <= size());
        VerifyOrDie(offset + length <= size());
        return chip::Span<T>(data() + offset, length);
    }

    chip::Span<T> SubSpan(size_t offset) const
    {
        VerifyOrDie(offset <= size());
        return Span(data() + offset, size() - offset);
    }

    void Adopt(pointer data, size_t dataLen)
    {
        FreeData();
        mDataBuf = data;
        mDataLen = mCapacity = dataLen;
    }

    CHIP_ERROR Assign(pointer data, size_t dataLen)
    {
        if (!dataLen)
        {
            FreeData();
            mDataBuf = nullptr;
            mDataLen = 0;
            return CHIP_NO_ERROR;
        }
        if (!mDataBuf || dataLen > mCapacity || dataLen <= mCapacity / 2)
        {
            pointer newDataBuf = chip::Platform::MemoryAlloc(dataLen * sizeof(T));
            VerifyOrReturnError(newDataBuf, CHIP_ERROR_NO_MEMORY);
            FreeData();
            mDataBuf  = newDataBuf;
            mCapacity = dataLen;
        }
        memcpy(mDataBuf, data, dataLen * sizeof(T));
        mDataLen = dataLen;
        return CHIP_NO_ERROR;
    }

    template <class U, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    CHIP_ERROR Assign(const OwnedSpan<U> & span)
    {
        return Assign(span.data(), span.size());
    }

    template <class U, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    CHIP_ERROR Assign(const chip::Span<U> & span)
    {
        return Assign(span.data(), span.size());
    }

    template <class U, size_t N, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    CHIP_ERROR Assign(const chip::FixedSpan<U, N> & span)
    {
        return Assign(span.data(), span.size());
    }

    bool data_equal(const T * otherData, size_t otherSize)
    {
        return (size() == otherSize) && (empty() || (memcmp(data(), otherData, size() * sizeof(T)) == 0));
    }

    template <class U, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    bool data_equal(const OwnedSpan<U> & span)
    {
        return data_equal(span.data(), span.size());
    }

    template <class U, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    bool data_equal(const chip::Span<U> & span)
    {
        return data_equal(span.data(), span.size());
    }

    template <class U, size_t N, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value>>
    bool data_equal(const chip::FixedSpan<U, N> & span)
    {
        return data_equal(span.data(), span.size());
    }

    void reduce_size(size_t new_size)
    {
        VerifyOrDie(new_size <= size());
        mDataLen = new_size;
    }

    ~OwnedSpan() { FreeData(); }

private:
    pointer mDataBuf;
    size_t mDataLen;
    size_t mCapacity;

    void FreeData()
    {
        if (mDataBuf)
            chip::Platform::MemoryFree(const_cast<void *>(static_cast<const void *>(mDataBuf)));
    }
};

using OwnedByteSpan = OwnedSpan<const uint8_t>;
using OwnedCharSpan = OwnedSpan<const char>;

} // namespace Apple
