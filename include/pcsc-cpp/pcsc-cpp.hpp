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

#include "flag-set-cpp/flag_set.hpp"

#include <memory>
#include <vector>
#include <limits>

namespace pcsc_cpp
{

using byte_vector = std::vector<unsigned char>;
#ifdef _WIN32
using string_t = std::wstring;
#else
using string_t = std::string;
#endif

/** Opaque class that wraps the PC/SC resource manager context. */
class Context;
using ContextPtr = std::shared_ptr<Context>;

/** Returns the value of the response status bytes SW1 and SW2 as a single status word SW. */
inline constexpr uint16_t toSW(byte_vector::value_type sw1, byte_vector::value_type sw2)
{
    return sw1 << 8 | sw2;
}

/** Struct that wraps response APDUs. */
struct ResponseApdu
{
    enum Status {
        OK = 0x90,
        MORE_DATA_AVAILABLE = 0x61,
        VERIFICATION_FAILED = 0x63,
        VERIFICATION_CANCELLED = 0x64,
        WRONG_LENGTH = 0x67,
        COMMAND_NOT_ALLOWED = 0x69,
        WRONG_PARAMETERS = 0x6a,
        WRONG_LE_LENGTH = 0x6c
    };

    byte_vector::value_type sw1 {};
    byte_vector::value_type sw2 {};

    byte_vector data;

    static const size_t MAX_DATA_SIZE = 256;
    static const size_t MAX_SIZE = MAX_DATA_SIZE + 2; // + sw1 and sw2

    ResponseApdu(byte_vector::value_type s1, byte_vector::value_type s2, byte_vector d = {}) :
        sw1(s1), sw2(s2), data(std::move(d))
    {
    }

    ResponseApdu() = default;

    static ResponseApdu fromBytes(const byte_vector& data)
    {
        if (data.size() < 2) {
            throw std::invalid_argument("Need at least 2 bytes for creating ResponseApdu");
        }

        // SW1 and SW2 are in the end
        return ResponseApdu {data[data.size() - 2], data[data.size() - 1],
                             byte_vector {data.cbegin(), data.cend() - 2}};
    }

    byte_vector toBytes() const
    {
        // makes a copy, valid both if data is empty or full
        auto bytes = data;

        bytes.push_back(sw1);
        bytes.push_back(sw2);

        return bytes;
    }

    uint16_t toSW() const { return pcsc_cpp::toSW(sw1, sw2); }

    bool isOK() const { return sw1 == OK && sw2 == 0x00; }

    // TODO: friend function toString() in utilities.hpp
};

/** Struct that wraps command APDUs. */
struct CommandApdu
{
    unsigned char cla;
    unsigned char ins;
    unsigned char p1;
    unsigned char p2;
    unsigned short le;
    // Lc is data.size()
    byte_vector data;

    static const size_t MAX_DATA_SIZE = 255;
    static const unsigned short LE_UNUSED = std::numeric_limits<unsigned short>::max();

    CommandApdu(unsigned char c, unsigned char i, unsigned char pp1, unsigned char pp2,
                byte_vector d = {}, unsigned short l = LE_UNUSED) :
        cla(c),
        ins(i), p1(pp1), p2(pp2), le(l), data(std::move(d))
    {
    }

    CommandApdu(const CommandApdu& other, byte_vector d) :
        cla(other.cla), ins(other.ins), p1(other.p1), p2(other.p2), le(other.le), data(std::move(d))
    {
    }

    bool isLeSet() const { return le != LE_UNUSED; }

    static CommandApdu fromBytes(const byte_vector& bytes, bool useLe = false)
    {
        if (bytes.size() < 4) {
            throw std::invalid_argument("Command APDU must have > 3 bytes");
        }

        if (bytes.size() == 4) {
            return CommandApdu {bytes[0], bytes[1], bytes[2], bytes[3]};
        }

        if (bytes.size() == 5) {
            if (useLe) {
                return CommandApdu {bytes[0], bytes[1],      bytes[2],
                                    bytes[3], byte_vector(), bytes[4]};
            }
            throw std::invalid_argument("Command APDU size 5 is invalid without LE");
        }

        if (bytes.size() == 6 && useLe) {
            throw std::invalid_argument("Command APDU size 6 uses LE");
        }

        // 0 - cla, 1 - ins, 2 - p1, 3 - p2, 4 - data size
        // FIXME: can command chaining use byte 5 for data size too?
        auto dataStart = bytes.cbegin() + 5;

        if (useLe) {
            return CommandApdu {bytes[0],
                                bytes[1],
                                bytes[2],
                                bytes[3],
                                byte_vector(dataStart, bytes.cend() - 1),
                                *(bytes.cend() - 1)};
        }
        return CommandApdu {bytes[0], bytes[1], bytes[2], bytes[3],
                            byte_vector(dataStart, bytes.cend())};
    }

    byte_vector toBytes() const
    {
        if (data.size() > MAX_DATA_SIZE) {
            throw std::invalid_argument("Command chaining not supported");
        }

        auto bytes = byte_vector {cla, ins, p1, p2};

        if (!data.empty()) {
            bytes.push_back(static_cast<unsigned char>(data.size()));
            bytes.insert(bytes.end(), data.cbegin(), data.cend());
        }

        if (isLeSet()) {
            // TODO: EstEID spec: the maximum value of Le is 0xFE
            if (le > ResponseApdu::MAX_DATA_SIZE)
                throw std::invalid_argument("LE larger than response size");
            bytes.push_back(static_cast<unsigned char>(le));
        }

        return bytes;
    }
};

/** Opaque class that wraps the PC/SC smart card resources like card handle and I/O protocol. */
class CardImpl;
using CardImplPtr = std::unique_ptr<CardImpl>;

/** PIN pad PIN entry timer timeout */
constexpr uint8_t PIN_PAD_PIN_ENTRY_TIMEOUT = 90; // 1 minute, 30 seconds

/** SmartCard manages bidirectional input/output to an ISO 7816 smart card. */
class SmartCard
{
public:
    enum class Protocol { UNDEFINED, T0, T1 }; // AUTO = T0 | T1

    using ptr = std::unique_ptr<SmartCard>;

    class TransactionGuard
    {
    public:
        TransactionGuard(const CardImpl& CardImpl, bool& inProgress);
        ~TransactionGuard();

        // The rule of five (C++ Core guidelines C.21).
        TransactionGuard(const TransactionGuard&) = delete;
        TransactionGuard& operator=(const TransactionGuard&) = delete;
        TransactionGuard(TransactionGuard&&) = delete;
        TransactionGuard& operator=(TransactionGuard&&) = delete;

    private:
        const CardImpl& card;
        bool& inProgress;
    };

    SmartCard(const ContextPtr& context, const string_t& readerName, byte_vector atr);
    SmartCard(); // Null object constructor.
    ~SmartCard();

    // The rule of five.
    SmartCard(const SmartCard&) = delete;
    SmartCard& operator=(const SmartCard&) = delete;
    SmartCard(SmartCard&&) = delete;
    SmartCard& operator=(SmartCard&&) = delete;

    TransactionGuard beginTransaction();
    ResponseApdu transmit(const CommandApdu& command) const;
    ResponseApdu transmitCTL(const CommandApdu& command, uint16_t lang, uint8_t minlen) const;
    bool readerHasPinPad() const;

    Protocol protocol() const { return _protocol; }
    const byte_vector& atr() const { return _atr; }

private:
    CardImplPtr card;
    Protocol _protocol;
    byte_vector _atr;
    bool transactionInProgress = false;
};

/** Reader provides card reader information, status and gives access to the smart card in it. */
class Reader
{
public:
    enum class Status {
        UNAWARE,
        IGNORE,
        CHANGED,
        UNKNOWN,
        UNAVAILABLE,
        EMPTY,
        PRESENT,
        ATRMATCH,
        EXCLUSIVE,
        INUSE,
        MUTE,
        UNPOWERED,
        _
    };

    Reader(ContextPtr context, string_t name, byte_vector cardAtr, flag_set<Status> status);

    SmartCard::ptr connectToCard() const { return std::make_unique<SmartCard>(ctx, name, cardAtr); }

    bool isCardInserted() const { return status[Status::PRESENT]; }

    std::string statusString() const;

    const string_t name;
    const byte_vector cardAtr;
    const flag_set<Status> status;

private:
    ContextPtr ctx;
};

/**
 * Access system smart card readers, entry point to the library.
 *
 * @throw ScardError, SystemError
 */
std::vector<Reader> listReaders();

// Utility functions.

extern const byte_vector APDU_RESPONSE_OK;

/** Convert bytes to hex string. */
std::string bytes2hexstr(const byte_vector& bytes);

/** Transmit APDU command and verify that expected response is received. */
void transmitApduWithExpectedResponse(const SmartCard& card, const CommandApdu& command,
                                      const byte_vector& expectedResponseBytes = APDU_RESPONSE_OK);
void transmitApduWithExpectedResponse(const SmartCard& card, const byte_vector& commandBytes,
                                      const byte_vector& expectedResponseBytes = APDU_RESPONSE_OK);

/** Read data length from currently selected file header, file must be ASN.1-encoded. */
size_t readDataLengthFromAsn1(const SmartCard& card);

/** Read lenght bytes from currently selected binary file in blockLength-sized chunks. */
byte_vector readBinary(const SmartCard& card, const size_t length, const size_t blockLength);

// Errors.

/** Base class for all pcsc-cpp errors. */
class Error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/** Programming or system errors. */
class SystemError : public Error
{
public:
    using Error::Error;
};

/** Base class for all SCard API errors. */
class ScardError : public Error
{
public:
    using Error::Error;
};

/** Thrown when the PC/SC service is not running. */
class ScardServiceNotRunningError : public ScardError
{
public:
    using ScardError::ScardError;
};

/** Thrown when no card readers are connected to the system. */
class ScardNoReadersError : public ScardError
{
public:
    using ScardError::ScardError;
};

/** Thrown when no card is connected to the selected reader. */
class ScardNoCardError : public ScardError
{
public:
    using ScardError::ScardError;
};

/** Thrown when communication with the card or reader fails. */
class ScardCardCommunicationFailedError : public ScardError
{
public:
    using ScardError::ScardError;
};

/** Thrown when the card is removed from the selected reader. */
class ScardCardRemovedError : public ScardError
{
public:
    using ScardError::ScardError;
};

/** Thrown when the card transaction fails. */
class ScardTransactionFailedError : public ScardError
{
public:
    using ScardError::ScardError;
};

} // namespace pcsc_cpp
