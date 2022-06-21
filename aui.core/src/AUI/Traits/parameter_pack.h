/*
 * =====================================================================================================================
 * Copyright (c) 2021 Alex2772
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
 
 * Original code located at https://github.com/aui-framework/aui
 * =====================================================================================================================
 */

#pragma once

#include <algorithm>

namespace aui {
    namespace impl {
        template<typename First, typename... Up>
        struct first {
            using type = First;
        };
    }
    struct parameter_pack {
        template<typename Callable, typename... Args>
        inline static void for_each(Callable c, Args&&... args) {
            invoke(std::move(c), std::forward<Args>(args)...);
        }
        template<typename... Types>
        using first = typename impl::first<Types...>::type;
    private:

        template<typename Callable, typename Arg1>
        inline static void invoke(Callable c, Arg1&& arg1) {
            c(std::forward<Arg1>(arg1));
        }
        template<typename Callable, typename Arg1, typename... Args>
        inline static void invoke(Callable c, Arg1&& arg1, Args&&... args) {
            c(std::forward<Arg1>(arg1));
            invoke(std::move(c), std::forward<Args>(args)...);
        }

    };
}