// AUI Framework - Declarative UI toolkit for modern C++17
// Copyright (C) 2020-2022 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

//
// Created by Alex2772 on 2/4/2022.
//

#pragma once

#include "values.h"
#include "types.h"
#include <AUI/Common/SharedPtr.h>

class IInputStream;
class IOutputStream;

template<typename T, typename T2 = void>
struct ASerializable;

namespace aui {

    template<typename T>
    constexpr bool is_serializable = aui::is_complete<ASerializable<T>>;

    template<typename T>
    inline void serialize(aui::no_escape<IOutputStream> dst, const T& t) {
        static_assert(is_serializable<T>, "T is not serializable");
        ASerializable<T>::write(*dst, t);
    }

    template<typename T>
    inline void deserialize(aui::no_escape<IInputStream> from, T& t) {
        static_assert(is_serializable<T>, "T is not serializable");
        ASerializable<T>::read(*from, t);
    }

    template<typename T>
    inline T deserialize(aui::no_escape<IInputStream> from) {
        T t;
        deserialize<T>(from, t);
        return t;
    }
}

#include <AUI/IO/IInputStream.h>
#include <AUI/IO/IOutputStream.h>

namespace aui {
    template<typename T>
    struct raw_serializable {
        static void write(IOutputStream& os, const T& value) {
            os.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
        static void read(IInputStream& is, T& t) {
            is.readExact(reinterpret_cast<char*>(&t), sizeof(T));
        }
    };

    template<typename T>
    struct serialize_sized {
        T* value;

        serialize_sized(T& value): value(&value) {}
        serialize_sized(T&& value): value(&value) {}
    };
    template<typename T>
    struct serialize_raw {
        T* value;

        serialize_raw(T& value): value(&value) {}
        serialize_raw(T&& value): value(&value) {}
    };
}

// ints, floats, doubles, etc...
template<typename T>
struct ASerializable<T, std::enable_if_t<std::is_arithmetic_v<T>>>: aui::raw_serializable<T> {};

// _<SerializableType>
template<typename T>
struct ASerializable<_<T>> {
    static void write(IOutputStream& os, const _<T>& value) {
        aui::serialize(os, *value);
    }
    static void read(IInputStream& is, _<T>& t) {
        t = _new<T>(std::move(aui::deserialize<T>(is)));
    }
};

// std::string
template<>
struct ASerializable<std::string> {
    static void write(IOutputStream& os, const std::string& value) {
        os.write(value.data(), value.length());
    }
};

// input stream
template<typename T>
struct ASerializable<T, std::enable_if_t<std::is_base_of_v<IInputStream, T>>> {
    static void write(IOutputStream& os, const T& value) {
        char buf[0x1000];
        for (size_t r; (r = const_cast<T&>(value).read(buf, sizeof(buf))) != 0;) {
            os.write(buf, r);
        }
    }
};

// string literal
template<int L>
struct ASerializable<char[L]> {
    static void write(IOutputStream& os, const char* value) {
        os.write(value, L - 1);
    }
};

// also string literal, but unknown blob size
template<>
struct ASerializable<const char*> {
    static void write(IOutputStream& os, const char* value) {
        os.write(value, std::strlen(value));
    }
};


template<>
struct ASerializable<AString> {
    static void write(IOutputStream& os, const AString& value) {
        aui::serialize(os, value.toStdString());
    }
};

template<>
struct ASerializable<APath>: ASerializable<AString> {};


template<typename T>
struct ASerializable<aui::serialize_sized<T>> {
    static void write(IOutputStream& os, aui::serialize_sized<T> t) {
        os << std::uint32_t(t.value->size());
        os.write(reinterpret_cast<const char*>(t.value->data()), sizeof(*t.value->data()) * t.value->size());
    }
    static void read(IInputStream& is, aui::serialize_sized<T>& t) {
        std::uint32_t s;
        is >> s;
        t.value->resize(s);
        is.read(reinterpret_cast<char*>(t.value->data()), sizeof(*t.value->data()) * t.value->size());
    }
};

template<typename T>
struct ASerializable<aui::serialize_raw<T>> {
    static void write(IOutputStream& os, aui::serialize_raw<T> t) {
        os.write(reinterpret_cast<const char*>(t.value), sizeof(T));
    }
    static void read(IInputStream& is, aui::serialize_raw<T>& t) {
        is.read(reinterpret_cast<char*>(t.value), sizeof(T));
    }
};
