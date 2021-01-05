/**
 * =====================================================================================================================
 * Copyright (c) 2020 Alex2772
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 * Original code located at https://github.com/Alex2772/aui
 * =====================================================================================================================
 */

//
// Created by alex2 on 02.01.2021.
//

#pragma once

#include "attribute.h"
#include <AUI/Util/kAUI.h>
#include <AUI/View/AView.h>
#include <AUI/ASS/AAssHelper.h>

#include "hovered.h"
#include "active.h"
#include "focus.h"

namespace ass {

    namespace detail {
        template<typename T>
        struct Type : virtual IAssSubSelector {
        public:
            bool isPossiblyApplicable(AView* view) override {
                return dynamic_cast<T*>(view) != nullptr;
            }

        };
    }

    template<typename T>
    struct any: detail::Type<T>, AttributeHelper<any<T>> {
    public:

        using hover = ass::hovered<detail::Type<T>>;
        using active = ass::active<detail::Type<T>>;
        using focus = ass::active<detail::Type<T>>;
    };
}