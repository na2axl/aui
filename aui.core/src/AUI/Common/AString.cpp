/**
 * =====================================================================================================================
 * Copyright (c) 2021 Alex2772
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 * Original code located at https://github.com/aui-framework/aui
 * =====================================================================================================================
 */

#include <cstring>
#include "AString.h"
#include "AStringVector.h"

inline static void fromUtf8_impl(AString& destination, const char* str, size_t length) {
    destination.reserve(length);

    // parse utf8
    for (; *str && length; --length)
    {
        if (*str & 0x80)
        {
            wchar_t t;
            // utf8 symbol
            if (*str & 0b00100000)
            {
                // 3-byte symbol
                t = *(str++) & 0b1111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                destination.push_back(t);
            } else
            {
                // 2-byte symbol
                t = *(str++) & 0b11111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                destination.push_back(t);
            }
        } else
        {
            // ascii symbol
            destination.push_back(*(str++));
        }
    }
}

AString::AString(const char* str) noexcept
{
    fromUtf8_impl(*this, str, strlen(str));
}

AString::AString(const std::string& str) noexcept
{
    fromUtf8_impl(*this, str.c_str(), str.length());
}

AString AString::fromUtf8(const AByteBuffer& buffer) {
    return AString::fromUtf8(buffer.data(), buffer.getSize());
}

AString AString::fromUtf8(const char* buffer, size_t length) {
    AString result;
    fromUtf8_impl(result, buffer, length);
    return result;
}


_<AByteBuffer> AString::toUtf8() const
{
    auto buf = _new<AByteBuffer>();
    for (wchar_t c : *this)
    {
        if (c >= 0x80)
        {
            if (c >= 0x800)
            {
                char b[] = {
                        static_cast<char>(0b11100000 | (c >> 12 & 0b1111)),
                        static_cast<char>(0b10000000 | (c >> 6 & 0b111111)),
                        static_cast<char>(0b10000000 | (c & 0b111111)),
                };
                buf->put(b, sizeof(b));
            } else if (c >= 0x80)
            {
                char b[] = {
                        static_cast<char>(0b11000000 | (c >> 6 & 0b11111)),
                        static_cast<char>(0b10000000 | (c & 0b111111)),
                };
                buf->put(b, sizeof(b));
            }
        } else
        {
            buf->put(reinterpret_cast<char*>(&c), 1);
        }
    }
    return buf;
}

AStringVector AString::split(wchar_t c) const noexcept
{
    if (empty()) {
        return {};
    }
    AStringVector result;
    result.reserve(length() / 10);
    for (size_type s = 0;;)
    {
        auto next = std::wstring::find(c, s);
        if (next == npos)
        {
            result << substr(s);
            break;
        }

        result << substr(s, next - s);
        s = next + 1;
    }
    return result;
}

AString AString::trimLeft(wchar_t symbol) const noexcept
{
    for (auto i = begin(); i != end(); ++i)
    {
        if (*i != symbol)
        {
            return { i, end() };
        }
    }
    return {};
}

AString AString::trimRight(wchar_t symbol) const noexcept
{
    for (auto i = rbegin(); i != rend(); ++i)
    {
        if (*i != symbol)
        {
            return { begin(),i.base() };
        }
    }
    return {};
}

AString AString::replacedAll(const AString& from, const AString& to) const noexcept
{
    AString result;
    for (size_type pos = 0;;)
    {
        auto next = find(from, pos);
        if (next == NPOS)
        {
            result.insert(result.end(), begin() + pos, end());
            return result;
        }
        result.insert(result.end(), begin() + pos, begin() + next);
        result.insert(result.end(), to.begin(), to.end());
        pos = next + from.length();
    }
}

float AString::toFloat() const noexcept
{
    try {
        return std::stof(*this);
    } catch (...)
    {
        return 0.f;
    }
}

double AString::toDouble() const noexcept
{
    try {
        return std::stod(*this);
    }
    catch (...)
    {
        return 0.0;
    }
}

int AString::toInt() const noexcept
{
    try
    {
        if (length() >= 2) {
            if ((*this)[1] == 'x' || (*this)[1] == 'X') {
                // hex
                return std::stoi(AString{begin() + 2, end()}, nullptr, 16);
            }
        }
        return std::stoi(*this);
    } catch (...)
    {
        return 0;
    }
}
unsigned AString::toUInt() const noexcept
{
    try
    {
        if (length() >= 2) {
            if ((*this)[1] == 'x' || (*this)[1] == 'X') {
                // hex
                return std::stoul(AString{begin() + 2, end()}, nullptr, 16);
            }
        }
        return std::stoul(*this);
    } catch (...)
    {
        return 0;
    }
}

bool AString::toBool() const noexcept
{
    return *this == "true";
}

AString AString::fromLatin1(const AByteBuffer& buffer)
{
    AString result;

    result.reserve(buffer.getAvailable());

    while (buffer.getAvailable())
    {
        char b;
        buffer.get(&b, 1);
        result += b;
    }

    return result;
}


AString AString::fromLatin1(const char* buffer) {
    AString s;
    for (; *buffer; ++buffer)
        s.push_back(*buffer);

    return s;
}


int AString::toNumberDec() const noexcept
{
    int n;
    if (std::swscanf(c_str(), L"%d", &n) < 0)
        return -1;

    return n;
}

int AString::toNumberHex() const noexcept
{
    int n;
    if (std::swscanf(c_str(), L"%x", &n) < 0)
        return -1;

    return n;
}

std::string AString::toStdString() const noexcept
{
    auto encoded = toUtf8();
    std::string dst;
    dst.reserve(encoded->getSize());
    dst.insert(0, encoded->data(), encoded->getSize());

    return dst;
}

AString AString::uppercase() const {
    auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());
    AString result = *this;
    f.toupper(&result[0], &result[0] + result.size());
    return result;
}

AString AString::lowercase() const {
    auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());
    AString result = *this;
    f.tolower(&result[0], &result[0] + result.size());
    return result;
}

void AString::replaceAll(wchar_t from, wchar_t to) noexcept {
    for (auto& s : *this) {
        if (s == from)
            s = to;
    }
}

void AString::resizeToNullTerminator() {
    wchar_t* i;
    for (i = data(); *i; ++i);
    resize(i - data());
}

AString AString::restrictLength(size_t s, const AString& stringAtEnd) const {
    if (length() > s) {
        return mid(0, s) + stringAtEnd;
    }
    return *this;
}

AString AString::numberHex(int i) noexcept {
    char buf[32];
    sprintf(buf, "%x", i);
    return buf;
}

AString AString::processEscapes() const {
    AString result;
    result.reserve(length());
    bool doEscape = false;
    for (auto& c : *this) {
        if (doEscape) {
            doEscape = false;
            switch (c) {
                case '\\':
                    result << '\\';
                    break;
                case 'n':
                    result << '\n';
                    break;
                default:
                    result << c;
            }
        } else if (c == '\\') {
            doEscape = true;
        } else {
            result << c;
        }
    }
    return result;
}

AString AString::excessSpacesRemoved() const {
    AString s;
    s.reserve(length() + 1);
    bool prevWasSpace = false;
    for (auto c : *this) {
        if (c == ' ') {
            if (prevWasSpace) {
                continue;
            }
            prevWasSpace = true;
        } else {
            prevWasSpace = false;
        }
        s << c;
    }
    return s;
}