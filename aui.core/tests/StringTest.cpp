/**
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

//
// Created by alex2 on 30.08.2020.
//

#include <gtest/gtest.h>
#include <AUI/Common/AString.h>



TEST(String, UTF8)
{
    const char* someUtfString = "Ютф строка! \u0228";

    AString s = someUtfString;

    ASSERT_TRUE(s == someUtfString);
    ASSERT_TRUE(memcmp(s.toUtf8().data(), someUtfString, strlen(someUtfString)) == 0);

}
TEST(String, lowercase_en)
{
    ASSERT_EQ("Hello"_as.lowercase(), "hello");
}
TEST(String, uppercase_en)
{
    ASSERT_EQ("Hello"_as.uppercase(), "HELLO");
}

TEST(String, lowercase_ru)
{
    ASSERT_EQ("Привет"_as.lowercase(), "привет");
}
TEST(String, uppercase_ru)
{
    ASSERT_EQ("Привет"_as.uppercase(), "ПРИВЕТ");
}


