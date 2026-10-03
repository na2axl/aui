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

#pragma once

#include <AUI/View/AButton.h>
#include <AUI/View/AComboboxRow.h>
#include <AUI/View/AScrollArea.h>
#include <AUI/View/ALabel.h>
#include <AUI/View/ATextField.h>
#include <AUI/Platform/AWindow.h>
#include <AUI/Platform/AOverlappingSurface.h>
#include <AUI/Model/IListModel.h>
#include <AUI/Model/AListModelFilter.h>
#include <AUI/Model/AModels.h>
#include <AUI/Common/AOptional.h>
#include <AUI/Layout/AVerticalLayout.h>
#include <AUI/Animator/ATranslationAnimator.h>
#include <AUI/Animator/ASizeAnimator.h>
#include <AUI/ASS/ASS.h>
#include <AUI/Util/Assert.h>
#include <AUI/Util/Declarative/Containers.h>
#include <AUI/Util/kAUI.h>
#include <functional>
#include <optional>

/**
 * @brief A button with a popup list whose items are of an arbitrary type.
 *
 * @ingroup views_input
 * @details
 * Where ADropdownList presents AStrings, this presents T, so a widget can carry an item's identity
 * rather than a row number. That matters the moment the visible rows are reordered: this widget
 * resolves its selection by value, so replacing or filtering the model can never make the
 * selection drift onto a different item.
 *
 * Selection is AOptional so that "nothing selected" is representable. An index sentinel cannot
 * express it safely, because a negative index promotes to a huge unsigned value when compared
 * against a size_t list size.
 *
 * T must either be AString, or be given a ViewFactory via setViewFactory() before the model is set.
 *
 * <!-- aui:index_alias ADropdownList -->
 */
template <typename T>
class ACombobox : public AButton {
public:
    using ViewFactory = std::function<_<AView>(const T&)>;
    using Filter = std::function<bool(const T&, AStringView query)>;

    explicit ACombobox(const _<IListModel<T>>& model) : ACombobox() { setModel(model); }

    ACombobox() {
        // The class the default stylesheet keys the button's own label off. Keying that rule on
        // t<ADropdownList>() instead would have made it match ACombobox<AString> and nothing else,
        // so every other T -- the whole reason this class is templated -- rendered its label
        // centred instead of left-aligned, with nothing to say so. See AStylesheet.cpp.
        addAssName(".combobox");
        if constexpr (std::is_same_v<T, AString>) {
            mViewFactory = [](const T& value) -> _<AView> { return _new<ALabel>(value); };
        }
        // Any other T has no default renderer: turning an arbitrary value into a view is the app's
        // call. The factory cannot default to one that diagnoses its absence at compile time -- this
        // constructor already renders the first row before the caller gets a chance to install it --
        // so viewFor is where the precondition is checked instead. See setViewFactory.
        mRowsContainer = _new<AViewContainer>();
        mRowsContainer->setLayout(std::make_unique<AVerticalLayout>());
    }

    ~ACombobox() override {
        if (auto w = mComboWindow.lock()) {
            w->close();
        }
    }

    void setModel(const _<IListModel<T>>& model) {
        if (!model) {
            mModel = nullptr;
            mFiltered = nullptr;
            mModelSink = nullptr;
            // The rows have to go either way -- they are still sitting there for a model that no
            // longer exists -- which is what applySelection owes on the half of its work that
            // setSelected, short-circuiting on an unchanged selection, does not do.
            applySelection(AOptional<T> {});
            return;
        }

        // The old implementation connected the model's three signals on every setModel call with
        // nothing to unbind them, so a combobox re-binding per keystroke would leak three
        // connections per keystroke. AUI has no unbind overload -- connections die with their
        // *receiver*, so replacing this sink is what releases the previous model's subscriptions.
        mModelSink = nullptr;
        mModel = model;

        mFiltered = _new<AListModelFilter<T, RowFilter>>(mModel, [this](const T& item) {
            return !mFilterPredicate || mFilterPredicate(item, mFilter);
        });

        mModelSink = _new<AObject>();
        AObject::connect(mModel->dataChanged, mModelSink, [this](const AListModelRange<T>&) { onModelMutated(); });
        AObject::connect(mModel->dataInserted, mModelSink, [this](const AListModelRange<T>&) { onModelMutated(); });
        AObject::connect(mModel->dataRemoved, mModelSink, [this](const AListModelRange<T>&) { onModelMutated(); });

        // Preserve by value: look the selection up in the new model rather than carrying its index.
        // `hadSelection` is what separates "nothing was selected yet" (pick a sensible default) from
        // "the selected item is gone" (stay empty). Picking the default unconditionally would put
        // the selection straight back on whatever now sits at the dropped item's index, which is
        // the very failure this widget exists to remove.
        const bool hadSelection = mSelected.hasValue();
        AOptional<T> selection = mSelected;
        if (hadSelection) {
            if (!AModels::indexOf(mModel, mSelected.value()).has_value()) {
                selection = AOptional<T> {};
            }
        } else if (mModel->listSize() > 0) {
            selection = mModel->listItemAt(AListModelIndex(0));
        }
        applySelection(selection);
    }

    [[nodiscard]] const _<IListModel<T>>& getModel() const noexcept { return mModel; }

    /**
     * @brief Installs the renderer turning an item of type T into a view.
     * @details Must be called before the model is set whenever T is not AString: setModel renders
     *          the default selection straight away, so a model installed with no factory yet trips
     *          the assertion in viewFor.
     */
    void setViewFactory(ViewFactory factory) {
        mViewFactory = std::move(factory);
        updateText();
        rebuildRows();
    }

    /**
     * @brief Sets the predicate deciding which items the current query shows.
     * @param predicate nullptr restores pass-through, i.e. every item is shown.
     * @details There is no default predicate, and cannot be one: turning an arbitrary T into
     *          searchable text is the application's call, not the framework's. So an unset
     *          predicate shows everything, which is what setFilter with no predicate installed
     *          does -- the query is still stored and still announced, it simply excludes nothing.
     */
    void setFilterPredicate(Filter predicate) {
        mFilterPredicate = std::move(predicate);
        if (mFiltered) {
            mFiltered->setFilter([this](const T& item) {
                return !mFilterPredicate || mFilterPredicate(item, mFilter);
            });
            // AListModelFilter announces on its own signals while the combobox subscribes to the
            // source model's, so setFilter's notification stops here. Without this a predicate
            // installed after a query exists would re-filter the model but leave the rows alone.
            rebuildRows();
        }
    }

    /**
     * @brief Shows, or hides, a text field at the top of the popup that filters the rows as it is
     *        typed into.
     * @details Opt-in, so a combo box nobody filters looks exactly as it did. The field is worth
     *          nothing without a predicate: setFilterPredicate has no default because an
     *          arbitrary T cannot be turned into searchable text, so the field on its own types a
     *          query that hides nothing.
     *          Turning the field off also clears the query, so the widget is never left hiding rows
     *          behind a control the author has just taken away. That clear is an edit and is
     *          announced like one; the clear that happens when a popup opens is not (see setFilter).
     *          A popup that is already open survives the switch: the popup is a window of its own
     *          and takes no instruction from the widget, so its field keeps the text it had. What it
     *          loses is its vote -- the field's signals are ignored from here on, so typing into it
     *          cannot put a query back into a filter that is off.
     */
    void setFilterEnabled(bool enabled) {
        if (mFilterEnabled == enabled) {
            return;
        }
        mFilterEnabled = enabled;
        if (!enabled) {
            setFilter({});
        }
    }
    [[nodiscard]] bool isFilterEnabled() const noexcept { return mFilterEnabled; }

    auto filter() const {
        return APropertyDef { this, &ACombobox::getFilter, &ACombobox::setFilter, filterChanged };
    }
    [[nodiscard]] AString getFilter() const noexcept { return mFilter; }

    /**
     * @brief Current filter query.
     * @details Never written by opening the popup or by a model swap; only by setFilter, and by the
     * author typing in the filter field. Narrowing the rows is not an edit of the value.
     */
    void setFilter(AString query) {
        if (mFilter == query) {
            return;
        }
        mFilter = std::move(query);
        if (mFiltered) {
            mFiltered->invalidate();
        }
        // Note what is deliberately absent: the selection is not touched. A filter that silently
        // changed the author's value while they typed would be worse than no filter at all.
        rebuildRows();
        emit filterChanged(mFilter);
    }

    auto selected() const {
        return APropertyDef { this, &ACombobox::getSelected, &ACombobox::setSelected, selectedChanged };
    }
    [[nodiscard]] AOptional<T> getSelected() const noexcept { return mSelected; }
    void setSelected(const AOptional<T>& value) {
        if (mSelected == value) {
            return;
        }
        mSelected = value;
        updateText();
        // Selection is a property of the rows too, so a programmatic change has to reach them --
        // otherwise an open popup keeps highlighting the previous selection. Only while there is one:
        // onPointerReleased rebuilds before showing the list, so the closed case needs nothing, and
        // setSelected is the hot path of a two-way binding that would otherwise tear down and
        // reconnect every row per change, for a popup the user cannot see.
        if (mComboWindow.lock()) {
            rebuildRows();
        }
        emit selectedChanged(value);
        // What makes this announcement safe to act on is not this class: a property destination
        // drops the echo back into the widget while this very signal is mid-emission. See
        // applySelection before changing who is connected to it.
        emit selectionChanged(getSelectionId());
    }

    auto selectionId() const {
        return APropertyDef { this, &ACombobox::getSelectionId, &ACombobox::setSelectionId, selectionChanged };
    }
    /**
     * @brief Index of the selected item among the currently visible rows, or -1 for no selected row.
     * @details -1 means there is no selected row, which covers three cases that are worth telling
     *          apart: nothing is selected at all; there is no model, or the model is empty and so
     *          has no row 0 to default to; and the selected item is real but is not among the rows
     *          being shown, because the current filter hides it. In the last case getSelected() still
     *          answers with the value -- the selection is not lost, only out of sight.
     */
    [[nodiscard]] int getSelectionId() const noexcept {
        if (!mSelected.hasValue()) {
            return -1;
        }
        auto model = visibleModel();
        if (!model) {
            return -1;
        }
        if (auto found = AModels::indexOf(model, mSelected.value()); found.has_value()) {
            return int(*found);
        }
        return -1;
    }
    /**
     * @brief Selects the row at the given index.
     * @param id The row to select. -1 is the real clearing value, as it is for the property's
     *           current value: it empties the selection and announces it on selectedChanged and
     *           selectionChanged.
     * @details An index past the last visible row is ignored -- nothing is stored and nothing is
     *          announced. This is deliberate, and it is a difference from the old ADropdownList,
     *          whose mSelectionId assignment went through with any value and emitted regardless: the
     *          old class could be told to select row 7 of a 2-row model and would report row 7 back
     *          out of getSelectionId(). Resolving the index here means the number that comes out is
     *          always a number that means something.
     *          A caller that used to clear the selection with setSelectionId(listSize()) no longer
     *          gets an announcement to propagate; -1 says the same thing and does.
     */
    void setSelectionId(int id) {
        if (id < 0) {
            setSelected(AOptional<T> {});
            return;
        }
        if (auto value = itemAt(std::size_t(id)); value.hasValue()) {
            setSelected(value);
        }
    }

    /**
     * @brief Index of the selected item among the currently visible rows.
     * @details The other spelling of getSelectionId, which documents what the number means; this
     *          is the same value under the name ADropdownList's callers know.
     */
    [[nodiscard]] int getSelectedId() const noexcept { return getSelectionId(); }

    [[nodiscard]] bool isPopupOpen() const noexcept { return !mComboWindow.expired(); }
    void destroyWindow();

    void render(ARenderContext context) override;
    [[nodiscard]] int getContentMinimumWidth() override;
    void onPointerPressed(const APointerPressedEvent& event) override;
    void onPointerReleased(const APointerReleasedEvent& event) override;
    void onPointerMove(glm::vec2 pos, const APointerMoveEvent& event) override;

signals:
    emits<AOptional<T>> selectedChanged;
    emits<AString> filterChanged;
    /**
     * @brief The selected row's index among the visible rows, announced whenever it changes, -1 for
     *        no selected row.
     * @details Bind to the property, `combo->selectionId()`, not to this signal with a lambda.
     *          -1 is also what a selection dropped by a model mutation or a model replacement
     *          announces, and a lambda slot has nothing to stop it acting on that: the handler
     *          clears its model, the expression emits 0, and the widget re-selects row 0 -- undoing,
     *          one frame later, the drift a value-resolved selection exists to remove. A property
     *          destination has the loop guard that drops that echo (the destination's own `changed`
     *          is mid-emission, aui.core/src/AUI/Common/detail/property.h:32), so the -1 sticks.
     *          Nothing to fix here; the guard is a framework rule.
     */
    emits<int> selectionChanged;

protected:
    virtual void updateText();
    virtual void onComboBoxWindowCreated() {}
    _<AViewContainer> comboWindow() { return mComboWindow.lock(); }
    /**
     * @brief Rebuilds the visible rows out of the current model, query and selection.
     */
    virtual void rebuildRows() {
        mRowsContainer->removeAllViews();
        mRowSink = _new<AObject>();
        auto model = visibleModel();
        if (!model) {
            return;
        }
        const int selectedId = getSelectionId();
        for (std::size_t i = 0; i < model->listSize(); ++i) {
            auto value = model->listItemAt(AListModelIndex(i));
            auto content = viewFor(value);
            auto row = _new<AComboboxRow>(content);
            row->setSelected(int(i) == selectedId);
            const auto onActivated = [this, value] {
                setSelected(value);
                destroyWindow();
            };
            // Both, not just the row: a press that lands on the content is routed to the content,
            // not to the row it sits in, so a row whose content fills it would never be selectable.
            //
            // The receiver is the sink rather than `this`, for the same reason mModelSink exists:
            // AWindow::close() is deferred, so the popup's rows can outlive the combobox briefly,
            // and a handler reaching a destroyed combobox is exactly what the sink prevents.
            connect(row->clicked, mRowSink, onActivated);
            connect(content->clicked, mRowSink, onActivated);
            mRowsContainer->addView(row);
        }
    }

private:
    /**
     * @brief The predicate AListModelFilter is instantiated with: it takes the item alone, because
     *        the query is captured from mFilter by the adapter built in setModel.
     * @details Distinct from [Filter], which is the app-facing predicate taking both.
     */
    using RowFilter = std::function<bool(const T&)>;

    /**
     * @brief Brings the widget back in step with a model that just announced a change.
     * @details ADropdownList answered all three signals with updateText() alone, which was enough
     *          while the selection was an index: repainting the button re-read whatever now sat at
     *          that index. A value cannot be painted before it has been resolved, because a mutation
     *          can take the selected item out of the model without setModel ever running -- editing
     *          the selected row in place is exactly that, and it arrives as dataChanged. So the
     *          selection is re-resolved first, under the same rule setModel applies (an item the
     *          model no longer holds leaves nothing selected), and only then is the button repainted.
     *          Skipping the repaint is what leaves the button naming an item the model has dropped
     *          while getSelectionId() can no longer find it.
     * @note Deliberately not folded into rebuildRows: that rebuilds the popup's rows out of the
     *       model, and the button's label is not one of them.
     */
    void onModelMutated() {
        if (mModel && mSelected.hasValue() && !AModels::indexOf(mModel, mSelected.value()).has_value()) {
            setSelected(AOptional<T> {});
        }
        updateText();
        // Only while there is a popup, for the reason setSelected gives: a closed combo box is
        // rebuilt on its way to opening anyway, so rebuilding here would construct an AComboboxRow
        // and a content view for every item of a popup nobody can see -- and the rows it did build
        // would be thrown away and rebuilt on the next click. On a 5k-item dropdown that is 5k view
        // allocations per model mutation, for nothing. ADropdownList was not wasteful here because
        // it answered the model signals with updateText() alone.
        if (mComboWindow.lock()) {
            rebuildRows();
        }
    }

    /**
     * @brief Applies the selection setModel has settled on, announcing it the way any other change
     *        is announced.
     * @details setModel and onModelMutated apply one policy -- an item the model no longer holds
     *          leaves nothing selected -- and used to implement it two ways: setModel assigned
     *          mSelected directly and said nothing, while onModelMutated went through setSelected and
     *          announced. A two-way binding therefore heard about a selection lost to a mutation and
     *          was left pointing at a value that no longer existed when the same selection was lost
     *          to a replacement.
     *          setSelected already repaints, and rebuilds the rows while a popup is open; doing that
     *          here as well would run both a second time on every setModel call, so on the one path
     *          where setSelected has nothing to announce they are run here instead. That is the only
     *          case that reaches them: an announced change has already had them, unconditionally for
     *          the repaint and for the rebuild exactly when there is a popup for setSelected to see.
     *          The announcement happens with the value already stored, so a handler that writes back
     *          into the model -- or calls setModel again -- re-enters a widget that has settled, and
     *          its own setSelected short-circuits instead of starting a cycle.
     *          What makes the -1 announcement safe for a binding to act on is the property binding's
     *          own loop guard; the shape that needs it is on selectionChanged, which is public.
     */
    void applySelection(const AOptional<T>& value) {
        if (mSelected == value) {
            updateText();
            rebuildRows();
            return;
        }
        setSelected(value);
    }

    _<IListModel<T>> mModel;
    _<AListModelFilter<T, RowFilter>> mFiltered;
    _<AObject> mModelSink;                      // connection sink; replaced to unbind the old model
    _<AObject> mRowSink;                        // connection sink for the row handlers
    ViewFactory mViewFactory;
    Filter mFilterPredicate;
    bool mFilterEnabled = false;
    AOptional<T> mSelected;
    AString mFilter;

    _<AViewContainer> mRowsContainer;
    _weak<AOverlappingSurface> mComboWindow;

    /**
     * @brief The widget's own record that it has a popup open.
     * @details Not redundant with mComboWindow: the popup is created through createOverlappingSurface's
     *          position-factory overload, whose closeOnClick defaults to true, so
     *          ASurface::onPointerPressed drops the surface -- and with it the last strong reference --
     *          before the press ever reaches this view. By the time onPointerPressed could ask,
     *          mComboWindow is expired whether the popup was open or never existed, so only a value
     *          the widget writes itself can tell the two apart. Reconciled against mComboWindow on
     *          every pointer move, since a click that lands anywhere else also kills the popup without
     *          running a line of this class.
     * @note This latch and isPopupOpen() deliberately disagree during the stale window -- after a
     *       foreign kill, before the next pointer move. The latch is pessimistically "I owe the user
     *       a toggle-close", which is what onPointerReleased has to read; isPopupOpen() reports
     *       observable truth. `isPopupOpen() { return mPopupIsOpen; }` is the obvious tidy-up and it
     *       breaks the toggle: the next press after a click-away would close a popup that has been
     *       gone for a while instead of opening one.
     */
    bool mPopupIsOpen = false;

    /**
     * @brief Whether the popup was open when the current click went down.
     * @details A copy, not a reference to mPopupIsOpen: the press that closes the popup must not be
     *          able to change the answer the release is about to read.
     */
    bool mPopupWasOpenOnPress = false;

    /**
     * @brief The model the visible rows are read from: the filtered one while a model is installed,
     *        the raw one otherwise. null when there is no model at all.
     * @details `_<AListModelFilter>` and `_<IListModel>` have no implicit common type, so the
     * conversion is spelled out rather than left to a conditional operator.
     */
    [[nodiscard]] _<IListModel<T>> visibleModel() const noexcept {
        return mFiltered ? _<IListModel<T>>(mFiltered) : mModel;
    }

    AOptional<T> itemAt(std::size_t index) const {
        auto model = visibleModel();
        if (!model || index >= model->listSize()) {
            return {};
        }
        return model->listItemAt(AListModelIndex(index));
    }

    /**
     * @brief Renders an item through the currently installed factory.
     * @details Asserts rather than falling back to an empty view: a forgotten setViewFactory is a
     *          developer precondition, and silently rendering nothing hides the mistake for good.
     */
    [[nodiscard]] _<AView> viewFor(const T& value) {
        AUI_ASSERTX(mViewFactory != nullptr, "ACombobox<T> requires setViewFactory(): T is not AString");
        return mViewFactory(value);
    }
};

template <typename T>
void ACombobox<T>::updateText() {
    // Same mechanism as AButton::setText, so the existing t<ADropdownList>() >> t<ALabel>()
    // stylesheet rule keeps matching the button's content.
    setContents(declarative::Centered { mSelected.hasValue() ? viewFor(mSelected.value()) : _new<ALabel>() });
}

template <typename T>
void ACombobox<T>::render(ARenderContext context) {
    AButton::render(context);
    if (auto arrow = IDrawable::fromUrl(":uni/svg/combo.svg")) {
        auto size = arrow->getSizeHint();
        IDrawable::Params p;
        p.size = size;
        p.offset = { getWidth() - size.x - getPadding().right, (getHeight() - size.y) / 2 };
        arrow->draw(context.render, p);
    }
}

template <typename T>
int ACombobox<T>::getContentMinimumWidth() {
    return AButton::getContentMinimumWidth() + 20;
}

template <typename T>
void ACombobox<T>::destroyWindow() {
    if (auto w = mComboWindow.lock()) {
        w->close();
    }
    mPopupIsOpen = false;
    mComboWindow.reset();
    emit customCssPropertyChanged;
    redraw();
}

template <typename T>
void ACombobox<T>::onPointerPressed(const APointerPressedEvent& event) {
    // Before delegating, and read from mPopupIsOpen rather than from mComboWindow: the window this
    // press went to has already run closeOverlappingSurfacesOnClick(), so the surface is destroyed
    // by the time it gets here and cannot answer the question. This is the last moment of the click
    // at which the answer still exists.
    mPopupWasOpenOnPress = mPopupIsOpen;
    AView::onPointerPressed(event);
}

template <typename T>
void ACombobox<T>::onPointerMove(glm::vec2 pos, const APointerMoveEvent& event) {
    // The one place a popup closed by a click somewhere else can be noticed: such a click never
    // reaches this class, so mPopupIsOpen would go stale and the next press on the combo box would
    // close a popup that is long gone instead of opening one. Reaching here means the pointer is on
    // the combo box, and nothing can have clicked elsewhere without it first leaving.
    if (mPopupIsOpen && mComboWindow.expired()) {
        mPopupIsOpen = false;
    }
    AView::onPointerMove(pos, event);
}

template <typename T>
void ACombobox<T>::onPointerReleased(const APointerReleasedEvent& event) {
    AView::onPointerReleased(event);
    const bool popupWasOpenOnPress = mPopupWasOpenOnPress;
    mPopupWasOpenOnPress = false;
    if (!event.triggerClick) {
        return;
    }
    if (popupWasOpenOnPress) {
        // The framework closed the surface during the press; only the widget's own bookkeeping is
        // left. destroyWindow() would re-issue close() on a window already on its way out, which on
        // win32 is harmless but emits closed() twice.
        mPopupIsOpen = false;
        mComboWindow.reset();
        emit customCssPropertyChanged;
        redraw();
        return;
    }
    if (mComboWindow.lock()) {
        destroyWindow();
        return;
    }
    auto parentWindow = getWindow();
    if (!parentWindow) {
        return;
    }
    if (mFilterEnabled && !mFilter.empty()) {
        // A query left over from the last time the combo was used hides rows the author expects to
        // see, and reads as a wrong list rather than a stale one. Clearing it must not fire
        // filterChanged: opening a popup is not an author edit.
        mFilter = {};
        if (mFiltered) {
            mFiltered->invalidate();
        }
        // No rebuildRows() here: the unconditional one a few lines further down does it, before the
        // popup exists at all, so the one this block used to carry was a second rebuild of rows
        // nothing had changed in between.
    }
    auto comboBoxPos = getPositionInWindow();

    // The ceiling the popup may not pass, measured against the room its placement actually has.
    // createOverlappingSurface takes the first placement the factory offers that lands at
    // non-negative coordinates (ASurface.h:277) and "below" is offered first, so downwards is the
    // side the popup opens towards for every combobox that is not itself off the top or left edge of
    // its window; above is what the rest get. The two sides are not measured the same way, and the
    // reason is that only one of them has a hard edge to measure to. Below, the combobox is above
    // the popup and there is a definite amount of window left underneath it. Above, the position
    // factory clamps the popup into the window, so the window is the only bound available before the
    // height is known -- measuring instead the gap between the window's top and the combobox's own
    // top degenerates to nothing for a combobox that hangs off the top edge, which is exactly the
    // case that gets there, and would collapse the popup to zero height.
    //
    // The bound is a slice of the parent's own height, and it is a bound on the popup as a whole
    // rather than on the scroll area alone: the popup is `content` plus the surface's own chrome, and
    // popupHeight adds the 2px bias underneath, so a popup pinned to the cap can still hang that much
    // below the parent's bottom edge. That is also why the cap is not merely handed over to
    // createOverlappingSurface as a size: the popup is packed to its own minimum height, which is the
    // content's, which is the rows' -- so a ceiling handed over as a size alone is undone by the next
    // layout pass.
    //
    // Capping cannot push the rows off the screen either: below, the position does not depend on the
    // height at all, and above, the factory clamps.
    const int openBelow = comboBoxPos.y + getHeight();
    const bool opensBelow = openBelow >= 0 && comboBoxPos.x >= 0;
    const int room = opensBelow ? parentWindow->getHeight() - openBelow : parentWindow->getHeight();
    const int popupMaxHeight = (glm::min)((glm::max)(room, 0), parentWindow->getHeight());

    rebuildRows();

    // The filter field is built first, and that is not a matter of taste: its height comes out of
    // the popup's ceiling below, and the ceiling goes into the scroll area's MaxSize, which is what
    // the popup's own height is computed from. Anything the field is going to cost has to be known
    // before that number is asked for.
    _<ATextField> field;
    int filterHeight = 0;
    if (mFilterEnabled) {
        field = _new<ATextField>();
        // .input-field, which ATextField already carries, supplies the white fill, the text colour
        // and the font. .combobox_filter is what fits the field into the frame the popup now draws
        // around both halves (see where content is named .combobox_popup): it zeroes the radius,
        // and because every border flavour shares one property slot, its BorderBottom is not an
        // addition to .input-field's frame but a replacement of it -- the popup's grey as a divider
        // under the field instead of a second frame above it. Both rules are in AStylesheet.cpp,
        // with the reasoning that moved the frame up here.
        field << ".combobox_filter";
        // No setText(mFilter): the clear above already ran, and it runs whenever mFilter is
        // non-empty, so by this line the query is empty and the field starts empty with it. The
        // connections below are made after this point anyway, so a setText here would reach nobody.
        //
        // Both announcements, and for the reason each one exists: setText announces on textChanged
        // (AAbstractTextField.cpp:89) and a keystroke announces on textChanging (:175), so a field
        // connected to only one of them answers only half the ways its own text can change --
        // connected to textChanging alone it would show a setText's query without narrowing, and
        // connected to textChanged alone the user could not type at all.
        //
        // Both check mFilterEnabled because the field outlives the setting in one case: the popup
        // is a window of its own, so setFilterEnabled(false) while it is open leaves the field
        // sitting there with the text it had. Without the guard, typing into it would put a query
        // back into a filter the author had just switched off.
        AObject::connect(field->textChanging, this, [this](const AString& text) {
            if (mFilterEnabled) {
                setFilter(text);
            }
        });
        AObject::connect(field->textChanged, this, [this](const AString& text) {
            if (mFilterEnabled) {
                setFilter(text);
            }
        });
        // Asked for here, off a field that is not in a window yet, so it is a number the layout can
        // disagree with. A view with no ancestry is styled without the context-dependent rules that
        // match on an ancestor -- a rule like `Window ATextField { MinSize { 100, 90 } }` cannot
        // match it -- and adding the field to `content` puts the popup window above it, at which
        // point the view-graph change invalidates the styles and the same field lays out larger.
        // Measured, 400x300 window, 60 rows, that rule installed: 22 here, 90 once laid out.
        //
        // Which is why the ceiling below is also on `content` and not only on the list: the sum
        // the popup's height is computed from is this number plus the list's, and a field that
        // grows is room the popup does not have, so the popup leaves the parent by exactly its
        // overshoot rather than the list merely falling short of the room.
        filterHeight = field->getMinimumHeight();
    }

    // AScrollArea has only a default constructor; content goes in via setContents.
    auto scrollArea = _new<AScrollArea>();
    scrollArea->setContents(mRowsContainer);
    scrollArea AUI_OVERRIDE_STYLE {
        ass::Margin { 0 },
        // Not expanding vertically: AScrollArea::getContentMinimumHeight answers 0 for a view that
        // expands on that axis (AScrollArea.cpp:67), and the popup is packed to its own minimum as
        // soon as it is laid out -- so an expanding list is a popup with no rows, and one that does
        // not expand is a popup a couple of pixels short of its last row, scrollbar and all. That is
        // also why the room above the list has to be taken off the ceiling by hand below, and why it
        // cannot be taken off by letting the list expand into it.
        ass::Expanding { 1, 0 },
        // ... so the width still has to be stated outright; the height needs no statement.
        ass::MinSize { AMetric(getWidth(), AMetric::T_PX), {} },
        // ... up to a point. The ceiling belongs here and not only on the surface passed to
        // createOverlappingSurface: the popup is packed to its own minimum height, which is the
        // content's, which is the rows' -- so a ceiling handed over as a size alone is undone by the
        // next layout pass. MaxSize is AScrollArea's documented way of becoming a scroll area without
        // expanding, and it is also what makes the viewport smaller than the content; with the two
        // the same size no scrollbar can appear however long the list is.
        //
        // What the list may have is what the room leaves once the filter field has taken its share.
        // AView::getMinimumHeight clamps to MaxSize, so this is what the popup's own height is
        // computed from a few lines further down -- had the ceiling stayed the full room, the field
        // would have been added on top of a popup already sized to fill the window, and the bottom
        // of the popup would hang past the parent's edge by the field's height.
        ass::MaxSize { {}, AMetric((glm::max)(popupMaxHeight - filterHeight, 0), AMetric::T_PX) },
    };
    scrollArea << ".combobox_list";

    auto content = _new<AViewContainer>();
    content->setLayout(std::make_unique<AVerticalLayout>());
    content AUI_OVERRIDE_STYLE {
        // The same ceiling as the scroll area's, on the popup's content as a whole. The list's own
        // MaxSize has the field's share subtracted from it, so the two sum to the room only while
        // the field measures the same here as it measures once it is laid out -- and it need not
        // (see where filterHeight is read). Uncapped, the popup's minimum is popupMaxHeight plus
        // exactly the field's overshoot, so the popup leaves the parent by that much rather than
        // the list falling short of the room.
        //
        // Clamping popupHeight instead would not do, for the reason the scroll area's MaxSize is
        // there at all: the popup is packed to its own minimum every frame, so a bound that is not
        // on something the minimum is read through is undone by the next layout pass.
        ass::MaxSize { {}, AMetric(popupMaxHeight, AMetric::T_PX) },
    };
    if (field) {
        // `field` is the predicate here and mFilterEnabled is what it tracks: the handle is created
        // under exactly that test at the top of this function and is never reassigned, so a popup
        // has one exactly when the filter is on. Naming .combobox_popup there and not here would
        // restyle every ADropdownList in every application -- it is what makes .combobox_list give
        // up its own frame in exchange, and a popup with no field has nothing to share it with, so
        // it keeps the frame it has always had. See AStylesheet.cpp.
        content << ".combobox_popup";
        content->addView(field);
    }
    content->addView(scrollArea);

    // Height must cover the whole popup content, so the filter field above has to be in before this
    // is computed -- it lands in `content` and is counted here. The ceiling is the room there is and
    // the field spends its share of it, so a list that no longer fits is the list that scrolls.
    // The ceiling is already folded into the minimum height read here (getMinimumHeight clamps to
    // MaxSize), on `content` as well as on the list inside it, so this is the capped height when
    // the rows do not fit and the content's own height when they do.
    const int popupHeight = content->getMinimumHeight() + 2; // bias
    unsigned usedPositionIndex = 0;

    auto comboWindow = parentWindow->createOverlappingSurface(
        [&](unsigned attempt) -> AOptional<glm::ivec2> {
            usedPositionIndex = attempt;
            switch (attempt) {
                case 0: return comboBoxPos + glm::ivec2(0, getHeight());
                // Clamped, because the raw subtraction is not a fallback at all: it is strictly
                // above attempt 0's y for every popupHeight >= 2, so an unclamped attempt 1 could
                // only ever be rejected where attempt 0 was, and the popup would never open at all
                // for a combo box near the top of its window. Clamping makes it a real second
                // choice -- flush with the window's top when there is room, slid down when there is
                // not -- which is also what makes the room measured above the correct ceiling.
                //
                // Only the y is bounded, which is narrower than what the framework's other
                // createOverlappingSurface overload clamps: that one subtracts the size on both
                // axes (ASurface.h:253), so it also pulls the x in to keep the popup inside the
                // window's right edge. Here the x is bounded by the window's width alone, so a
                // popup wider than its window still hangs off the right. That is the looseness
                // attempt 0 has always had, not something the clamp introduced, and
                // ThePopupClampsItselfIntoTheWindowWhenItHasToOpenAbove pins it as it is -- the x
                // at the window's width, not at the window's width less the popup's.
                case 1: return glm::clamp(comboBoxPos - glm::ivec2(0, popupHeight - 1),
                                          { 0, 0 }, parentWindow->getSize() - glm::ivec2(0, popupHeight));
                default: return std::nullopt;
            }
        },
        { (glm::max)(getWidth(), content->getMinimumWidth()), popupHeight });

    comboWindow->setLayout(std::make_unique<AVerticalLayout>());
    comboWindow->addView(content);
    mComboWindow = comboWindow;
    mPopupIsOpen = true;

    // ADropdownList's reveal, kept: opening downward grows the list out of nothing, opening upward
    // additionally slides it in from below. ASizeAnimator drives the real size and settles on it, so
    // the only price is that the rows are not hittable until the reveal is over.
    //
    // On the list and not on the popup, so with the filter field enabled the field is simply there
    // while the rows grow in under it. That reads correctly rather than as a glitch: the field is the
    // control the user is reaching for, and the rows are the answer to it arriving. Animating it too
    // would only make the query impossible to type into during the reveal.
    if (usedPositionIndex == 0) {
        scrollArea->setAnimator(_new<ASizeAnimator>(glm::ivec2 { scrollArea->getWidth(), 0 }) AUI_LET {
            it->setDuration(0.15f);
        });
    } else {
        scrollArea->setAnimator(AAnimator::combine({
            _new<ATranslationAnimator>(glm::ivec2(0, popupHeight)) AUI_LET { it->setDuration(0.15f); },
            _new<ASizeAnimator>(glm::ivec2 { scrollArea->getWidth(), 0 }) AUI_LET { it->setDuration(0.15f); },
        }));
    }

    onComboBoxWindowCreated();
}
