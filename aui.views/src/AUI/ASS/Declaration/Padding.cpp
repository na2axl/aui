// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
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
// Created by alex2 on 01.01.2021.
//

#include "Padding.h"

void ass::decl::Declaration<ass::Padding>::applyFor(AView* view) {
    auto padding = view->getPadding();

    mInfo.left.bind_to(padding.left);
    mInfo.top.bind_to(padding.top);
    mInfo.right.bind_to(padding.right);
    mInfo.bottom.bind_to(padding.bottom);

    view->setPadding(padding);
}