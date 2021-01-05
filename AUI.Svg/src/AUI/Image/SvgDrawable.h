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

#pragma once
#include <functional>
#include <AUI/Common/SharedPtrTypes.h>

#include "AUI/Common/ADeque.h"
#include "AUI/Image/IDrawable.h"
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <AUI/GL/Texture2D.h>


class SvgDrawable: public IDrawable
{
private:
    struct Pair {
        uint64_t key;
        _<GL::Texture2D> texture;
    };

    NSVGimage* mImage;
    ADeque<Pair> mRasterized;

public:
    explicit SvgDrawable(AByteBuffer& data);
    ~SvgDrawable();

	void draw(const glm::ivec2& size) override;
	glm::ivec2 getSizeHint() override;

	bool isDpiDependent() const override;

};
