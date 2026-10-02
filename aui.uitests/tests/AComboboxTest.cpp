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

#include <AUI/UITest.h>
#include <AUI/Common/ATimer.h>
#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/View/ALabel.h>
#include <AUI/Platform/AWindow.h>
#include <AUI/View/AViewContainer.h>
#include <AUI/View/AComboboxRow.h>

using namespace declarative;

// AUI's connect() requires the receiver to derive from AObjectBase. A testing::UITest fixture does
// not, so every signal observation in this file goes through a file-scope sink and a counter.
static AObject gSink;
static int gNotifications = 0;

class AComboboxRowTest : public testing::UITest {
public:
protected:
    class TestWindow : public AWindow {
    public:
        TestWindow() : AWindow("AComboboxRowTest", 400_dp, 300_dp) {}
    };

    _<TestWindow> mWindow;
    _<AComboboxRow> mRow;

    void SetUp() override {
        UITest::SetUp();
        mWindow = _new<TestWindow>();
        mWindow->setContents(Vertical {
            mRow = _new<AComboboxRow>(_new<ALabel>("row")),
        });
        mWindow->show();
        uitest::frame();
    }

    void TearDown() override {
        mWindow->removeAllViews();
        AThread::processMessages();
        UITest::TearDown();
    }
};

TEST_F(AComboboxRowTest, CarriesListItemAssName) {
    EXPECT_TRUE(mRow->getAssNames().contains(".list-item"));
}

TEST_F(AComboboxRowTest, SelectionRoundTrips) {
    EXPECT_FALSE(mRow->isSelected());
    mRow->setSelected(true);
    EXPECT_TRUE(mRow->isSelected());
    mRow->setSelected(false);
    EXPECT_FALSE(mRow->isSelected());
}

TEST_F(AComboboxRowTest, SelectionChangeInvalidatesCss) {
    gNotifications = 0;
    AObject::connect(mRow->customCssPropertyChanged, &gSink, [&] { ++gNotifications; });

    mRow->setSelected(true);
    EXPECT_GE(gNotifications, 1);

    // A redundant set must not notify, so consumers are not woken for nothing.
    const int after = gNotifications;
    mRow->setSelected(true);
    EXPECT_EQ(gNotifications, after);
}

TEST_F(AComboboxRowTest, IsClickable) {
    gNotifications = 0;
    AObject::connect(mRow->clicked, &gSink, [&] { ++gNotifications; });

    By::type<AComboboxRow>().perform(click());
    uitest::frame();
    EXPECT_EQ(gNotifications, 1);
}