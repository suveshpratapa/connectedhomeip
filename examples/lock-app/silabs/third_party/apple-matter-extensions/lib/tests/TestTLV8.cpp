/*
 *    Copyright (c) 2023 Apple Inc. All rights reserved.
 *
 *    Apple Matter Extensions is licensed under Apple Inc.'s MFi Sample Code
 *    License Agreement, which is contained in the LICENSE file distributed with
 *    the Apple Matter Extensions, and only to those who accept that license.
 */

#include <apple/support/TLV8.h>
#include <lib/support/UnitTestContext.h>
#include <lib/support/UnitTestRegistration.h>
#include <nlunit-test.h>

using namespace Apple;
using namespace Apple::TLV8;

void CheckReadEmpty(nlTestSuite * inSuite, void * inContext)
{
    TLVReader reader;
    reader.Init({});
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV); // stays that way
}

void CheckReadBlobs(nlTestSuite * inSuite, void * inContext)
{
    uint8_t byteBuf[10];
    char charBuf[10];
    TLVReader reader;
    reader.Init({ 1, 0, 2, 5, 'h', 'e', 'l', 'l', 'o', 3, 8, 3, 2, 1, 0, 1, 2, 3, 4 });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 1);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 0);
    memset(byteBuf, 0x55, sizeof(byteBuf));
    NL_TEST_ASSERT(inSuite, reader.GetBytes(byteBuf, 0) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, byteBuf[0] == 0x55); // not modified
    NL_TEST_ASSERT(inSuite, reader.GetBytes(byteBuf, sizeof(byteBuf)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, byteBuf[0] == 0x55); // not modified, length == 0
    memset(charBuf, '?', sizeof(charBuf));
    NL_TEST_ASSERT(inSuite, reader.GetString(charBuf, 0) == CHIP_ERROR_BUFFER_TOO_SMALL);
    NL_TEST_ASSERT(inSuite, charBuf[0] == '?'); // not modified
    NL_TEST_ASSERT(inSuite, reader.GetString(charBuf, 1) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, charBuf[0] == 0);
    NL_TEST_ASSERT(inSuite, charBuf[1] == '?');

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 2);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 5);
    memset(byteBuf, '$', sizeof(byteBuf));
    NL_TEST_ASSERT(inSuite, reader.GetBytes(byteBuf, 4) == CHIP_ERROR_BUFFER_TOO_SMALL);
    NL_TEST_ASSERT(inSuite, byteBuf[0] == '$'); // not modified
    NL_TEST_ASSERT(inSuite, reader.GetBytes(byteBuf, sizeof(byteBuf)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, !memcmp(byteBuf, "hello$", 6));
    memset(charBuf, '#', sizeof(charBuf));
    NL_TEST_ASSERT(inSuite, reader.GetString(charBuf, 5) == CHIP_ERROR_BUFFER_TOO_SMALL);
    NL_TEST_ASSERT(inSuite, charBuf[0] == '#'); // not modified
    NL_TEST_ASSERT(inSuite, reader.GetString(charBuf, sizeof(charBuf)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, !memcmp(charBuf, "hello\0#", 7));

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 3);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 8);
    memset(byteBuf, '*', sizeof(byteBuf));
    NL_TEST_ASSERT(inSuite, reader.GetBytes(byteBuf, sizeof(byteBuf)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, !memcmp(byteBuf, "\3\2\1\0\1\2\3\4*", 9));
    memset(charBuf, '+', sizeof(charBuf));
    NL_TEST_ASSERT(inSuite, reader.GetString(charBuf, sizeof(charBuf)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, !memcmp(charBuf, "\3\2\1\0\1\2\3\4\0+", 10));

    uint8_t * bytePtr = nullptr;
    uint32_t byteLen  = 555;
    NL_TEST_ASSERT(inSuite, reader.DupBytes(bytePtr, byteLen) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, bytePtr != nullptr);
    NL_TEST_ASSERT(inSuite, byteLen == 8);
    NL_TEST_ASSERT(inSuite, !memcmp(bytePtr, "\3\2\1\0\1\2\3\4", 8));
    chip::Platform::MemoryFree(bytePtr);

    char * charPtr = nullptr;
    NL_TEST_ASSERT(inSuite, reader.DupString(charPtr) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, charPtr != nullptr);
    NL_TEST_ASSERT(inSuite, !memcmp(charPtr, "\3\2\1\0\1\2\3\4\0", 9));
    chip::Platform::MemoryFree(charPtr);
}

void CheckReadIntegers(nlTestSuite * inSuite, void * inContext)
{
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    TLVReader reader;
    reader.Init(
        { 10, 1, 0xab, 11, 2, 0xcd, 0xab, 12, 4, 0x12, 0xef, 0xcd, 0xab, 13, 8, 0x90, 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 10);
    NL_TEST_ASSERT(inSuite, reader.Get(u8) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u8 == 0xab);
    NL_TEST_ASSERT(inSuite, reader.Get(i8) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i8 == -85);
    NL_TEST_ASSERT(inSuite, reader.Get(u16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u16 == 0xab);
    NL_TEST_ASSERT(inSuite, reader.Get(i16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i16 == -85);
    NL_TEST_ASSERT(inSuite, reader.Get(u32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u32 == 0xab);
    NL_TEST_ASSERT(inSuite, reader.Get(i32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i32 == -85);
    NL_TEST_ASSERT(inSuite, reader.Get(u64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u64 == 0xab);
    NL_TEST_ASSERT(inSuite, reader.Get(i64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i64 == -85);

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 11);
    NL_TEST_ASSERT(inSuite, reader.Get(u8) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(i8) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u16 == 0xabcd);
    NL_TEST_ASSERT(inSuite, reader.Get(i16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i16 == -21555);
    NL_TEST_ASSERT(inSuite, reader.Get(u32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u32 == 0xabcd);
    NL_TEST_ASSERT(inSuite, reader.Get(i32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i32 == -21555);
    NL_TEST_ASSERT(inSuite, reader.Get(u64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u64 == 0xabcd);
    NL_TEST_ASSERT(inSuite, reader.Get(i64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i64 == -21555);

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 12);
    NL_TEST_ASSERT(inSuite, reader.Get(u8) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u16) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u32 == 0xabcdef12);
    NL_TEST_ASSERT(inSuite, reader.Get(i32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i32 == -1412567278);
    NL_TEST_ASSERT(inSuite, reader.Get(u64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u64 == 0xabcdef12);
    NL_TEST_ASSERT(inSuite, reader.Get(i64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i64 == -1412567278);

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 13);
    NL_TEST_ASSERT(inSuite, reader.Get(u8) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u16) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u32) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(u64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u64 == 0xabcdef1234567890);
    NL_TEST_ASSERT(inSuite, reader.Get(i64) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, i64 == -6066930261531658096);

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV);
}

void CheckReadBools(nlTestSuite * inSuite, void * inContext)
{
    bool b;
    TLVReader reader;
    reader.Init({ 0xff, 1, 0, 0xfe, 1, 1, 0xfc, 1, 0xaa });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 0xff);
    NL_TEST_ASSERT(inSuite, reader.Get(b) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, b == false);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 0xfe);
    NL_TEST_ASSERT(inSuite, reader.Get(b) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, b == true);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 0xfc);
    NL_TEST_ASSERT(inSuite, reader.Get(b) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, b == true);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV);
}

void CheckReadFloats(nlTestSuite * inSuite, void * inContext)
{
    float f;
    double d;
    TLVReader reader;
    reader.Init({ 1, 4, 0x00, 0x00, 0x88, 0x3e, 2, 8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xd0, 0x3f });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 1);
    NL_TEST_ASSERT(inSuite, reader.Get(f) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, f == 0.265625f);
    NL_TEST_ASSERT(inSuite, reader.Get(d) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 2);
    NL_TEST_ASSERT(inSuite, reader.Get(f) == CHIP_ERROR_WRONG_TLV_TYPE);
    NL_TEST_ASSERT(inSuite, reader.Get(d) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, d == 0.2578125);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV);
}

void CheckReadContinuations(nlTestSuite * inSuite, void * inContext)
{
    uint32_t u32;
    TLVReader reader;
    reader.Init({ 1, 4, 90, 91, 92, 93, 1, 2, 95, 96, 1, 0, 2, 1, 0x44, 2, 2, 0x33, 0x22, 2, 1, 0x11 });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 1);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 6);
    uint8_t expected[] = { 90, 91, 92, 93, 95, 96 };
    uint8_t buffer[sizeof(expected)];
    NL_TEST_ASSERT(inSuite, reader.GetBytes(buffer, sizeof(buffer)) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, !memcmp(buffer, expected, sizeof(expected)));

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 1);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 0); // length 0 -> not coalesced

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.GetTag() == 2);
    NL_TEST_ASSERT(inSuite, reader.GetLength() == 4);
    NL_TEST_ASSERT(inSuite, reader.Get(u32) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u32 = 0x11223344);

    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_END_OF_TLV);
}

void CheckReadNested(nlTestSuite * inSuite, void * inContext)
{
    uint16_t u16;
    TLVReader outer, middle, inner;
    outer.Init({ 10, 4, 11, 1, 12, 11, 10, 5, 2, 2, 0x37, 11, 2, 10, 9, 0x13, 13, 11, 2, 2, 0x0d, 11, 1, 0xd0, 14, 0 });
    //           TT  L  ~~~~~~~~~~~~~  TT  L  ~~~~~~~~~~~~~~~~~  TT  L  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  TT  L
    //           10 (4+5+9 == 18)
    // middle:          11, 1, 12, 11,        2, 2, 0x37, 11, 2,        0x13, 13, 11, 2, 2, 0x0d, 11, 1, 0xd0
    //                  TT  L  ~~  TT         L  ~~~~~~~  TT  L         ~~~~~~~~  TT  L  ~~~~~~~  TT  L  ~~~~
    //                  11 (1+2+2+2+1==8)
    // inner:                  12,               2, 0x37,               0x13, 13,        2, 0x0d,        0xd0
    //                         TT                L  ~~~~                ~~~~  TT         L  ~~~~         ~~~~

    NL_TEST_ASSERT(inSuite, outer.OpenContainer(middle) == CHIP_ERROR_INCORRECT_STATE);

    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.GetTag() == 10);
    NL_TEST_ASSERT(inSuite, outer.GetLength() == 18);

    OwnedByteSpan data;
    NL_TEST_ASSERT(inSuite, outer.GetBytes(data) == CHIP_NO_ERROR); // works before OpenContainer()

    NL_TEST_ASSERT(inSuite, outer.OpenContainer(middle) == CHIP_NO_ERROR);

    NL_TEST_ASSERT(inSuite, middle.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, middle.GetTag() == 11);
    NL_TEST_ASSERT(inSuite, middle.GetLength() == 8);
    NL_TEST_ASSERT(inSuite, middle.OpenContainer(inner) == CHIP_NO_ERROR);

    NL_TEST_ASSERT(inSuite, inner.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, inner.GetTag() == 12);
    NL_TEST_ASSERT(inSuite, inner.Get(u16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u16 == 0x1337);
    NL_TEST_ASSERT(inSuite, inner.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, inner.GetTag() == 13);
    NL_TEST_ASSERT(inSuite, inner.Get(u16) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, u16 == 0xd00d);
    NL_TEST_ASSERT(inSuite, inner.Next() == CHIP_END_OF_TLV);

    NL_TEST_ASSERT(inSuite, middle.CloseContainer(inner) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, middle.Next() == CHIP_END_OF_TLV);

    NL_TEST_ASSERT(inSuite, outer.CloseContainer(middle) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.GetBytes(data) == CHIP_ERROR_INCORRECT_STATE);        // can't read again
    NL_TEST_ASSERT(inSuite, outer.OpenContainer(middle) == CHIP_ERROR_INCORRECT_STATE); // can't open again

    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.GetTag() == 14);
    NL_TEST_ASSERT(inSuite, outer.GetLength() == 0);
    NL_TEST_ASSERT(inSuite, outer.OpenContainer(middle) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.CloseContainer(middle) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.GetBytes(data) == CHIP_ERROR_INCORRECT_STATE);        // can't read again
    NL_TEST_ASSERT(inSuite, outer.OpenContainer(middle) == CHIP_ERROR_INCORRECT_STATE); // can't open again

    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_END_OF_TLV);
}

void CheckUnderunInData(nlTestSuite * inSuite, void * inContext)
{
    TLVReader reader;
    reader.Init({ 10, 1, 0xdd, 11, 5, 1, 2, 3, 4 });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_ERROR_TLV_UNDERRUN);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_ERROR_TLV_UNDERRUN); // stays that way
}

void CheckUnderunInTag(nlTestSuite * inSuite, void * inContext)
{
    TLVReader reader;
    reader.Init({ 10, 1, 0xdd, 11 });
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_ERROR_TLV_UNDERRUN);
    NL_TEST_ASSERT(inSuite, reader.Next() == CHIP_ERROR_TLV_UNDERRUN); // stays that way
}

void CheckUnderunInNestedReaderDuringClose(nlTestSuite * inSuite, void * inContext)
{
    TLVReader outer, inner;
    outer.Init({ 1, 3, 2, 0, 0xff /* inner underrun */, 3, 0 });
    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, outer.GetTag() == 1);

    NL_TEST_ASSERT(inSuite, outer.OpenContainer(inner) == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, inner.Next() == CHIP_NO_ERROR);
    NL_TEST_ASSERT(inSuite, inner.GetTag() == 2);
    NL_TEST_ASSERT(inSuite, outer.CloseContainer(inner) == CHIP_ERROR_TLV_UNDERRUN);

    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_NO_ERROR); // outer can continue
    NL_TEST_ASSERT(inSuite, outer.GetTag() == 3);
    NL_TEST_ASSERT(inSuite, outer.Next() == CHIP_END_OF_TLV);
}

// clang-format off
static const nlTest sTests[] =
{
    NL_TEST_DEF("Read empty input", CheckReadEmpty),
    NL_TEST_DEF("Read blob values", CheckReadBlobs),
    NL_TEST_DEF("Read integer values", CheckReadIntegers),
    NL_TEST_DEF("Read bool values", CheckReadBools),
    NL_TEST_DEF("Read float values", CheckReadFloats),
    NL_TEST_DEF("Read continuations", CheckReadContinuations),
    NL_TEST_DEF("Read nested tlv8", CheckReadNested),
    NL_TEST_DEF("Reader underrun in data", CheckUnderunInData),
    NL_TEST_DEF("Reader underrun in tag", CheckUnderunInTag),
    NL_TEST_DEF("Reader underrun in nested container", CheckUnderunInNestedReaderDuringClose),
    NL_TEST_SENTINEL()
};
// clang-format on

int TestTLV8()
{
    chip::Platform::MemoryInit();
    nlTestSuite theSuite = { "apple-tlv8", sTests };
    return chip::ExecuteTestsWithoutContext(&theSuite);
}

CHIP_REGISTER_TEST_SUITE(TestTLV8)
