#include <cstring>
#include "AString.h"
#include "AStringVector.h"

AString::AString(const char* str) noexcept
{
    reserve(strlen(str));

    // прочитаем-ка utf8
    while (*str)
    {
        if (*str & 0x80)
        {
            wchar_t t;
            // ненормальный utf8 символ
            if (*str & 0b00100000)
            {
                // 3х-байтный символ
                t = *(str++) & 0b1111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                push_back(t);
            } else
            {
                // 2х-байтный символ
                t = *(str++) & 0b11111;
                t <<= 6;
                t |= *(str++) & 0b111111;
                push_back(t);
            }
        } else
        {
            // нормальный ascii символ
            push_back(*(str++));
        }
    }
}

_<ByteBuffer> AString::toUtf8() const
{
    auto buf = _new<ByteBuffer>();
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
    AStringVector result;
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

AString AString::replaceAll(const AString& from, const AString& to) const noexcept
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
        return std::stoi(*this);
    } catch (...)
    {
        return 0;
    }
}

bool AString::toBool() const noexcept
{
    return *this == "true";
}

AString AString::fromLatin1(_<ByteBuffer> buffer)
{
    AString result;

    result.reserve(buffer->getAvailable());

    while (buffer->getAvailable())
    {
        char b;
        buffer->get(&b, 1);
        result += b;
    }

    return result;
}

AString AString::number(int i) noexcept
{
    return std::to_wstring(i);
}
AString AString::number(unsigned i) noexcept
{
    return std::to_wstring(i);
}
AString AString::number(float i) noexcept
{
    return std::to_wstring(i);
}
AString AString::number(double i) noexcept
{
    return std::to_wstring(i);
}
AString AString::number(bool i) noexcept
{
    if (i)
        return "true";
    return "false";
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