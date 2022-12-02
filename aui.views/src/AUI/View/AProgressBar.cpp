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
// Created by Alex2772 on 1/8/2022.
//

#include "AProgressBar.h"

AProgressBar::AProgressBar() : mInner(_new<Inner>()) {
    addView(mInner);
    mInner->setPosition({0, 0});
}

AProgressBar::Inner::~Inner() {

}

AProgressBar::~AProgressBar() {

}

void AProgressBar::setSize(glm::ivec2 size) {
    AViewContainer::setSize(size);
    updateInnerWidth();
}

void AProgressBar::updateInnerWidth() {
    mInner->setGeometry(mPadding.left,
                        mPadding.top,
                        int(mValue * float(getContentWidth() - mPadding.horizontal())),
                        getContentHeight() - mPadding.vertical());
}

void AProgressBar::render() {
    AViewContainer::render();
}
