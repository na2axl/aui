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
#include <AUI/Model/AListModel.h>
#include <AUI/View/ACombobox.h>
#include <AUI/View/ADropdownList.h>

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

    // The popup reveals itself with an ASizeAnimator, growing its scroll area out of a zero height,
    // so a row is not hittable until the reveal is done. uitest::frame() does not advance the clock
    // -- the animator is wall-clock driven -- so a test that clicks straight after opening the popup
    // races it. Poll the list up to its full height with a bounded budget instead.
    //
    // TypingInTheFilterNarrowsTheRows, which arrives with the filter field in a later task, clicks
    // into the popup as well and has to wait the same way.
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
        // border, and that arithmetic only balances because of ass::Expanding { 1, 0 }: expanding
        // vertically is what stops AScrollArea::getContentMinimumHeight from reporting 0 and
        // swallowing the inset. It has broken once already, and the next task adds a filter field
        // above the list, perturbing precisely this arithmetic. So pin what actually breaks -- the
        // last row fitting inside the list:
        //
        //   ass::Expanding { 1, 0 } (current): list 64 tall, last row spans 42..62 -> fits
        //   ass::Expanding {}       (previous): list 60 tall, last row spans 42..62 -> clipped 2px
        //
        // Comparing the list against the window instead would prove nothing: the list is the
        // popup's whole content, so its bottom always equals the window's bottom.
        auto rows = By::name(".list-item").toVector();
        ASSERT_FALSE(rows.empty()) << "the popup lists no rows to fit";
        ASSERT_LE(rows.back()->getPositionInWindow().y + rows.back()->getSize().y,
                  list->getPositionInWindow().y + list->getSize().y)
            << "the popup is too short for its own rows; the last one is clipped";
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

    const int afterSwap = combo->rebuilds;

    // push_back announces on dataInserted, so a still-bound combobox would react to this.
    oldModel->push_back("MUTATED");
    EXPECT_EQ(combo->rebuilds, afterSwap) << "the swapped-out model is still driving the widget";

    // ...and the model actually in use does reach it.
    newModel->push_back("four");
    EXPECT_GT(combo->rebuilds, afterSwap) << "the current model is not driving the widget";
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
