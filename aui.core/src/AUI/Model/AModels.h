/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2025 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AListModelAdapter.h"
#include "AListModelFilter.h"
#include <optional>
#include <type_traits>
#include <AUI/Common/AOptional.h>

namespace AModels {
    /**
     * @brief Finds the row carrying `value`.
     * @return the index of the first matching row, or std::nullopt when the model carries no such
     *         value.
     * @details Deliberately value-based rather than index-based. A widget whose selection is an
     * index loses it every time the model is replaced or reordered, which is exactly what a
     * filtered list does on every keystroke. On duplicates the first match wins.
     * @note
     * `Model` is deduced rather than spelled `IListModel<T>` so that a `_<AListModel<T>>` can be
     * passed directly: `_<Derived>` only converts to `_<IListModel<T>>` explicitly, which a
     * function argument may not use. The [requires] clause keeps the "is a list model of T" rule.
     */
    template <typename Model, typename T>
        requires std::is_base_of_v<IListModel<T>, Model>
    [[nodiscard]]
    std::optional<std::size_t> indexOf(const _<Model>& model, const T& value) {
        for (std::size_t i = 0; i < model->listSize(); ++i) {
            if (model->listItemAt(AListModelIndex(i)) == value) {
                return i;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief [indexOf] treating an empty optional as "no value selected at all".
     */
    template <typename Model, typename T>
        requires std::is_base_of_v<IListModel<T>, Model>
    [[nodiscard]]
    std::optional<std::size_t> indexOf(const _<Model>& model, const AOptional<T>& value) {
        if (!value.hasValue()) {
            return std::nullopt;
        }
        return AModels::indexOf(model, value.value());
    }
}
