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

#pragma once

#include <cstdint>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#undef IGNORE
#endif

#ifdef __APPLE__
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

#ifndef SCARD_CTL_CODE
#define SCARD_CTL_CODE(code) (0x42000000 + (code))
#endif

// http://pcscworkgroup.com/Download/Specifications/pcsc10_v2.02.09.pdf
// http://ludovic.rousseau.free.fr/softwares/pcsc-lite/SecurePIN%20discussion%20v5.pdf
#define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)

enum DRIVER_FEATURES : uint8_t {
    FEATURE_VERIFY_PIN_START = 0x01,
    FEATURE_VERIFY_PIN_FINISH = 0x02,
    FEATURE_MODIFY_PIN_START = 0x03,
    FEATURE_MODIFY_PIN_FINISH = 0x04,
    FEATURE_GET_KEY_PRESSED = 0x05,
    FEATURE_VERIFY_PIN_DIRECT = 0x06,
    FEATURE_MODIFY_PIN_DIRECT = 0x07,
    FEATURE_MCT_READER_DIRECT = 0x08,
    FEATURE_MCT_UNIVERSAL = 0x09,
    FEATURE_IFD_PIN_PROPERTIES = 0x0A,
    FEATURE_ABORT = 0x0B,
    FEATURE_SET_SPE_MESSAGE = 0x0C,
    FEATURE_VERIFY_PIN_DIRECT_APP_ID = 0x0D,
    FEATURE_MODIFY_PIN_DIRECT_APP_ID = 0x0E,
    FEATURE_WRITE_DISPLAY = 0x0F,
    FEATURE_GET_KEY = 0x10,
    FEATURE_IFD_DISPLAY_PROPERTIES = 0x11,
    FEATURE_GET_TLV_PROPERTIES = 0x12,
    FEATURE_CCID_ESC_COMMAND = 0x13
};

using PCSC_TLV_STRUCTURE = struct
{
    uint8_t tag;
    uint8_t length;
    uint32_t value;
};

enum bmFormatString : uint8_t {
    FormatBinary = 0 << 0, // (1234 => 01h 02h 03h 04h)
    FormatBCD = 1 << 0, // (1234 => 12h 34h)
    FormatASCII = 1 << 1, // (1234 => 31h 32h 33h 34h)
    AlignLeft = 0 << 2,
    AlignRight = 1 << 2,
    PINFrameOffsetUnitBits = 0 << 7,
    PINFrameOffsetUnitBytes = 1 << 7,
};

enum bmPINBlockString : uint8_t {
    PINLengthNone = 0 << 4,
    PINFrameSizeAuto = 0,
};

enum bmPINLengthFormat : uint8_t {
    PINLengthOffsetUnitBits = 0 << 4,
    PINLengthOffsetUnitBytes = 1 << 4,
};

enum bEntryValidationCondition : uint8_t {
    ValidOnMaxSizeReached = 1 << 0,
    ValidOnKeyPressed = 1 << 1,
    ValidOnTimeout = 1 << 2,
};

enum bNumberMessage : uint8_t {
    NoInvitationMessage = 0,
    OneInvitationMessage = 1,
    TwoInvitationMessage = 2, // MODIFY
    ThreeInvitationMessage = 3, // MODIFY
    CCIDDefaultInvitationMessage = 0xFF,
};

enum bConfirmPIN : uint8_t {
    ConfirmNewPin = 1 << 0,
    RequestCurrentPin = 1 << 1,
    AdvancedModify = 1 << 2,
};

using PIN_VERIFY_STRUCTURE = struct
{
    uint8_t bTimerOut; // timeout in seconds (00 means use default timeout)
    uint8_t bTimerOut2; // timeout in seconds after first key stroke
    uint8_t bmFormatString; // formatting options
    uint8_t bmPINBlockString; // PIN block definition
    uint8_t bmPINLengthFormat; // PIN length definition
    uint16_t wPINMaxExtraDigit; // 0xXXYY where XX is minimum PIN size in digits, and YY is maximum
                                // PIN size in digits
    uint8_t
        bEntryValidationCondition; // Conditions under which PIN entry should be considered complete
    uint8_t bNumberMessage; // Number of messages to display for PIN verification
    uint16_t wLangId; // Language for messages (http://www.usb.org/developers/docs/USB_LANGIDs.pdf)
    uint8_t bMsgIndex; // Message index (should be 00)
    uint8_t bTeoPrologue[3]; // T=1 I-block prologue field to use (fill with 00)
    uint32_t ulDataLength; // length of Data to be sent to the ICC
};

#ifdef __APPLE__
#pragma pack()
#else
#pragma pack(pop)
#endif
