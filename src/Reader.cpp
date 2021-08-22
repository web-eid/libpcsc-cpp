/*
 * Copyright (c) 2020-2021 Estonian Information System Authority
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

#include "magic_enum/magic_enum.hpp"

#include <numeric>

namespace pcsc_cpp
{

Reader::Reader(ContextPtr c, string_t n, byte_vector atr, flag_set<Status> s) :
    name(std::move(n)), cardAtr(std::move(atr)), status(s), ctx(std::move(c))
{
}

std::string Reader::statusString() const
{
    std::vector<std::string> result;

    for (auto statusValue = int(Reader::Status::UNAWARE); statusValue < int(Reader::Status::_);
         ++statusValue) {
        if (status[Reader::Status(statusValue)]) {
            result.emplace_back(magic_enum::enum_name(Reader::Status(statusValue)));
        }
    }

    return std::accumulate(std::begin(result), std::end(result), std::string(),
                           [](const std::string& resultStr, const std::string& item) {
                               return resultStr.empty() ? item : resultStr + "," + item;
                           });
}

} // namespace pcsc_cpp
