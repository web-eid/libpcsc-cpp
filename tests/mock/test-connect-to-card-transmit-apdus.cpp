/*
 * Copyright (c) 2020-2023 Estonian Information System Authority
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pcsc-cpp/pcsc-cpp.hpp"

#include "pcsc-mock/pcsc-mock.hpp"
#include "pcsc-cpp/comp_winscard.hpp"

#include <gtest/gtest.h>

using namespace pcsc_cpp;

namespace
{

SmartCard::ptr connectToCard()
{
    auto readers = listReaders();
    EXPECT_EQ(readers.size(), 1U);

    return readers[0].connectToCard();
}

} // namespace

TEST(pcsc_cpp_test, connectToCardSuccess)
{
    auto card = connectToCard();

    EXPECT_EQ(card->atr(), PcscMock::DEFAULT_CARD_ATR);
    EXPECT_EQ(card->protocol(), SmartCard::Protocol::T1);
}

TEST(pcsc_cpp_test, transmitApduSuccess)
{
    auto card = connectToCard();

    auto command = CommandApdu::fromBytes(PcscMock::DEFAULT_COMMAND_APDU);
    auto expectedResponse = ResponseApdu::fromBytes(PcscMock::DEFAULT_RESPONSE_APDU);

    auto transactionGuard = card->beginTransaction();
    auto response = card->transmit(command);

    EXPECT_EQ(response.toBytes(), expectedResponse.toBytes());
}
