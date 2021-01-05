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
// Created by alex2 on 31.08.2020.
//

#include <boost/test/unit_test.hpp>
#include <AUI/Data/ASqlDatabase.h>
#include <AUI/Data/AMeta.h>

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(Meta)


BOOST_AUTO_TEST_CASE(Meta) {
        Autumn::put(ASqlDatabase::connect("sqlite", ":memory:"));
        AMeta::set("kek", 4);
        AMeta::set("lol", "azaza");
        BOOST_CHECK_EQUAL(AMeta::get("kek"), 4);
        BOOST_CHECK_EQUAL(AMeta::get("lol"), "azaza");
        AMeta::set("kek", "four");
        AMeta::set("lol", 42.0);
        BOOST_CHECK_EQUAL(AMeta::get("kek"), "four");
        BOOST_CHECK_EQUAL(AMeta::get("lol"), 42.0);
}


BOOST_AUTO_TEST_SUITE_END()