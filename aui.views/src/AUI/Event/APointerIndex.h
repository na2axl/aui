//  AUI Framework - Declarative UI toolkit for modern C++20
//  Copyright (C) 2020-2023 Alex2772
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <compare>
#include <AUI/Platform/AInput.h>

/**
 * @brief Wrapper class that stores either mouse button index or finger index.
 * @ingroup views
 */
class APointerIndex {
public:
    constexpr APointerIndex() noexcept: mValue(AInput::LBUTTON) {}

    static constexpr APointerIndex button(AInput::Key button) {
        assert((button == AInput::LBUTTON ||
                button == AInput::RBUTTON ||
                button == AInput::CBUTTON));
        return APointerIndex((int)button);
    }

    static constexpr APointerIndex finger(int fingerIndex) {
        return APointerIndex(fingerIndex + MAX_BUTTON_VALUE + 1);
    }


    /**
     * @return true if (mouse) button.
     */
    [[nodiscard]]
    bool isButton() const noexcept {
        return mValue <= MAX_BUTTON_VALUE;
    }

    /**
     * @return true if finger index.
     */
    [[nodiscard]]
    bool isFinger() const noexcept {
        return mValue > MAX_BUTTON_VALUE;
    }

    /**
     * @return (mouse) button, if it is.
     */
    AOptional<AInput::Key> button() const noexcept {
        if (!isButton()) {
            return std::nullopt;
        }
        return (AInput::Key)mValue;
    }

    /**
     * @return finger index, if it is.
     */
    AOptional<int> finger() const noexcept {
        if (!isFinger()) {
            return std::nullopt;
        }
        return mValue - MAX_BUTTON_VALUE - 1;
    }

    [[nodiscard]]
    auto operator<=>(const APointerIndex& rhs) const noexcept = default;

    [[nodiscard]]
    constexpr int rawValue() const noexcept {
        return mValue;
    }

private:
    int mValue;
    static constexpr int MAX_BUTTON_VALUE = AInput::RBUTTON;

    explicit constexpr APointerIndex(int value) : mValue(value) {}
};


inline std::ostream& operator<<(std::ostream& o, const APointerIndex& color) {
    o << "APointerIndex{";
    if (auto b = color.button()) {
        o << "button=";
        switch (*b) {
            case AInput::LBUTTON:
                o << "l";
                break;
            case AInput::RBUTTON:
                o << "r";
                break;
            case AInput::CBUTTON:
                o << "c";
                break;
            default:
                o << (int)*b;
                break;
        }
    } else if (auto b = color.finger()) {
        o << "finger=" << *b;
    }
    o << "}";
    return o;
}