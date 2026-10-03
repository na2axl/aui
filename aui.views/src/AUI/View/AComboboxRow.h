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

#include <AUI/View/AViewContainer.h>
#include <AUI/ASS/AAssHelper.h>
#include <AUI/ASS/Selector/Selected.h>

/**
 * @brief One selectable row of a combo box popup.
 * @ingroup views_input
 * @details Carries the ".list-item" ass name, which is what gives it the shared hover and
 * Selected(...) backgrounds from the default stylesheet, and implements ass::ISelectable so the
 * Selected selector can read its state.
 */
class API_AUI_VIEWS AComboboxRow : public AViewContainerBase, public ass::ISelectable {
public:
    explicit AComboboxRow(_<AView> content);

    void setSelected(bool selected);
    [[nodiscard]] bool isSelected() const noexcept { return mSelected; }

protected:
    bool selectableIsSelectedImpl() override { return mSelected; }

private:
    bool mSelected = false;
};
