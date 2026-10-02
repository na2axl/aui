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
#include <AUI/Model/AListModel.h>
#include <AUI/Model/AListModelFilter.h>
#include <AUI/Model/AModels.h>
#include <AUI/Common/AObject.h>

// AUI's connect() requires the receiver to derive from AObjectBase. A gtest fixture does not, so
// every signal observation in these tests goes through a file-scope sink object and a counter.
static AObject gSink;
static int gNotifications = 0;

static AVector<AString> itemsOf(const _<IListModel<AString>>& model) {
    AVector<AString> result;
    for (size_t i = 0; i < model->listSize(); ++i) {
        result << model->listItemAt(AListModelIndex(i));
    }
    return result;
}

TEST(AListModelFilter, FiltersTheSource) {
    auto source = AListModel<AString>::make({ "alpha", "beta", "gamma" });
    auto filter = AModels::filter(source, [](const AString& item) { return item.contains("l"); });

    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "alpha" }));
}

TEST(AListModelFilter, SetFilterReFilters) {
    auto source = AListModel<AString>::make({ "alpha", "beta", "gamma" });
    auto filter = AModels::filter(source, [](const AString&) { return true; });
    EXPECT_EQ(itemsOf(filter).size(), 3u);

    filter->setFilter([](const AString& item) { return item.contains("l"); });
    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "alpha" }));
}

TEST(AListModelFilter, SurvivesSourceInsert) {
    auto source = AListModel<AString>::make({ "alpha", "beta" });
    auto filter = AModels::filter(source, [](const AString& item) { return item.contains("a"); });

    source->push_back("gamma");                     // left the filter stale before this change
    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "alpha", "beta", "gamma" }));
}

TEST(AListModelFilter, SurvivesSourceChangeAndRemove) {
    auto source = AListModel<AString>::make({ "alpha", "beta" });
    auto filter = AModels::filter(source, [](const AString& item) { return item.contains("a"); });

    source->setItem(AListModelIndex(0), "omega");
    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "omega", "beta" }));

    source->removeItem(AListModelIndex(1));
    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "omega" }));
}

TEST(AListModelFilter, NotifiesWhenContentChangesAtTheSameSize) {
    auto source = AListModel<AString>::make({ "ac", "bd" });
    auto filter = AModels::filter(source, [](const AString& item) { return item.contains("a"); });
    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "ac" }));

    gNotifications = 0;
    AObject::connect(filter->dataChanged, &gSink, [&](const AListModelRange<AString>&) { ++gNotifications; });
    filter->setFilter([](const AString& item) { return item.contains("b"); });

    EXPECT_EQ(itemsOf(filter), (AVector<AString>{ "bd" }));
    EXPECT_GE(gNotifications, 1);
}
