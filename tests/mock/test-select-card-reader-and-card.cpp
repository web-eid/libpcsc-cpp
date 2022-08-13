/*
 * Copyright (c) 2020-2022 Estonian Information System Authority
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

TEST(pcsc_cpp_test, listReadersSuccess)
{
    using namespace pcsc_cpp;

    auto readers = listReaders();
    EXPECT_EQ(readers.size(), 1U);
#ifdef _WIN32
    EXPECT_EQ(readers[0].name, L"PcscMock-reader");
#else
    EXPECT_EQ(readers[0].name, "PcscMock-reader");
#endif
    EXPECT_EQ(readers[0].statusString(), "PRESENT");
}

TEST(pcsc_cpp_test, listReadersNoReaders)
{
    using namespace pcsc_cpp;

    PcscMock::addReturnValueForScardFunctionCall("SCardListReaders", SCARD_E_NO_READERS_AVAILABLE);

    auto readers = listReaders();
    EXPECT_EQ(readers.size(), 0U);

    PcscMock::reset();
}

TEST(pcsc_cpp_test, listReadersNoService)
{
    using namespace pcsc_cpp;

    PcscMock::addReturnValueForScardFunctionCall("SCardEstablishContext", SCARD_E_NO_SERVICE);

    EXPECT_THROW({ listReaders(); }, ScardServiceNotRunningError);

    PcscMock::reset();
}
