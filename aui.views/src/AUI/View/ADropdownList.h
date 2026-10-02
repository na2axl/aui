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

#include "ACombobox.h"

/**
 * @brief A button with dropdown list.
 *
 * ![](imgs/views/ADropdownList.png)
 *
 * @ingroup views_input
 * @details
 * Shows the selected item as a button; a click opens a popup with all the items. The items come from a list model of
 * strings (`AListModel<AString>` is the simplest one).
 *
 * <!-- aui:snippet examples/ui/views/src/ExampleWindow.cpp dropdown -->
 *
 * `selectionId()` is a property with the index of the selected item (starting from 0, the first item is selected by
 * default). Bind it or listen to `selectionChanged`:
 *
 * ```cpp
 * auto list = _new<ADropdownList>(AListModel<AString>::make({ "one-way flight", "return flight" }));
 * AObject::connect(list->selectionId().changed, AObject::GENERIC_OBSERVER, [](int index) {
 *     ALogger::info("App") << "Selected " << index;
 * });
 * ```
 *
 * The model can be replaced later with `setModel`. Like any other view, the list can be disabled with `setDisabled()`.
 *
 * An ACombobox over AString, kept under the name every existing call site already uses.
 * This is an alias rather than a subclass so that ass::t<ADropdownList>() in the default
 * stylesheet keeps matching, and so the framework ends up with one popup implementation rather
 * than two.
 */
using ADropdownList = ACombobox<AString>;
