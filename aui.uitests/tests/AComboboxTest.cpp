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
#include <AUI/View/AButton.h>
#include <AUI/Platform/AWindow.h>
#include <AUI/View/AViewContainer.h>
#include <AUI/View/AComboboxRow.h>
#include <AUI/Model/AListModel.h>
#include <AUI/View/ACombobox.h>
#include <AUI/View/ADropdownList.h>
#include <AUI/View/AScrollArea.h>
#include <AUI/View/ATextField.h>
#include <AUI/ASS/AStylesheet.h>
#include <AUI/ASS/Selector/ParentSelector.h>
#include <AUI/ASS/Selector/AAssSelector.h>
#include <AUI/ASS/Selector/type_of.h>
#include <AUI/ASS/Rule.h>

#include <chrono>
#include <thread>

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

// A deliberately non-AString item type, so the generic path is genuinely exercised: T is a struct
// here, which is the whole point of the component. Anonymous namespace: this TU is linked alongside
// every other test file, so the type must not have external linkage.
namespace {
    struct Person {
        int id;
        AString name;
        bool operator==(const Person& rhs) const { return id == rhs.id; }
    };
} // namespace

class AComboboxTest : public testing::UITest {
public:
protected:
    class TestWindow : public AWindow {
    public:
        // Dimensions need the _dp suffix: the integral AMetric constructor is
        // AUI_ASSERTX(value == 0, ...), so bare ints throw.
        TestWindow() : AWindow("AComboboxTest", 400_dp, 300_dp) {}
    };

    _<TestWindow> mWindow;
    _<ACombobox<AString>> mCombo;
    _<ACombobox<Person>> mPersonCombo;

    void SetUp() override {
        UITest::SetUp();
        mWindow = _new<TestWindow>();
        // AStubWindowManager::drawFrame packs every window to its minimum before every frame, so the
        // 400x300 this window is constructed with arrives at the tests as a 76x38 strip. Nothing
        // that only reads the selection minds, and everything that lays the popup out does: the
        // popup is capped to the room the button leaves inside its parent, and a parent with no room
        // would cap the popup to nothing. Pinning the window to the size it was constructed with is
        // what a real window manager would hand it anyway.
        mWindow->setFixedSize({ 400, 300 });
        // AViewContainer has no variadic constructor -- Vertical is what every uitest uses.
        mWindow->setContents(Vertical {
            mCombo = _new<ACombobox<AString>>(
                AListModel<AString>::make({ "alpha", "beta", "gamma" })),
            mPersonCombo = _new<ACombobox<Person>>(),
        });
        // Order matters for a non-AString T: setModel renders the default selection straight away,
        // so the factory has to be in place first or viewFor trips its assertion.
        mPersonCombo->setViewFactory([](const Person& p) { return _new<ALabel>(p.name); });
        mPersonCombo->setModel(AListModel<Person>::make({ Person { 1, "alpha" }, Person { 2, "beta" } }));
        mWindow->show();
        uitest::frame();
    }

    // ACombobox cannot guess a sensible default predicate: turning an arbitrary T into searchable
    // text is the app's call, so the default is pass-through and every filtering test must install
    // one.
    void enableSubstringFilter() {
        mCombo->setFilterPredicate([](const AString& item, AStringView query) {
            return item.lowercase().contains(query.lowercase());
        });
    }

    // There is no AView::performClick(); the uitest DSL is the repo's way to push a real click
    // through the window, which is also what exercises ACombobox::onPointerReleased.
    void clickCombo() { By::type<ACombobox<AString>>().perform(click()); }

    // The label a combo box paints its selection into. updateText hands AButton::setText's
    // `setContents(Centered { label })` a declarative::Centered, and setContents takes that
    // container's children rather than the container itself, keeping its layout
    // (AViewContainerBase.cpp:589) -- so the button's one child is the label, laid out by the
    // AStackedLayout the Centered brought with it.
    _<AView> buttonLabel(const _<AView>& button) {
        auto container = _cast<AViewContainer>(button);
        if (!container || container->getViews().size() != 1) {
            return nullptr;
        }
        return container->getViews()[0];
    }

    // How far from its button's content edge a button's label sits, which is what separates a
    // left-aligned label from a centred one. AStackedLayout decides it: a view that expands
    // horizontally is placed at x = 0, one that does not at (width - finalWidth) / 2
    // (AStackedLayout.cpp:18-27). Zero therefore means left-aligned, and nothing else.
    int labelInset(const _<AView>& button) {
        auto label = buttonLabel(button);
        if (!label) {
            ADD_FAILURE() << "the button does not hold exactly one label";
            return -1;
        }
        return label->getPositionInWindow().x - button->getPositionInWindow().x - button->getPadding().left;
    }

    // The popup reveals itself with an ASizeAnimator, growing its scroll area out of a zero height,
    // so a row is not hittable until the reveal is done. uitest::frame() does not advance the clock
    // -- the animator is wall-clock driven -- so a test that clicks straight after opening the popup
    // races it. Poll the list up to its full height with a bounded budget instead.
    void waitForPopupReveal() {
        auto list = By::name(".combobox_list").one();
        ASSERT_NE(list, nullptr) << "there is no open popup to wait for";

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (list->getHeight() < list->getMinimumHeight()) {
            if (std::chrono::steady_clock::now() >= deadline) {
                ADD_FAILURE() << "the popup's list never finished its reveal animation";
                return;
            }
            uitest::frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        uitest::frame();

        // The popup's height is content height plus the .combobox_list container's own 4px padding and 2px
        // border, and that arithmetic only balances because of ass::Expanding { 1, 0}: expanding
        // vertically is what stops AScrollArea::getContentMinimumHeight from reporting 0 (it answers
        // 0 outright for a view that expands on that axis, AScrollArea.cpp:67) and swallowing the
        // inset. It has broken once already. So pin what actually breaks -- the last row fitting
        // inside the list:
        //
        //   ass::Expanding { 1, 0 } (current): list 64 tall, last row spans 42..62 -> fits
        //   ass::Expanding {}       (previous): list 60 tall, last row spans 42..62 -> clipped 2px
        //   ass::Expanding { 1, 1 } (tried):  list  4 tall, nothing fits at all
        //
        // Comparing the list against the window instead would prove nothing: without a filter field
        // the list is the popup's whole content, so its bottom always equals the window's bottom.
        //
        // It is also, since the cap landed, window-height dependent where it used not to be, and the
        // filter field added a second term to the same sum. The popup is capped to the room its
        // button leaves inside the parent minus whatever the field takes of it, so this holds because
        // the fixture's 300px window leaves far more than its three rows (~64px) plus a filter field
        // (~22px) need -- before the cap the popup's height bore no relation to the parent's at all.
        // Shrink the window far enough and this fails because the popup is capped, which is a
        // different fact from the expanding and inset arithmetic the assertion was written to guard;
        // the capped case has waitForPopupToSettle below, which is what the long list uses.
        //
        // Nothing here is specific to whether the field is on: these numbers are relative to the
        // list, which the field is a sibling of and not a parent of, so a field shifts the list's
        // position inside the popup without changing anything measured here.
        auto rows = By::name(".list-item").toVector();
        ASSERT_FALSE(rows.empty()) << "the popup lists no rows to fit";
        ASSERT_LE(rows.back()->getPositionInWindow().y + rows.back()->getSize().y,
                  list->getPositionInWindow().y + list->getSize().y)
            << "the popup is too short for its own rows; the last one is clipped";
    }

    // Waits out the same reveal for a list that is not expected to fit its own last row, and returns
    // the height it settled at. waitForPopupReveal() cannot serve that case on either count: it waits
    // for the list to reach its *minimum* height, which is the height of every row -- the very height
    // a capped popup refuses to reach -- and it then insists the last row is inside the list, which is
    // precisely what scrolling the overflow away means. So it waits for the height to stop moving
    // instead: the settled height is not knowable here, being the rows for a short list and the cap
    // for a long one.
    int waitForPopupToSettle() {
        auto list = By::name(".combobox_list").one();
        if (list == nullptr) {
            ADD_FAILURE() << "there is no open popup to wait for";
            return 0;
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        int previous = -1;
        int stable = 0;
        while (stable < 3) {
            // The `> 0` matters: the animator may not have produced its first frame yet, and three
            // frames at zero is not a settled list, it is a list that has not started.
            const int height = list->getHeight();
            if (height > 0 && height == previous) {
                ++stable;
            } else {
                previous = height;
                stable = 0;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                ADD_FAILURE() << "the popup's list never finished its reveal animation";
                break;
            }
            uitest::frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        uitest::frame();
        return previous;
    }

    void TearDown() override {
        mWindow->removeAllViews();
        AThread::processMessages();
        UITest::TearDown();
    }
};

TEST_F(AComboboxTest, SelectsFirstRowByDefault) {
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "alpha" });
    EXPECT_EQ(mCombo->getSelectionId(), 0);
}

TEST_F(AComboboxTest, SelectionFollowsTheValueNotTheIndex) {
    mCombo->setSelectionId(1);
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "beta" });

    // Reordering the model must not silently slide the selection onto a different item.
    mCombo->setModel(AListModel<AString>::make({ "gamma", "beta", "alpha" }));
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "beta" });
    EXPECT_EQ(mCombo->getSelectionId(), 1);
}

TEST_F(AComboboxTest, DroppedSelectionIsClearedRatherThanMislabelled) {
    mCombo->setSelectionId(2);                                    // "gamma"
    mCombo->setModel(AListModel<AString>::make({ "alpha", "beta" }));

    // "gamma" is gone. The selection must be empty, not whatever now sits at the old index --
    // this is exactly what every v0.19.6 dropdown got wrong.
    EXPECT_FALSE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelectionId(), -1);
}

TEST_F(AComboboxTest, NullModelIsLegal) {
    mCombo->setModel(nullptr);
    EXPECT_FALSE(mCombo->getSelected().hasValue());
    mCombo->setModel(AListModel<AString>::make({ "x" }));
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "x" });
}

// Counts rebuilds. The previous version of this fixture asserted on the rows' contents, which
// could not tell a live subscription from a dead one: rebuildRows always reads the *current* model,
// so any mutation of a swapped-out model left the rendering identical either way. Counting the seam
// makes the difference observable.
class CountingCombobox : public ACombobox<AString> {
public:
    int rebuilds = 0;

protected:
    void rebuildRows() override {
        ++rebuilds;
        ACombobox<AString>::rebuildRows();
    }
};

TEST_F(AComboboxTest, SwappedOutModelStopsDrivingTheWidget) {
    auto combo = _new<CountingCombobox>();
    auto oldModel = AListModel<AString>::make({ "alpha", "beta" });
    combo->setModel(oldModel);
    auto newModel = AListModel<AString>::make({ "one", "two", "three" });
    combo->setModel(newModel);

    // The popup has to be open, which is what this test was retargeted onto, not relaxed onto. A
    // closed combo box rebuilds nothing on a mutation any more -- there is no popup to be stale and
    // its rows are rebuilt on the way to opening -- so counting rebuilds with the popup shut counts
    // zero and proves nothing. Opened, the count is a live subscription again: a leaked binding to
    // oldModel moves it, an intact one does not.
    mWindow->addView(combo);
    uitest::frame();
    By::value(combo).perform(click());
    uitest::frame();
    ASSERT_TRUE(combo->isPopupOpen()) << "the popup did not open, so nothing below observes anything";

    const int afterOpen = combo->rebuilds;
    ASSERT_GT(afterOpen, 0) << "opening the popup rebuilt nothing, so this counter is not wired up";

    // push_back announces on dataInserted, so a still-bound combobox would react to this.
    oldModel->push_back("MUTATED");
    EXPECT_EQ(combo->rebuilds, afterOpen) << "the swapped-out model is still driving the widget";

    // ...and the model actually in use does reach it.
    newModel->push_back("four");
    EXPECT_GT(combo->rebuilds, afterOpen) << "the current model is not driving the widget";
}

// The other half of the same seam, and the one that has observable cost. A closed combo box is
// rebuilt on the way to opening anyway (onPointerReleased), so a model mutation while it is shut
// has nothing to refresh: rebuilding here would allocate an AComboboxRow and a content view per
// item, for a popup nobody can see, and on a 5k-item model that is 5k allocations per mutation to
// arrive at exactly the state the next click produces anyway.
//
// The assertion is on the closed case alone. The open case is what makes it discriminating, and it
// is the same measurement SwappedOutModelStopsDrivingTheWidget makes: a rebuild that does not happen
// with no popup open, and does happen with one, is the guard rather than a dead counter.
TEST_F(AComboboxTest, ModelMutationRebuildsRowsOnlyWhileThePopupIsOpen) {
    auto combo = _new<CountingCombobox>();
    auto model = AListModel<AString>::make({ "alpha", "beta", "gamma" });
    combo->setModel(model);
    mWindow->addView(combo);
    uitest::frame();

    const int beforeMutation = combo->rebuilds;
    model->push_back("delta");
    EXPECT_EQ(combo->rebuilds, beforeMutation)
        << "a closed combo box rebuilt its rows for a popup that does not exist";
    // The button still answers a mutation: it is the rows that are not rebuilt, not the widget.
    ASSERT_TRUE(combo->getSelected().hasValue()) << "the mutation dropped the selection it should not have";

    By::value(combo).perform(click());
    uitest::frame();
    ASSERT_TRUE(combo->isPopupOpen()) << "the popup did not open, so nothing below observes anything";

    const int afterOpen = combo->rebuilds;
    model->push_back("epsilon");
    EXPECT_GT(combo->rebuilds, afterOpen) << "an open popup's rows are not rebuilt on a mutation any more";
}

TEST_F(AComboboxTest, PopupOpensAndCloses) {
    EXPECT_FALSE(mCombo->isPopupOpen());
    clickCombo();
    uitest::frame();
    EXPECT_TRUE(mCombo->isPopupOpen());

    // Closed by a real pointer press on the combobox, not by the widget closing itself -- a
    // destroyWindow() here would only assert the negation of what the test itself just forced.
    //
    // A full click() does not work for this: the popup is created through createOverlappingSurface's
    // position-factory overload, which defaults closeOnClick to true, so ASurface::onPointerPressed
    // closes the surface on *press* (measured: isPopupOpen() is already false one frame after the
    // press, with zero .combobox_list views left). The release then finds no popup and falls into
    // ACombobox::onPointerReleased's opening branch, which builds a fresh one -- so click()ing the
    // button again leaves the popup open instead of closing it, and ACombobox's own close branch is
    // unreachable through a press on the parent window.
    By::type<ACombobox<AString>>().perform(mousePress());
    uitest::frame();
    EXPECT_FALSE(mCombo->isPopupOpen());
}

// Clicking the combo box while its popup is open must close it: the button is a toggle, and a
// dropdown that can only be dismissed by clicking away is not one a user would call working.
//
// This is the gesture PopupOpensAndCloses could not express -- mousePress() there stops at the press,
// which the framework has already handled. A full click() is the real thing, and it is where the
// defect lives: the press closes the surface, the destruction drains before the release, and the
// release then finds nothing open and builds a second popup.
TEST_F(AComboboxTest, ClickingTheComboWhileOpenClosesThePopup) {
    clickCombo();
    uitest::frame();
    ASSERT_TRUE(mCombo->isPopupOpen());

    clickCombo();
    uitest::frame();
    EXPECT_FALSE(mCombo->isPopupOpen());
}

TEST_F(AComboboxTest, ClickingAwayLeavesTheNextClickFreeToOpen) {
    clickCombo();
    uitest::frame();
    ASSERT_TRUE(mCombo->isPopupOpen());

    // A click anywhere else dismisses the popup, which is behaviour the framework owns and which
    // must survive: nothing in ACombobox runs, so the widget only finds out that its popup is gone
    // when the pointer next crosses the combo box.
    By::type<ACombobox<Person>>().perform(click());
    uitest::frame();
    EXPECT_FALSE(mCombo->isPopupOpen());

    By::type<ACombobox<AString>>().perform(pointerMove());
    uitest::frame();

    clickCombo();
    uitest::frame();
    EXPECT_TRUE(mCombo->isPopupOpen()) << "click-away must not cost the next click its open";
}

TEST_F(AComboboxTest, DestroyWindowClosesThePopup) {
    clickCombo();
    uitest::frame();
    ASSERT_TRUE(mCombo->isPopupOpen());

    // The imperative escape hatch is public API, so it is worth covering on its own.
    mCombo->destroyWindow();
    uitest::frame();
    EXPECT_FALSE(mCombo->isPopupOpen());
}

TEST_F(AComboboxTest, ClickingARowSelectsItAndClosesThePopup) {
    clickCombo();
    uitest::frame();
    waitForPopupReveal();

    By::text("beta").perform(click());
    uitest::frame();

    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "beta" });
    EXPECT_FALSE(mCombo->isPopupOpen());
}

TEST_F(AComboboxTest, FilteringHidesRowsWithoutMovingTheSelection) {
    mCombo->setSelectionId(2); // "gamma"
    enableSubstringFilter();

    clickCombo();
    uitest::frame();
    waitForPopupReveal();
    ASSERT_EQ(By::name(".list-item").toVector().size(), 3u);

    // Only "beta" survives this query, so the selected row is filtered out of sight. The selection
    // must stay where it was rather than slide onto the row that is still visible.
    mCombo->setFilter("b");
    uitest::frame();

    EXPECT_EQ(By::name(".list-item").toVector().size(), 1u);
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "gamma" });
    EXPECT_EQ(mCombo->getSelectionId(), -1); // hidden: not among the visible rows
}

TEST_F(AComboboxTest, SelectionSignalsAndPropertyRoundTrip) {
    AOptional<AString> selected;
    int selectionId = 0;
    int selectedNotifications = 0;
    int selectionNotifications = 0;
    AObject::connect(mCombo->selectedChanged, &gSink,
                     [&](const AOptional<AString>& value) { selected = value; ++selectedNotifications; });
    AObject::connect(mCombo->selectionChanged, &gSink, [&](int id) { selectionId = id; ++selectionNotifications; });

    mCombo->selected() = AOptional<AString>(AString { "gamma" });

    // One selection change announces on both signals: selectedChanged carries the value,
    // selectionChanged the index.
    EXPECT_EQ(selectedNotifications, 1);
    EXPECT_EQ(selectionNotifications, 1);
    ASSERT_TRUE(selected.hasValue());
    EXPECT_EQ(selected.value(), AString { "gamma" });
    EXPECT_EQ(selectionId, 2);
    EXPECT_EQ(mCombo->getSelected().value(), AString { "gamma" });
    EXPECT_EQ(mCombo->getSelectionId(), 2);

    // Re-setting the same value must stay silent, or a binding would feed itself.
    mCombo->selected() = AOptional<AString>(AString { "gamma" });
    EXPECT_EQ(selectedNotifications, 1);
    EXPECT_EQ(selectionNotifications, 1);

    mCombo->selectionId() = 0;
    EXPECT_EQ(selectedNotifications, 2);
    EXPECT_EQ(selectionNotifications, 2);
    EXPECT_EQ(selectionId, 0);
    EXPECT_EQ(mCombo->getSelectionId(), 0);
    EXPECT_EQ(mCombo->getSelected().value(), AString { "alpha" });
}

// The two halves of examples/7guis/flight_booker's only selectionId() binding, which is the only
// two-way binder over this widget in the tree. Examples are not configured in this build tree (and
// that one's ctre.hpp is absent), so nothing compiled it and nothing tested it -- and each half on
// its own looks right: a one-way connect in either direction only shows up broken once the other
// half is exercised. Both directions are asserted here so that gap stays shut.
//
// The model side is an AProperty<bool> in the manner of UIDataBindingTest: AUI_REACT subscribes to
// property access, so a plain local would never announce and the model -> widget half would pass for
// the wrong reason.
namespace {
    struct FlightBookerState {
        AProperty<bool> isReturnFlight { false };
    };
} // namespace

TEST_F(AComboboxTest, SelectionIdBindsBothWays) {
    auto state = aui::ptr::manage_shared(new FlightBookerState);
    auto combo = _new<ADropdownList>(AListModel<AString>::make({ "one-way flight", "return flight" }));

    AObject::connect(AUI_REACT(state->isReturnFlight ? 1 : 0), combo->selectionId());
    AObject::connect(combo->selectionId().changed, &gSink,
                     [&](int id) { state->isReturnFlight = id == 1; });

    // widget -> model: the user picks the second row.
    combo->selectionId() = 1;
    EXPECT_TRUE(state->isReturnFlight);
    ASSERT_EQ(combo->getSelected().value(), AString { "return flight" });

    // model -> widget: the model goes back, and the button has to follow.
    state->isReturnFlight = false;
    EXPECT_FALSE(combo->isPopupOpen());
    EXPECT_EQ(combo->getSelectedId(), 0);
    ASSERT_TRUE(combo->getSelected().hasValue());
    EXPECT_EQ(combo->getSelected().value(), AString { "one-way flight" });

    // ...without the model and the widget talking each other in circles: pushing row 1 through the
    // property also goes back out to the model, which is a write of the value it already holds.
    combo->selectionId() = 1;
    EXPECT_TRUE(state->isReturnFlight);
    EXPECT_EQ(combo->getSelectedId(), 1);
}

// Both ways this widget applies one policy -- the selected item is not in the model any more, so
// nothing is selected -- have to say so. onModelMutated routed through setSelected and announced;
// setModel wrote mSelected directly and announced nothing, so a binding that lost its selection to a
// mutation was told, and one that lost the same selection to a replacement was left holding a value
// the widget no longer had.
//
// The binding is SelectionIdBindsBothWays' own, verbatim, because being told is only half of what a
// binding is for: the model side has to learn the change too, or it goes on asserting a row the
// widget has dropped.
TEST_F(AComboboxTest, SelectionDroppedByModelReplacementIsAnnounced) {
    auto state = aui::ptr::manage_shared(new FlightBookerState);
    auto model = AListModel<AString>::make({ "one-way flight", "return flight" });
    auto combo = _new<ADropdownList>(model);

    AObject::connect(AUI_REACT(state->isReturnFlight ? 1 : 0), combo->selectionId());
    AObject::connect(combo->selectionId().changed, &gSink,
                     [&](int id) { state->isReturnFlight = id == 1; });

    combo->selectionId() = 1;
    ASSERT_TRUE(state->isReturnFlight) << "the binding is not carrying the selection either way";
    ASSERT_EQ(combo->getSelected().value(), AString { "return flight" });

    int announcements = 0;
    AObject::connect(combo->selectionChanged, &gSink, [&](int) { ++announcements; });

    // Dropped by a mutation: the item is taken out from under the widget.
    announcements = 0;
    model->removeItem(AListModelIndex(1));
    EXPECT_FALSE(state->isReturnFlight) << "the mutation dropped the selection without telling the binding";
    EXPECT_EQ(announcements, 1) << "the drop";

    // Dropped by a replacement: the model is swapped for one that no longer holds the item.
    model->push_back("return flight");
    combo->setSelectionId(1);
    ASSERT_TRUE(state->isReturnFlight) << "the binding did not hear the selection being taken again";
    ASSERT_EQ(combo->getSelected().value(), AString { "return flight" });

    announcements = 0;
    combo->setModel(AListModel<AString>::make({ "one-way flight", "another flight" }));
    EXPECT_FALSE(state->isReturnFlight) << "the replacement dropped the selection without telling the binding";
    EXPECT_EQ(announcements, 1) << "the drop";

    // Exactly one each, and nothing about the value is left hanging: the widget does not drift onto
    // whatever now sits where the dropped item used to be.
    EXPECT_FALSE(combo->getSelected().hasValue());
    EXPECT_EQ(combo->getSelectedId(), -1);

    // What is deliberately not asserted is the write-back the announcement provokes, and the reason
    // is the property binding's own loop guard rather than anything APropertyPrecomputed does.
    // AObject::connect(expression, property) (AObject.h:121) resolves its destination through
    // property.assignment() into aui::detail::property::makeAssignment, whose invocable returns
    // early while the destination property's own `changed` is mid-emission
    // (aui.core/src/AUI/Common/detail/property.h:32) -- the guard against a bidirectional connection
    // feeding itself. selectionId()'s `changed` is the combobox's selectionChanged member
    // (ACombobox.h:198), which is exactly the signal being emitted as the drop announces, so the row
    // the binding would push back goes nowhere until the binding next writes. Not the precomputed:
    // APropertyPrecomputed::invalidate tests *its own* signal, a different object that is not
    // mid-emission here -- it emits, and the value dies at the guard one level downstream. That is
    // equally true of both halves -- it is what the mutation path has always done -- and it is a
    // framework rule rather than this widget's.
    //
    // Which makes the *shape* of the binding load-bearing, not incidental. Everything above rests on
    // this guard and on nothing else: written as a lambda instead --
    //   AObject::connect(AUI_REACT(state->isReturnFlight ? 1 : 0), [&](int id) { combo->setSelectionId(id); });
    // -- the slot has no guard, so the drop announces -1, the handler clears isReturnFlight, the
    // expression emits 0, setSelectionId(0) runs, and the combobox re-selects row 0 on the spot. The
    // announcement would then contradict itself and undo the very drift this widget exists to prevent
    // -- and this commit widened the exposure from one path (mutation) to two (mutation and
    // replacement). A binding that has to survive a drop therefore has to be the property form.
    //
    // What this last step does show is the cost of *not* announcing. The binding still believes it
    // is looking at a return flight, so writing true is writing the value it already holds, nothing
    // moves, and a widget that dropped its selection silently leaves the binding stuck on a row that
    // is not there with no way out but for it to be told.
    state->isReturnFlight = true;
    ASSERT_EQ(combo->getSelectedId(), 1);
    EXPECT_EQ(combo->getSelected().value(), AString { "another flight" });
    EXPECT_TRUE(state->isReturnFlight);
}

// 60 rows is a little over a thousand pixels of list inside a 300 px window. That is the case the
// scroll area was chosen for and never got: the popup used to be exactly as tall as its rows, so the
// viewport was exactly the content, no scrollbar could appear, and every row past the bottom of the
// screen was simply gone.
TEST_F(AComboboxTest, LongListIsCappedAndStillScrolls) {
    auto model = _new<AListModel<AString>>();
    for (int i = 0; i < 60; ++i) {
        model->push_back("row-"_format(i));
    }
    mCombo->setModel(model);

    clickCombo();
    uitest::frame();

    auto listView = By::name(".combobox_list").one();
    ASSERT_NE(listView, nullptr) << "there is no open popup";
    auto list = _cast<AScrollArea>(listView);
    ASSERT_NE(list, nullptr) << "the popup's list is no longer a scroll area";
    ASSERT_GT(waitForPopupToSettle(), 0);

    // Capped, not unbounded. The bound is the room the button leaves inside its own window, so a
    // popup taller than the window it belongs to is by definition not capped.
    EXPECT_LT(list->getHeight(), mWindow->getHeight())
        << "the popup grew past the window it belongs to instead of being capped";

    // ...and capped means scrolling: the scroll area has a viewport smaller than its content, which
    // is the first time that has ever been true here.
    ASSERT_GT(list->verticalScrollbar()->getMaxScroll(), 0u)
        << "the list is capped but reports nothing to scroll: the rows below the cap are unreachable";

    auto rows = By::name(".list-item").toVector();
    ASSERT_EQ(rows.size(), 60u) << "the rows are not all built, capped or not";

    // The content is still reachable: scrolled to its end, the last row is inside the list again.
    list->setScrollY(list->verticalScrollbar()->getMaxScroll());
    uitest::frame();
    EXPECT_LE(rows.back()->getPositionInWindow().y + rows.back()->getSize().y,
              list->getPositionInWindow().y + list->getSize().y)
        << "the last row cannot be scrolled into view";

    mCombo->destroyWindow();
    uitest::frame();
}

TEST_F(AComboboxTest, FilterSignalAndPropertyRoundTrip) {
    AString notified;
    int notifications = 0;
    AObject::connect(mCombo->filterChanged, &gSink, [&](const AString& query) { notified = query; ++notifications; });

    mCombo->filter() = AString { "be" };

    EXPECT_EQ(notifications, 1);
    EXPECT_EQ(notified, AString { "be" });
    EXPECT_EQ(mCombo->getFilter(), AString { "be" });

    mCombo->filter() = AString { "be" };
    EXPECT_EQ(notifications, 1);
}

TEST_F(AComboboxTest, GenericItemTypeRoundTripsByValue) {
    mPersonCombo->setSelectionId(1);
    ASSERT_TRUE(mPersonCombo->getSelected().hasValue());
    EXPECT_EQ(mPersonCombo->getSelected().value().id, 2);
}

// The button's label, for an item type that is not AString.
//
// The stylesheet rule that makes a combo box's button label left-aligning was keyed on
// t<ADropdownList>(), which is ACombobox<AString> and nothing else, so ACombobox<Person> -- the
// case this widget exists for -- matched nothing at all. Its label was neither expanding nor
// left-aligned, and since updateText paints it into a declarative::Centered, it came out centred
// while the dropdown it replaced rendered left-aligned. Nothing asserts, nothing fails: the widget
// simply looks subtly wrong, and only for the item types nobody had written yet.
//
// The rule is keyed on the .combobox class every ACombobox carries, which survives the next
// ACombobox<Foo>. Keying it on the alias would have kept this exactly one instantiation wide.
TEST_F(AComboboxTest, ButtonLabelIsLeftAlignedForANonStringItemType) {
    class W : public AWindow {
    public:
        _<ACombobox<Person>> personCombo;
        _<AButton> plainButton;

        W() : AWindow("label-alignment", 400_dp, 300_dp) {
            setFixedSize({ 400, 300 });
            personCombo = _new<ACombobox<Person>>();
            // The control: laid out by the same declarative::Centered, matching no combobox rule,
            // and therefore placed exactly where a non-AString combo box's label used to land.
            plainButton = _new<AButton>(AString { "alpha" });
            setContents(Vertical { personCombo, plainButton });
            // Order matters for a non-AString T: setModel renders the default selection straight
            // away, so the factory has to be in place first or viewFor trips its assertion.
            personCombo->setViewFactory([](const Person& p) { return _new<ALabel>(p.name); });
            personCombo->setModel(AListModel<Person>::make({ Person { 1, "alpha" } }));
        }
    };

    auto window = _new<W>();
    window->show();
    uitest::frame();

    // ...first, that the measurement can tell a centred label from a left-aligned one at all. An
    // inset of zero is only evidence if zero is not what every label here reads.
    ASSERT_GT(labelInset(window->plainButton), 0)
        << "the control label is not centred, so this fixture cannot tell the two apart";

    EXPECT_EQ(labelInset(window->personCombo), 0)
        << "a combo box whose item type is not AString renders its button label centred";

    window->removeAllViews();
    AThread::processMessages();
}

// The filter field. Everything below is opt-in: with setFilterEnabled left alone the popup holds the
// list and nothing else, which is what the tests above already assert.
//
// PopupWindowSpy exists for the two that have to look at the popup from the outside. comboWindow() is
// protected, and the surface it hands back is a view inside the popup's own AWindow rather than the
// window: what has a position on the screen is the window, which is the surface's parent. Anonymous
// namespace, like Person above -- this TU is linked alongside every other test file.
namespace {
    class PopupWindowSpy : public ACombobox<AString> {
    public:
        AWindow* popupWindow() {
            if (auto surface = comboWindow()) {
                return dynamic_cast<AWindow*>(surface->getParent());
            }
            return nullptr;
        }
    };

    // Presses and releases `view` the way ViewActionClick does, but without the uitest::frame()
    // that would follow it, so a caller can read the popup window between the release and the next
    // frame.
    //
    // The frame has to be left out on purpose, and its absence is the whole reason the popup's
    // placement used to look unobservable. AStubWindowManager::drawFrame packs every window to its
    // minimum, and packing an AWindow is not size-only: AView::pack calls setSize, which for a
    // window resolves to AWindow::setSize, which calls setGeometry(getWindowPosition()...), and
    // AWindow::getWindowPosition() answers {0, 0} for a window with no native handle
    // (win32/AWindowsImpl.cpp:464). Under the stub manager a window never gets one, so the first
    // frame after the popup is shown overwrites the position createOverlappingSurfaceImpl stored.
    // Before that frame the position is exactly what the factory produced and the popup was shown
    // at; after it, the value is {0, 0} no matter what the factory returned. The matcher DSL can
    // only ever read the second.
    void clickLeavingFrame(const _<AView>& view) {
        const auto coords = view->getPositionInWindow() + view->getSize() / 2;
        auto window = view->getWindow();
        AInput::overrideStateForTesting(AInput::LBUTTON, true);
        window->onPointerPressed({ coords, APointerIndex::button(AInput::LBUTTON) });
        uitest::frame();
        AInput::overrideStateForTesting(AInput::LBUTTON, false);
        window->onPointerReleased({ coords, APointerIndex::button(AInput::LBUTTON) });
    }

    // Puts the global stylesheet back on every exit path, a failed ASSERT included. A rule left
    // installed would leak into every test that follows it in the same binary.
    class StylesheetGuard {
    public:
        StylesheetGuard() : mSaved(AStylesheet::global().getRules()) {}
        ~StylesheetGuard() { AStylesheet::global().setRules(mSaved); }

    private:
        AVector<ass::Rule> mSaved;
    };
} // namespace

TEST_F(AComboboxTest, FilterFieldAppearsOnlyWhenEnabled) {
    clickCombo();
    uitest::frame();
    EXPECT_EQ(By::type<ATextField>().toVector().size(), 0u);
    mCombo->destroyWindow();
    uitest::frame();

    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();
    EXPECT_EQ(By::type<ATextField>().toVector().size(), 1u);
}

TEST_F(AComboboxTest, WithoutAPredicateTheFilterIsPassThrough) {
    mCombo->setFilterEnabled(true);
    mCombo->setFilter("bet");
    clickCombo();
    uitest::frame();

    // ACombobox cannot turn an arbitrary T into searchable text, so the default predicate shows
    // everything. This is the documented default, not a bug.
    EXPECT_EQ(By::name(".list-item").toVector().size(), 3u);
}

TEST_F(AComboboxTest, TypingInTheFilterNarrowsTheRows) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();

    auto field = _cast<ATextField>(By::type<ATextField>().one());
    ASSERT_NE(field, nullptr);
    field->setText("bet");
    uitest::frame();

    EXPECT_EQ(By::name(".list-item").toVector().size(), 1u);
    EXPECT_EQ(By::text("beta").toVector().size(), 1u);
    // Scoped to the rows on purpose. By::text() is asked of every view in every window, and this
    // fixture keeps a second combo box whose button reads "alpha" too -- mPersonCombo selects
    // Person{ 1, "alpha" } -- so By::text("alpha") is 2 whether or not a row survived the filter.
    EXPECT_EQ((By::name(".list-item").allChildren() & By::text("alpha")).toVector().size(), 0u);
}

TEST_F(AComboboxTest, TypingIntoTheFieldItselfNarrowsTheRows) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();

    // The same narrowing, by the route a user actually takes: keystrokes into the focused field.
    // A programmatic setText and a keystroke are two different announcements (textChanged against
    // textChanging) and the field has to answer both -- a filter that only narrows when nobody is
    // typing would look exactly right in a test and dead in the app.
    By::name(".combobox_filter").perform(type("bet"));
    uitest::frame();

    EXPECT_EQ(mCombo->getFilter(), AString { "bet" });
    EXPECT_EQ(By::name(".list-item").toVector().size(), 1u);
    EXPECT_EQ(By::text("beta").toVector().size(), 1u);
}

TEST_F(AComboboxTest, TheFilterFieldIsClickableAndDoesNotCloseThePopup) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();
    ASSERT_TRUE(mCombo->isPopupOpen());

    // The popup is its own AWindow with its own (empty) list of close-on-click surfaces, so a press
    // inside it is handled there and never reaches the parent's close loop. That is load-bearing for
    // a field: a widget inside a popup that the popup cannot survive a press on is not a field, and
    // nothing about the type of T or the predicate would tell you -- the popup would simply close the
    // moment it was touched, and the query could never be typed.
    By::name(".combobox_filter").perform(click());
    uitest::frame();
    EXPECT_TRUE(mCombo->isPopupOpen()) << "clicking the filter field closed the popup";
    EXPECT_TRUE(By::name(".combobox_filter").one()->hasFocus()) << "the filter field did not take the focus";

    // ...and focus leaving the field is not a different story: AViewContainer::onPointerPressed runs
    // after the close loop, so the press that steals the focus is the same press that would have
    // closed it.
    By::name(".combobox_filter").perform(type("bet"));
    uitest::frame();
    EXPECT_TRUE(mCombo->isPopupOpen()) << "typing into the filter field closed the popup";

    // A row is still selectable while the field holds the focus -- the field being focused is not a
    // reason the rows stopped answering.
    By::text("beta").perform(click());
    uitest::frame();
    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "beta" });
    EXPECT_FALSE(mCombo->isPopupOpen());
}

TEST_F(AComboboxTest, FilteringDoesNotChangeTheSelection) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    mCombo->setSelectionId(0); // "alpha"
    clickCombo();
    uitest::frame();

    auto field = _cast<ATextField>(By::type<ATextField>().one());
    ASSERT_NE(field, nullptr);
    field->setText("bet"); // hides the selected row
    uitest::frame();

    ASSERT_TRUE(mCombo->getSelected().hasValue());
    EXPECT_EQ(mCombo->getSelected().value(), AString { "alpha" });
    EXPECT_EQ(mCombo->getSelectionId(), -1); // not visible, so no row highlighted
}

// The query the field is holding when the popup closes must not come back with it. The brief this
// was specified from set the query with setFilter and then asserted it survived the opening click,
// which is the opposite of the invariant the same brief states -- opening a popup clears the query.
// So the query is put there the way a user leaves it, by typing, and what is asserted is the reopen.
//
// Which is the harder direction anyway: a query written before the popup existed has been cleared by
// the time anyone could look at the list it was meant to narrow.
TEST_F(AComboboxTest, FilterIsClearedWhenThePopupReopens) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();

    By::name(".combobox_filter").perform(type("bet"));
    uitest::frame();
    ASSERT_EQ(mCombo->getFilter(), AString { "bet" });
    EXPECT_EQ(By::name(".list-item").toVector().size(), 1u);

    mCombo->destroyWindow();
    uitest::frame();
    clickCombo();
    uitest::frame();

    EXPECT_EQ(mCombo->getFilter(), AString { "" });
    EXPECT_EQ(By::name(".list-item").toVector().size(), 3u);
}

// The counterpart of the clear above, and the invariant that makes it a clear rather than an edit:
// opening a popup is not an author edit, so it must not be announced as one. Without this, a two-way
// binding over filter() would write the emptied query back into whatever it is bound to every time
// the combo box was clicked -- the same loop-guard hazard setSelected documents, one level down.
TEST_F(AComboboxTest, OpeningThePopupClearsTheFilterWithoutAnnouncingIt) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    mCombo->setFilter("bet");

    int announcements = 0;
    AObject::connect(mCombo->filterChanged, &gSink, [&](const AString&) { ++announcements; });

    clickCombo();
    uitest::frame();

    EXPECT_EQ(mCombo->getFilter(), AString { "" }) << "the stale query survived the popup opening";
    EXPECT_EQ(announcements, 0) << "opening a popup announced a filter change, which is an author edit's news";
    EXPECT_EQ(By::name(".list-item").toVector().size(), 3u);

    // Clearing the query the user cannot see is still not an edit, and turning the field off is --
    // that one *is* the author changing the widget, so it has to be announced or a binding over
    // filter() would keep the query it was told about.
    mCombo->destroyWindow();
    uitest::frame();
    mCombo->setFilter("bet");
    mCombo->setFilterEnabled(false);
    EXPECT_EQ(announcements, 2) << "the two author edits were not announced";
}

TEST_F(AComboboxTest, DisablingTheFilterRestoresEveryRow) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    mCombo->setFilter("bet");
    mCombo->setFilterEnabled(false);
    clickCombo();
    uitest::frame();
    EXPECT_EQ(By::name(".list-item").toVector().size(), 3u);
}

TEST_F(AComboboxTest, IsFilterEnabledReportsTheSetting) {
    EXPECT_FALSE(mCombo->isFilterEnabled()) << "the filter is opt-in";
    mCombo->setFilterEnabled(true);
    EXPECT_TRUE(mCombo->isFilterEnabled());
    mCombo->setFilterEnabled(false);
    EXPECT_FALSE(mCombo->isFilterEnabled());
}

// The clear in DisablingTheFilterRestoresEveryRow happens before the popup is built, so it cannot
// speak for what the field does while a popup is already open. The popup is a window of its own and
// takes no instruction from the widget, so it survives the switch and keeps a field -- holding the
// text it had, because nothing told it otherwise. What the field must not do is vote: a keystroke
// into a filter the author just switched off would put the query straight back.
TEST_F(AComboboxTest, DisablingTheFilterSilencesAnOpenPopupField) {
    enableSubstringFilter();
    mCombo->setFilterEnabled(true);
    clickCombo();
    uitest::frame();

    By::name(".combobox_filter").perform(type("bet"));
    uitest::frame();
    ASSERT_EQ(mCombo->getFilter(), AString { "bet" });
    ASSERT_EQ(By::name(".list-item").toVector().size(), 1u);

    mCombo->setFilterEnabled(false);
    EXPECT_EQ(mCombo->getFilter(), AString { "" }) << "switching the filter off did not clear the query";
    EXPECT_TRUE(mCombo->isPopupOpen()) << "the open popup did not survive the switch";

    By::name(".combobox_filter").perform(type("a"));
    uitest::frame();

    EXPECT_EQ(mCombo->getFilter(), AString { "" })
        << "the open popup's field put a query back into a filter that is off";
    EXPECT_EQ(By::name(".list-item").toVector().size(), 3u)
        << "the rows narrowed against a filter that is off";
}

// 60 rows is a little over a thousand pixels of list. With the field on top of it the popup is taller
// than the room the combo box leaves inside its window unless the ceiling knows the field is there:
// the ceiling is read off the list, and the field is added on top of the result, so a cap that does
// not subtract it puts the bottom of the popup past the bottom edge of the window -- 22px of it, in
// this fixture -- and the rows below the cap are then unreachable rather than merely capped.
TEST_F(AComboboxTest, TheFilterFieldIsPaidForOutOfTheCappedRoom) {
    auto model = _new<AListModel<AString>>();
    for (int i = 0; i < 60; ++i) {
        model->push_back("row-"_format(i));
    }

    auto combo = _new<PopupWindowSpy>();
    combo->setModel(model);
    combo->setFilterEnabled(true);
    auto window = _new<TestWindow>();
    window->setFixedSize({ 400, 300 });
    window->setContents(Vertical { combo });
    window->show();
    uitest::frame();

    By::type<PopupWindowSpy>().perform(click());
    uitest::frame();

    auto popupWindow = combo->popupWindow();
    ASSERT_NE(popupWindow, nullptr) << "the popup is not open";
    const int top = combo->getPositionInWindow().y + combo->getHeight();
    // The 2 is the popup's own bias, which LongListIsCappedAndStillScrolls documents and which this
    // does not try to fix: it is below the cap, not above it.
    EXPECT_LE(top + popupWindow->getSize().y, window->getHeight() + 2)
        << "the filter field pushed the popup out of the window it belongs to";

    auto field = By::name(".combobox_filter").one();
    auto listView = By::name(".combobox_list").one();
    ASSERT_NE(field, nullptr);
    ASSERT_NE(listView, nullptr);
    EXPECT_GT(field->getSize().y, 0) << "the field was squeezed out of a popup too small to hold it";
    EXPECT_LE(field->getPositionInWindow().y + field->getSize().y, listView->getPositionInWindow().y + 1)
        << "the field is not above the list it filters";

    auto list = _cast<AScrollArea>(listView);
    ASSERT_NE(list, nullptr);
    EXPECT_GT(list->verticalScrollbar()->getMaxScroll(), 0u)
        << "the list is capped but reports nothing to scroll: taking the field's share left the rows "
           "no room to be reached in";

    window->removeAllViews();
    AThread::processMessages();
}

// The second placement the factory offers is the only one a combo box off the top edge of its window
// ever gets, and it used to be unreachable by construction: "above" was comboBoxPos.y - popupHeight +
// 1 against "below"'s comboBoxPos.y + height, a strictly smaller y for every popupHeight >= 2, so
// whenever the first attempt was rejected for being off the top the second was rejected for the same
// reason, and createOverlappingSurface gave up with the rejected negative coordinate still stored --
// a popup created 165px above a window it belongs to, on top of a ceiling that had gone negative
// because "the room below" for a combo box that is itself off the top is no room at all. A filter
// field makes both halves worse -- the popup it adds to is taller, so the subtraction that pushes it
// off the top grows with the field -- which is why this is fixed here rather than left for whoever
// adds a field next.
//
// What is asserted is the ceiling, not the placement coordinate. The popup's position is not
// observable here: the position is correct when AWindow::show() runs, and reads back as (0, 0) from
// the next uitest::frame() onwards, so an assertion on it would pass whatever the factory returned.
// The clamp on attempt 1 is therefore argued from the framework's own reference overload
// (ASurface.h:253, glm::clamp against the parent's size) rather than asserted, and this test pins the
// half that *is* observable -- a ceiling measured on the side the popup actually opens towards.
TEST_F(AComboboxTest, TheCeilingFollowsTheSideThePopupOpensTowards) {
    // 60 rows, so the popup is capped whichever way it opens and the only thing left that can differ
    // is which side the ceiling was measured from.
    auto model = _new<AListModel<AString>>();
    for (int i = 0; i < 60; ++i) {
        model->push_back("row-"_format(i));
    }

    auto combo = _new<PopupWindowSpy>();
    combo->setModel(model);
    auto window = _new<TestWindow>();
    window->setFixedSize({ 400, 300 });
    combo AUI_OVERRIDE_STYLE {
        ass::Margin { -40_px },
    };
    window->setContents(Vertical { combo });
    window->show();
    uitest::frame();

    ASSERT_LT(combo->getPositionInWindow().y + combo->getHeight(), 0)
        << "the fixture did not put the combo box off the top edge, so this asserts nothing";

    By::type<PopupWindowSpy>().perform(click());
    uitest::frame();
    ASSERT_TRUE(combo->isPopupOpen()) << "the popup did not open at all";

    auto popupWindow = combo->popupWindow();
    ASSERT_NE(popupWindow, nullptr);

    // Opening upwards, the ceiling is the window itself: the position factory clamps the popup into
    // the window, so the window is the only bound available before the height is known. The room
    // below is not a substitute -- for a combo box off the top it is zero, and a zero ceiling is a
    // popup with no rows in it at all.
    EXPECT_EQ(popupWindow->getSize().y, window->getHeight())
        << "the popup's ceiling was measured on the side it does not open towards";

    window->removeAllViews();
    AThread::processMessages();
}

// The filter field's height is read off a field that has no ancestry yet, and read again through
// the popup's minimum once it has one. The two readings are not the same number, and the ceiling
// is arithmetic over both: the list's MaxSize has this reading subtracted from it, so a field that
// measures larger once it is inside the popup hands the popup room it does not have, and the popup
// leaves its parent by exactly the difference.
//
// Nothing exotic is needed to make the two disagree. A rule that needs a window above the view
// cannot match a view that has no parent, which is what this field is when its minimum is read;
// adding it to the popup's content is a view-graph change, and that invalidates the styles, so the
// same field resolves again with the popup window above it and comes out larger.
TEST_F(AComboboxTest, AFilterFieldThatGrowsInContextCannotPushThePopupOutOfTheParent) {
    StylesheetGuard guard;
    AStylesheet::global().addRule(ass::Rule{ ass::AAssSelector(ass::t<AWindow>() >> ass::t<ATextField>()),
                                             ass::MinSize { 100_dp, 90_dp } });

    auto model = _new<AListModel<AString>>();
    for (int i = 0; i < 60; ++i) {
        model->push_back("row-"_format(i));
    }
    auto combo = _new<PopupWindowSpy>();
    combo->setModel(model);
    combo->setFilterEnabled(true);
    auto window = _new<TestWindow>();
    window->setFixedSize({ 400, 300 });
    window->setContents(Vertical { combo });
    window->show();
    uitest::frame();

    const int top = combo->getPositionInWindow().y + combo->getHeight();
    By::type<PopupWindowSpy>().perform(click());
    uitest::frame();

    auto popupWindow = combo->popupWindow();
    ASSERT_NE(popupWindow, nullptr) << "the popup is not open";
    // The two readings do not diverge when the popup is created -- the height above is computed
    // before the field is given a parent -- but on the next layout pass, so waiting for the reveal
    // to settle is what gives the difference time to become visible.
    waitForPopupToSettle();

    auto field = By::name(".combobox_filter").one();
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->getMinimumHeight(), 90) << "the fixture did not make the field grow in context, so this "
                                                 "asserts nothing about the case it exists for";

    // The 2 is the popup's own bias, which TheFilterFieldIsPaidForOutOfTheCappedRoom documents.
    EXPECT_LE(top + popupWindow->getSize().y, window->getHeight() + 2)
        << "a filter field that measures larger once it is in a window pushed the popup out of the "
           "window it belongs to";

    window->removeAllViews();
    AThread::processMessages();
}

// The placement, in the direction that does not need clamping, read where it is still readable --
// see clickLeavingFrame for why that is before the next frame and not after it.
//
// This is the half of the factory the clamp does not touch, and it is pinned here so that the pair
// is covered: a test that only asserts the clamped case says nothing about which branch ran.
TEST_F(AComboboxTest, ThePopupOpensBelowTheComboBoxWhenThereIsRoomUnderIt) {
    auto combo = _new<PopupWindowSpy>();
    combo->setModel(AListModel<AString>::make({ "alpha", "beta", "gamma" }));
    auto window = _new<TestWindow>();
    window->setFixedSize({ 400, 300 });
    window->setContents(Vertical { combo });
    window->show();
    uitest::frame();

    const glm::ivec2 expected = combo->getPositionInWindow() + glm::ivec2(0, combo->getHeight());
    clickLeavingFrame(combo);

    auto popupWindow = combo->popupWindow();
    ASSERT_NE(popupWindow, nullptr) << "the popup is not open";
    EXPECT_EQ(popupWindow->getPosition(), expected) << "the popup did not open below the combo box";

    uitest::frame();
    window->removeAllViews();
    AThread::processMessages();
}

// The placement in the direction that does, including the case where the clamp is the only thing
// stopping the popup from being rejected outright.
//
// The unclamped second placement is `comboBoxPos - {0, popupHeight - 1}`, whose y is negative for
// every combo box that gets as far as it -- being off the top is exactly what rejected the first
// placement. An unclamped factory therefore rejects both, and createOverlappingSurface opens the
// popup at its {0, 0} fallback. So a test that only watches the y, which the clamp always pulls to
// 0, cannot tell the clamp from the fallback: both read (0, 0). The x is what tells them apart, and
// that is why this fixture also puts the combo box hard against the right edge of its window.
TEST_F(AComboboxTest, ThePopupClampsItselfIntoTheWindowWhenItHasToOpenAbove) {
    auto combo = _new<PopupWindowSpy>();
    combo->setModel(AListModel<AString>::make({ "alpha", "beta", "gamma" }));
    auto window = _new<TestWindow>();
    window->setFixedSize({ 400, 300 });
    // top -40 puts it off the top edge, which is what forces the second placement; left 420 puts it
    // past the right edge, so the second placement's x has to be clamped as well. Four numbers
    // rather than a named constant: the point of the fixture is the geometry, and a reader should
    // be able to see it without running anything.
    combo AUI_OVERRIDE_STYLE {
        ass::Margin { -40_px, 0_px, 0_px, 420_px },
    };
    window->setContents(Vertical { combo });
    window->show();
    uitest::frame();

    const glm::ivec2 comboPos = combo->getPositionInWindow();
    ASSERT_LT(comboPos.y + combo->getHeight(), 0) << "the fixture did not put the combo box off the top edge";
    ASSERT_GT(comboPos.x, window->getSize().x) << "the fixture did not put it past the right edge";

    clickLeavingFrame(combo);

    auto popupWindow = combo->popupWindow();
    ASSERT_NE(popupWindow, nullptr) << "the popup is not open";

    const glm::ivec2 unclamped = comboPos - glm::ivec2(0, popupWindow->getSize().y - 1);
    ASSERT_LT(unclamped.y, 0) << "the unclamped placement would not have been rejected, so the clamp is not what "
                                "put the popup where it is";
    ASSERT_NE(unclamped, glm::ivec2(0, 0)) << "the unclamped placement is indistinguishable from the fallback";

    // Flush with the window's top edge, and pulled back to its right edge rather than sitting at
    // the x it was handed -- which is the assertion that would fail with the clamp removed.
    EXPECT_EQ(popupWindow->getPosition(), (glm::ivec2 { window->getSize().x, 0 }))
        << "the popup was not clamped into the window it belongs to";

    uitest::frame();
    window->removeAllViews();
    AThread::processMessages();
}

// ADropdownListCompat describes behaviour ADropdownList already has, so it is green against the old
// implementation and must stay green once ADropdownList is an alias onto ACombobox<AString>. These
// are the calls every existing user of the old class makes; if one of them stops compiling or stops
// behaving, the alias dropped something.
//
// testing::UITest rather than a bare TEST: SetUp() (UITestCase.cpp:99) is what calls uitest::setup()
// (UITestCase.cpp:104), and that is what installs the AStubWindowManager a window needs before show().
// ButtonStillRendersTheSelectedLabel builds one, so without the fixture there is no window manager at
// all.
//
// show() is called on the constructed window rather than from inside W's constructor: AWindow::show()
// asks for shared_from_this() (AWindowsImpl.cpp:518, unguarded -- the try/catch two lines above it at
// 513-516 only covers mSelfHolder), and doing that before the object is fully constructed throws
// bad_weak_ptr. Every other uitest in the repo calls show() afterwards.
class ADropdownListCompat : public testing::UITest {};

TEST_F(ADropdownListCompat, IndexApiBehavesAsBefore) {
    auto combo = _new<ADropdownList>(AListModel<AString>::make({ "alpha", "beta", "gamma" }));
    EXPECT_EQ(combo->getSelectionId(), 0);
    combo->setSelectionId(2);
    EXPECT_EQ(combo->getSelectedId(), 2);
    EXPECT_EQ(combo->getModel()->listSize(), 3u);
}

TEST_F(ADropdownListCompat, SelectionIdSignalStillFires) {
    auto combo = _new<ADropdownList>(AListModel<AString>::make({ "a", "b" }));
    gNotifications = 0;
    AObject::connect(combo->selectionChanged, &gSink, [&](int) { ++gNotifications; });

    combo->setSelectionId(1);
    EXPECT_EQ(gNotifications, 1);
}

TEST_F(ADropdownListCompat, ButtonStillRendersTheSelectedLabel) {
    class W : public AWindow {
    public:
        _<ADropdownList> combo;

        W() : AWindow("compat", 400_dp, 300_dp) {
            setContents(Vertical {
                combo = _new<ADropdownList>(AListModel<AString>::make({ "alpha", "beta" })),
            });
            combo->setSelectionId(1);
        }
    };

    auto window = _new<W>();
    window->show();
    uitest::frame();

    EXPECT_EQ(By::text("beta").toVector().size(), 1u);

    window->removeAllViews();
    AThread::processMessages();
}

// The button's label is a view of the model, not a copy of the value at the moment it was selected.
// The old ADropdownList held an index and answered all three model signals with updateText(), so the
// repaint re-read whatever now sat at that index. A value cannot be painted without being resolved
// first, and a mutation can take the selected item out of the model without setModel ever running --
// editing the selected row in place does exactly that.
//
// Answering the model signals with rebuildRows() alone is not enough: it touches mRowsContainer and
// nothing else, so the button keeps naming an item the model no longer holds, while getSelectionId()
// has already given up on finding it. That is the state this test exists to keep out.
TEST_F(ADropdownListCompat, ButtonLabelFollowsTheModel) {
    class W : public AWindow {
    public:
        _<AListModel<AString>> model;
        _<ADropdownList> combo;

        W() : AWindow("label", 400_dp, 300_dp) {
            model = AListModel<AString>::make({ "alpha", "beta" });
            setContents(Vertical {
                combo = _new<ADropdownList>(model),
            });
        }
    };

    auto window = _new<W>();
    window->show();
    uitest::frame();

    ASSERT_EQ(window->combo->getSelectedId(), 0);
    ASSERT_EQ(By::text("alpha").toVector().size(), 1u) << "the button does not start out showing its selection";

    // Editing the selected row in place. setItem writes the vector; invalidate is what announces the
    // dataChanged the combobox subscribes to -- which is why the mutation is two statements and not one.
    window->model->setItem(AListModelIndex(0), AString { "zulu" });
    window->model->invalidate(0);

    EXPECT_EQ(By::text("alpha").toVector().size(), 0u)
        << "the button still names the item the model dropped";
    EXPECT_FALSE(window->combo->getSelected().hasValue())
        << "the widget reports a selected item that is not in the model";
    EXPECT_EQ(window->combo->getSelectedId(), -1);

    // Removing the selected row reaches the same place, and must not leave the same stale label.
    // The selection is not resurrected by putting "alpha" back -- a widget that re-selected itself
    // the moment the value returned would be a second bug, in the other direction -- so it is
    // selected again deliberately before the row is taken away underneath it.
    window->model->setItem(AListModelIndex(0), AString { "alpha" });
    window->model->invalidate(0);
    window->combo->setSelectionId(0);
    ASSERT_EQ(By::text("alpha").toVector().size(), 1u);

    window->model->removeItem(AListModelIndex(0));
    EXPECT_EQ(By::text("alpha").toVector().size(), 0u) << "removing the selected row left its label behind";
    EXPECT_FALSE(window->combo->getSelected().hasValue());
    EXPECT_EQ(window->combo->getSelectedId(), -1);

    window->removeAllViews();
    AThread::processMessages();
}
