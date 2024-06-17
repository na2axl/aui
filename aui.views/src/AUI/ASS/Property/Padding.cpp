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
// Created by alex2 on 01.01.2021.
//

#include "Padding.h"

void ass::prop::Property<ass::Padding>::applyFor(AView* view) {
    auto padding = view->getPadding();

    mInfo.left.bind_to(padding.left);
    mInfo.top.bind_to(padding.top);
    mInfo.right.bind_to(padding.right);
    mInfo.bottom.bind_to(padding.bottom);

    view->setPadding(padding);
}