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

#include "pcsc-cpp/pcsc-cpp.hpp"
#include "pcsc-cpp/comp_winscard.hpp"
#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include <string>

namespace pcsc_cpp
{

inline std::string buildErrorMessage(const char* callerFunctionName, const char* scardFunctionName,
                                     const LONG result, const char* file, int line)
{
    return std::string(scardFunctionName) + " returned " + int2hexstr(result) + " in "
        + removeAbsolutePathPrefix(file) + ':' + std::to_string(line) + ':' + callerFunctionName;
}

template <typename Func, typename... Args>
void SCardCall(const char* callerFunctionName, const char* file, int line,
               const char* scardFunctionName, Func scardFunction, Args... args)
{
    // TODO: Add logging - or is exception error message enough?
    const uint32_t result = scardFunction(args...);

    // TODO: Add more cases when needed.
    switch (result) {
    case SCARD_S_SUCCESS:
        return;
    case SCARD_E_NO_SERVICE:
    case SCARD_E_SERVICE_STOPPED:
        throw ScardServiceNotRunningError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    case SCARD_E_NO_READERS_AVAILABLE:
        throw ScardNoReadersError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    case SCARD_E_NO_SMARTCARD:
        throw ScardNoCardError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    case SCARD_W_REMOVED_CARD:
        throw ScardCardRemovedError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    case SCARD_E_NOT_TRANSACTED:
        throw ScardTransactionFailedError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    default:
        throw ScardError(
            buildErrorMessage(callerFunctionName, scardFunctionName, result, file, line));
    }
}

} // namespace pcsc_cpp

#define SCard(APIFunctionName, ...)                                                                \
    SCardCall(__FUNCTION__, __FILE__, __LINE__, "SCard" #APIFunctionName, SCard##APIFunctionName,  \
              __VA_ARGS__)
