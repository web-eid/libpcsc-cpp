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

#include "Context.hpp"
#include "pcsc-cpp/comp_winscard.hpp"

#ifdef _WIN32
#include <Winsock2.h>
#elif !defined(__APPLE__)
#include <arpa/inet.h>
#endif

#include <array>
#include <map>
#include <utility>

// TODO: Someday, maybe SCARD_SHARE_SHARED vs SCARD_SHARE_EXCLUSIVE and SCARD_RESET_CARD on
// disconnect if SCARD_SHARE_EXCLUSIVE, SCARD_LEAVE_CARD otherwise.

namespace
{

using namespace pcsc_cpp;

inline SmartCard::Protocol convertToSmartCardProtocol(const DWORD protocol)
{
    switch (protocol) {
    case SCARD_PROTOCOL_UNDEFINED:
        return SmartCard::Protocol::UNDEFINED;
    case SCARD_PROTOCOL_T0:
        return SmartCard::Protocol::T0;
    case SCARD_PROTOCOL_T1:
        return SmartCard::Protocol::T1;
    default:
        THROW(Error, "Unsupported card protocol: " + std::to_string(protocol));
    }
}

std::pair<SCARDHANDLE, DWORD> connectToCard(const SCARDCONTEXT ctx, const string_t& readerName)
{
    const unsigned requestedProtocol =
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1; // Let PCSC auto-select protocol.
    DWORD protocolOut = SCARD_PROTOCOL_UNDEFINED;
    SCARDHANDLE cardHandle = 0;

    SCard(Connect, ctx, readerName.c_str(), DWORD(SCARD_SHARE_SHARED), requestedProtocol,
          &cardHandle, &protocolOut);

    return std::pair<SCARDHANDLE, DWORD> {cardHandle, protocolOut};
}

} // namespace

namespace pcsc_cpp
{

class CardImpl
{
public:
    explicit CardImpl(std::pair<SCARDHANDLE, DWORD> cardParams) :
        cardHandle(cardParams.first), _protocol({cardParams.second, sizeof(SCARD_IO_REQUEST)})
    {
        // TODO: debug("Protocol: " + to_string(protocol()))
        try {
            DWORD size = 0;
            std::array<BYTE, 256> feature {};
            SCard(Control, cardHandle, DWORD(CM_IOCTL_GET_FEATURE_REQUEST), nullptr, 0U,
                  feature.data(), DWORD(feature.size()), &size);
            for (auto p = feature.cbegin(); DWORD(std::distance(feature.cbegin(), p)) < size;) {
                unsigned int tag = *p++;
                unsigned int len = *p++;
                unsigned int value = 0;
                for (unsigned int i = 0; i < len; ++i)
                    value |= (unsigned int)(*p++ << 8) * i;
                features[DRIVER_FEATURES(tag)] = ntohl(value);
            }
        } catch (const ScardError&) {
            // Ignore driver errors during card feature requests.
            // TODO: debug(error)
        }
    }

    ~CardImpl()
    {
        if (cardHandle) {
            // Cannot throw in destructor, so cannot use the SCard() macro here.
            auto result = SCardDisconnect(cardHandle, SCARD_LEAVE_CARD);
            cardHandle = 0;
            (void)result; // TODO: Log result here in case it is not OK.
        }
    }

    PCSC_CPP_DISABLE_COPY_MOVE(CardImpl);

    bool readerHasPinPad() const
    {
        if (getenv("SMARTCARDPP_NOPINPAD"))
            return false;
        return features.find(FEATURE_VERIFY_PIN_START) != features.cend()
            || features.find(FEATURE_VERIFY_PIN_DIRECT) != features.cend();
    }

    ResponseApdu transmitBytes(const byte_vector& commandBytes) const
    {
        auto responseBytes = byte_vector(ResponseApdu::MAX_SIZE, 0);
        auto responseLength = DWORD(responseBytes.size());

        // TODO: debug("Sending:  " + bytes2hexstr(commandBytes))

        SCard(Transmit, cardHandle, &_protocol, commandBytes.data(), DWORD(commandBytes.size()),
              nullptr, responseBytes.data(), &responseLength);

        auto response = toResponse(responseBytes, responseLength);

        if (response.sw1 == ResponseApdu::MORE_DATA_AVAILABLE) {
            getMoreResponseData(response);
        }

        return response;
    }

    ResponseApdu transmitBytesCTL(const byte_vector& commandBytes, uint16_t lang,
                                  uint8_t minlen) const
    {
        uint8_t PINFrameOffset = 0;
        uint8_t PINLengthOffset = 0;
        byte_vector cmd(sizeof(PIN_VERIFY_STRUCTURE));
        auto* data = (PIN_VERIFY_STRUCTURE*)cmd.data();
        data->bTimerOut = PIN_PAD_PIN_ENTRY_TIMEOUT;
        data->bTimerOut2 = PIN_PAD_PIN_ENTRY_TIMEOUT;
        data->bmFormatString =
            FormatASCII | AlignLeft | uint8_t(PINFrameOffset << 4) | PINFrameOffsetUnitBits;
        data->bmPINBlockString = PINLengthNone << 5 | PINFrameSizeAuto;
        data->bmPINLengthFormat = PINLengthOffsetUnitBits | PINLengthOffset;
        data->wPINMaxExtraDigit = uint16_t(minlen << 8) | 12;
        data->bEntryValidationCondition = ValidOnKeyPressed;
        data->bNumberMessage = CCIDDefaultInvitationMessage;
        data->wLangId = lang;
        data->bMsgIndex = NoInvitationMessage;
        data->bTeoPrologue[0] = 0x00;
        data->bTeoPrologue[1] = 0x00;
        data->bTeoPrologue[2] = 0x00;
        data->ulDataLength = uint32_t(commandBytes.size() + 1);
        cmd.insert(cmd.cend(), commandBytes.cbegin(), commandBytes.cend());
        cmd.resize(cmd.size() + 1);

        DWORD ioctl = features.at(features.find(FEATURE_VERIFY_PIN_START) != features.cend()
                                      ? FEATURE_VERIFY_PIN_START
                                      : FEATURE_VERIFY_PIN_DIRECT);
        auto responseBytes = byte_vector(ResponseApdu::MAX_SIZE, 0);
        DWORD responseLength = DWORD(responseBytes.size());
        SCard(Control, cardHandle, ioctl, cmd.data(), DWORD(cmd.size()),
              LPVOID(responseBytes.data()), DWORD(responseBytes.size()), &responseLength);

        if (features.find(FEATURE_VERIFY_PIN_FINISH) != features.cend()) {
            DWORD finish = features.at(FEATURE_VERIFY_PIN_FINISH);
            responseLength = DWORD(responseBytes.size());
            SCard(Control, cardHandle, finish, nullptr, 0U, LPVOID(responseBytes.data()),
                  DWORD(responseBytes.size()), &responseLength);
        }

        return toResponse(responseBytes, responseLength);
    }

    void beginTransaction() const { SCard(BeginTransaction, cardHandle); }

    void endTransaction() const { SCard(EndTransaction, cardHandle, DWORD(SCARD_LEAVE_CARD)); }

    DWORD protocol() const { return _protocol.dwProtocol; }

private:
    SCARDHANDLE cardHandle;
    const SCARD_IO_REQUEST _protocol;
    std::map<DRIVER_FEATURES, uint32_t> features;

    ResponseApdu toResponse(byte_vector& responseBytes, size_t responseLength) const
    {
        if (responseLength > responseBytes.size()) {
            THROW(Error, "SCardTransmit: received more bytes than buffer size");
        }
        responseBytes.resize(responseLength);

        // TODO: debug("Received: " + bytes2hexstr(responseBytes))

        auto response = ResponseApdu::fromBytes(responseBytes);

        // Let expected errors through for handling in upper layers or in if blocks below.
        switch (response.sw1) {
        case ResponseApdu::OK:
        case ResponseApdu::MORE_DATA_AVAILABLE: // See the if block after next.
        case ResponseApdu::VERIFICATION_FAILED:
        case ResponseApdu::VERIFICATION_CANCELLED:
        case ResponseApdu::WRONG_LENGTH:
        case ResponseApdu::COMMAND_NOT_ALLOWED:
        case ResponseApdu::WRONG_PARAMETERS:
        case ResponseApdu::WRONG_LE_LENGTH: // See next if block.
            break;
        default:
            THROW(Error,
                  "Error response: '" + bytes2hexstr({response.sw1, response.sw2}) + "', protocol "
                      + std::to_string(protocol()));
        }

        if (response.sw1 == ResponseApdu::WRONG_LE_LENGTH) {
            THROW(Error, "Wrong LE length (SW1=0x6C) in response, please set LE");
        }

        return response;
    }

    void getMoreResponseData(ResponseApdu& response) const
    {
        auto getResponseCommand = byte_vector {0x00, 0xc0, 0x00, 0x00, 0x00};

        auto newResponse = ResponseApdu(response.sw1, response.sw2);

        while (newResponse.sw1 == ResponseApdu::MORE_DATA_AVAILABLE) {
            getResponseCommand[4] = newResponse.sw2;
            newResponse = transmitBytes(getResponseCommand);
            response.data.insert(response.data.end(), newResponse.data.cbegin(),
                                 newResponse.data.cend());
        }

        response = {ResponseApdu::OK, 0};
    }
};

SmartCard::TransactionGuard::TransactionGuard(const CardImpl& card, bool& inProgress) :
    card(card), inProgress(inProgress)
{
    card.beginTransaction();
    inProgress = true;
}

SmartCard::TransactionGuard::~TransactionGuard()
{
    inProgress = false;
    try {
        card.endTransaction();
    } catch (...) {
        // Ignore exceptions in destructor.
    }
}

SmartCard::SmartCard(const ContextPtr& contex, const string_t& readerName, byte_vector atr) :
    card(std::make_unique<CardImpl>(connectToCard(contex->handle(), readerName))),
    _atr(std::move(atr)), _protocol(convertToSmartCardProtocol(card->protocol()))
{
    // TODO: debug("Card ATR -> " + bytes2hexstr(atr))
}

SmartCard::SmartCard() = default;
SmartCard::~SmartCard() = default;

SmartCard::TransactionGuard SmartCard::beginTransaction()
{
    REQUIRE_NON_NULL(card)
    return SmartCard::TransactionGuard {*card, transactionInProgress};
}

bool SmartCard::readerHasPinPad() const
{
    return card ? card->readerHasPinPad() : false;
}

ResponseApdu SmartCard::transmit(const CommandApdu& command) const
{
    REQUIRE_NON_NULL(card)
    if (!transactionInProgress) {
        THROW(std::logic_error, "Call SmartCard::transmit() inside a transaction");
    }

    return card->transmitBytes(command.toBytes());
}

ResponseApdu SmartCard::transmitCTL(const CommandApdu& command, uint16_t lang, uint8_t minlen) const
{
    REQUIRE_NON_NULL(card);
    if (!transactionInProgress) {
        THROW(std::logic_error, "Call SmartCard::transmit() inside a transaction");
    }

    return card->transmitBytesCTL(command.toBytes(), lang, minlen);
}

} // namespace pcsc_cpp
