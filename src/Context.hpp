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

#pragma once

#include "pcsc-cpp/pcsc-cpp.hpp"
#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include "SCardCall.hpp"

#include "pcsc-cpp/comp_winscard.hpp"

namespace pcsc_cpp
{

class Context
{
public:
    Context()
    {
        SCard(EstablishContext, DWORD(SCARD_SCOPE_USER), nullptr, nullptr, &contextHandle);
        if (!contextHandle) {
            THROW(ScardError,
                  "Context:SCardEstablishContext: service unavailable "
                  "(null context handle)");
        }
    }

    ~Context()
    {
        if (contextHandle) {
            // Cannot throw in destructor, so cannot use the SCard() macro here.
            auto result = SCardReleaseContext(contextHandle);
            contextHandle = 0;
            (void)result; // TODO: Log result here in case it is not OK.
        }
    }

    PCSC_CPP_DISABLE_COPY_MOVE(Context);

    SCARDCONTEXT handle() const { return contextHandle; }

private:
    SCARDCONTEXT contextHandle = 0;
};

} // namespace pcsc_cpp
