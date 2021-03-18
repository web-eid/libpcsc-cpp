/*
 * Copyright (c) 2020 The Web eID Project
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

#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace pcsc_cpp
{

/** Convert the given integer to a hex string. */
template <typename T>
inline std::string int2hexstr(const T value)
{
    std::ostringstream hexStringBuilder;

    hexStringBuilder << "0x" << std::setfill('0') << std::setw(sizeof(long) * 2) << std::hex
                     << value;

    return hexStringBuilder.str();
}

/** Remove absolute path prefix until 'src' from the given path, '/path/to/src/main.cpp' becomes
 * 'src/main.cpp'. */
inline std::string removeAbsolutePathPrefix(const std::string& filePath)
{
    const auto lastSrc = filePath.rfind("src");
    return lastSrc == std::string::npos ? filePath : filePath.substr(lastSrc);
}

} // namespace pcsc_cpp

#define THROW_WITH_CALLER_INFO(ExceptionType, message, file, line, func)                           \
    throw ExceptionType(std::string(message) + " in " + pcsc_cpp::removeAbsolutePathPrefix(file)   \
                        + ':' + std::to_string(line) + ':' + func)

#define THROW(ExceptionType, message)                                                              \
    THROW_WITH_CALLER_INFO(ExceptionType, message, __FILE__, __LINE__, __func__)
