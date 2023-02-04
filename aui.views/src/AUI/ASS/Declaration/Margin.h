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

#pragma once

#include <AUI/Util/AMetric.h>
#include "IDeclaration.h"

namespace ass {

    /**
     * @brief Controls view's @ref AView::setMargin "margins".
     * @ingroup ass
     */
    struct Margin {
        unset_wrap<AMetric> left;
        unset_wrap<AMetric> top;
        unset_wrap<AMetric> right;
        unset_wrap<AMetric> bottom;

        Margin(unset_wrap<AMetric> all):
            left(all),
            top(all),
            right(all),
            bottom(all)
        {
        }

        Margin(unset_wrap<AMetric> vertical, unset_wrap<AMetric> horizontal):
            left(horizontal),
            top(vertical),
            right(horizontal),
            bottom(vertical)
        {
        }

        Margin(unset_wrap<AMetric> top, unset_wrap<AMetric> horizontal, unset_wrap<AMetric> bottom):
            left(horizontal),
            top(top),
            right(horizontal),
            bottom(bottom)
        {
        }

        Margin(unset_wrap<AMetric> top, unset_wrap<AMetric> right, unset_wrap<AMetric> bottom, unset_wrap<AMetric> left):
            left(left),
            top(top),
            right(right),
            bottom(bottom)
        {
        }
    };

    namespace decl {
        template<>
        struct API_AUI_VIEWS Declaration<Margin>: IDeclarationBase {
        private:
            Margin mInfo;

        public:
            Declaration(const Margin& info) : mInfo(info) {

            }

            void applyFor(AView* view) override;

            [[nodiscard]]
            const auto& value() const noexcept {
                return mInfo;
            }
        };
    }
}