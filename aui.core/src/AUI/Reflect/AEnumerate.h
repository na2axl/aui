/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2024 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//
// Created by alex2 on 6/24/2021.
//

#pragma once

#include <type_traits>
#include <AUI/Common/AString.h>
#include <AUI/Common/AMap.h>
#include <AUI/Traits/types.h>
#include <fmt/format.h>


//NOLINTBEGIN(modernize-*,cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

template<typename enum_t>
class AEnumerate {
private:
    struct enum_less {
        constexpr bool operator()(enum_t l, enum_t r) const {
            return static_cast<std::underlying_type_t<enum_t>>(l) < static_cast<std::underlying_type_t<enum_t>>(r);
        }
    };

public:
    static_assert(std::is_enum_v<enum_t>, "AEnumerate accepts only enums");



    template<enum_t... values>
    struct Values {

    };

    template<enum_t value>
    static AString valueName() {
#if AUI_COMPILER_MSVC
        AString s = __FUNCSIG__;
        AString::iterator end = s.begin() + s.rfind('>');
        AString::iterator begin = (std::find_if(std::make_reverse_iterator(end), s.rend(), [](wchar_t c) {
            return c == ':' || c == '<';
        })).base();

        AString result(begin, end);
#else
        AString s = __PRETTY_FUNCTION__;
#if AUI_COMPILER_CLANG
        auto end = s.rfind(']');
#else
        auto end = s.rfind(';');
#endif
        size_t begin = s.rfind("value =", end);
        if (begin == AString::NPOS) {
            begin = s.rfind('[', end) + 1;
        } else {
            begin += 8;
        }
        AString result = {s.begin() + begin, s.begin() + end};


        for (size_t p; (p = result.find("::")) != AString::NPOS;) {
            result = result.substr(p + 2);
        }

#endif
        return result;
    }

    template<enum_t... values>
    static const AMap<AString, enum_t>& mapValueByName() {
        static AMap<AString, enum_t> map = {
            {valueName<values>(), values}...
        };
        return map;
    }

    template<enum_t... values>
    static const AMap<enum_t, AString, enum_less>& mapNameByValue() {
        static AMap<enum_t, AString, enum_less> map = {
            {values, valueName<values>() }...
        };
        return map;
    }

    static const AMap<AString, enum_t>& all();
    static const AMap<enum_t, AString, enum_less>& names();

    static enum_t byName(const AString& name) {
        if (auto c = all().contains(name.uppercase())) {
            return c->second;
        }
        return (enum_t)0;
    }

private:
    template<enum_t... values>
    static const AMap<AString, enum_t>& mapValueByNameImpl(const Values<values...>& v) {
        return mapValueByName<values...>();
    }
    template<enum_t... values>
    static const AMap<enum_t, AString, enum_less>& mapNameByValueImpl(const Values<values...>& v) {
        return mapNameByValue<values...>();
    }
};

template<typename enum_t>
struct AEnumerateAllValues;

template<typename enum_t>
const AMap<AString, enum_t>& AEnumerate<enum_t>::all() {
    static_assert(aui::is_complete<AEnumerateAllValues<enum_t>>, "AUI_ENUM_VALUES is not defined for this enum type");
    auto v = AEnumerateAllValues<enum_t>::get();

    return mapValueByNameImpl(v);
}

template<typename enum_t>
const AMap<enum_t, AString, typename AEnumerate<enum_t>::enum_less>& AEnumerate<enum_t>::names() {
    static_assert(aui::is_complete<AEnumerateAllValues<enum_t>>, "AUI_ENUM_VALUES is not defined for this enum type");
    auto v = AEnumerateAllValues<enum_t>::get();

    return mapNameByValueImpl(v);
}

#define AUI_ENUM_VALUES(enum_t, ...) template<> \
struct AEnumerateAllValues<enum_t>{         \
    static inline constexpr AEnumerate<enum_t>::Values<__VA_ARGS__> get() {return {}; } \
};                                         \
namespace std { inline AString to_wstring(enum_t v) { return AEnumerate<enum_t>::names().optional(v).valueOr("<unknown enum value {}>"_format(int(v))); } } \
inline std::ostream& operator<<(std::ostream& o, enum_t v) { return o << std::to_wstring(v); }

template <typename T> struct fmt::formatter<T, char, std::enable_if_t<aui::is_complete<AEnumerateAllValues<T>>>>: formatter<std::string> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(T c, FormatContext& ctx) const {
        return formatter<string_view>::format(AEnumerate<T>::names()[c].toStdString(), ctx);
    }
};



#define AUI_ENUM_FLAG(name) enum class name: int; \
                            constexpr inline name operator|(name a, name b) {return static_cast<name>(static_cast<int>(a) | static_cast<int>(b));} \
                            constexpr inline name operator&(name a, name b) {return static_cast<name>(static_cast<int>(a) & static_cast<int>(b));} \
                            constexpr inline name operator^(name a, name b) {return static_cast<name>(static_cast<int>(a) ^ static_cast<int>(b));} \
                            constexpr inline name operator|=(name& a, name b) {return a = static_cast<name>(static_cast<int>(a) | static_cast<int>(b));} \
                            constexpr inline name operator&=(name& a, name b) {return a = static_cast<name>(static_cast<int>(a) & static_cast<int>(b));} \
                            constexpr inline name operator^=(name& a, name b) {return a = static_cast<name>(static_cast<int>(a) ^ static_cast<int>(b));} \
                            constexpr inline name operator~(const name& a) {return static_cast<name>(~static_cast<int>(a));} \
                            constexpr inline bool operator!(const name& a) {return a == static_cast<name>(0);}                                     \
                                                                   \
                            constexpr inline bool operator&&(const name& a, bool v) {return static_cast<int>(a) && v;}                                     \
                            constexpr inline bool operator||(const name& a, bool v) {return static_cast<int>(a) || v;}                                     \
\
                            enum class name: int

#define AUI_ENUM_INT(name) enum class name: int; \
                           constexpr inline bool operator<(name a, name b) {return static_cast<int>(a) < static_cast<int>(b);} \
                           constexpr inline bool operator>(name a, name b) {return static_cast<int>(a) > static_cast<int>(b);} \
                           enum class name: int

//NOLINTEND(modernize-*,cppcoreguidelines-macro-usage,bugprone-macro-parentheses)