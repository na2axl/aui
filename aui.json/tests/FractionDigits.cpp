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

#include <gtest/gtest.h>
#include <AUI/Json/AJson.h>

namespace {
double parseValue(const AString& document) {
    return AJson::fromString(document)["value"].asNumber();
}
}   // namespace

TEST(JsonFraction, EndingALineUnderLf) {
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": 0.5\n}"), 0.5);
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": 1.25\n}"), 1.25);
}

TEST(JsonFraction, EndingALineUnderCrLf) {
    EXPECT_DOUBLE_EQ(parseValue("{\r\n  \"value\": 0.5\r\n}"), 0.5);
    EXPECT_DOUBLE_EQ(parseValue("{\r\n  \"value\": 1.25\r\n}"), 1.25);
}

TEST(JsonFraction, EndingTheDocument) {
    EXPECT_DOUBLE_EQ(AJson::fromString("0.5").asNumber(), 0.5);
    EXPECT_DOUBLE_EQ(AJson::fromString("12.375").asNumber(), 12.375);
}

TEST(JsonFraction, LeadingZeros) {
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": 0.05\n}"), 0.05);
    EXPECT_DOUBLE_EQ(parseValue("{\"value\": 0.005}"), 0.005);
    EXPECT_DOUBLE_EQ(parseValue("{\"value\": 3.0}"), 3.0);
}

TEST(JsonFraction, Negative) {
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": -0.5\n}"), -0.5);
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": -1.25\n}"), -1.25);
    EXPECT_DOUBLE_EQ(parseValue("{\r\n  \"value\": -0.05\r\n}"), -0.05);
}

TEST(JsonFraction, Exponents) {
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": 1.5e3\n}"), 1500.0);
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": 2.5E-2\n}"), 0.025);
    EXPECT_DOUBLE_EQ(parseValue("{\n  \"value\": -1.5e3\n}"), -1500.0);
}
