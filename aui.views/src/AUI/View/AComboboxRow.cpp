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

#include "AComboboxRow.h"

#include <AUI/Layout/AVerticalLayout.h>

AComboboxRow::AComboboxRow(const _<AView>& content) {
    addAssName(".list-item");
    setLayout(std::make_unique<AVerticalLayout>());
    addView(content);
}

void AComboboxRow::setSelected(bool selected) {
    if (mSelected == selected) {
        return;
    }
    mSelected = selected;
    // Selected(...) subscribes to customCssPropertyChanged to invalidate, so this is the
    // notification that actually repaints the row.
    emit customCssPropertyChanged;
}