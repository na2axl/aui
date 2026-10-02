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
 */
template <typename T>
class ACombobox : public AButton {
public:
    using ViewFactory = std::function<_<AView>(const T&)>;
    using Filter = std::function<bool(const T&, AStringView query)>;

    explicit ACombobox(const _<IListModel<T>>& model) : ACombobox() { setModel(model); }

    ACombobox() {
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
            // setSelected rebuilds only when the selection actually moved, but the rows have to go
            // either way: they are still sitting there for a model that no longer exists. So the
            // rebuild is forced exactly in the case setSelected short-circuits on.
            const bool hadSelection = mSelected.hasValue();
            setSelected(AOptional<T> {});
            if (!hadSelection) {
                rebuildRows();
            }
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
        if (hadSelection && !AModels::indexOf(mModel, mSelected.value()).has_value()) {
            mSelected = AOptional<T> {};
        } else if (!hadSelection && mModel->listSize() > 0) {
            mSelected = mModel->listItemAt(AListModelIndex(0));
        }
        updateText();
        rebuildRows();
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
     * @details The other spelling of [getSelectionId], which documents what the number means; this
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
        rebuildRows();
    }

    _<IListModel<T>> mModel;
    _<AListModelFilter<T, RowFilter>> mFiltered;
    _<AObject> mModelSink;                      // connection sink; replaced to unbind the old model
    _<AObject> mRowSink;                        // connection sink for the row handlers
    ViewFactory mViewFactory;
    Filter mFilterPredicate;
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

    rebuildRows();
    // AScrollArea has only a default constructor; content goes in via setContents.
    auto scrollArea = _new<AScrollArea>();
    scrollArea->setContents(mRowsContainer);
    scrollArea AUI_OVERRIDE_STYLE {
        ass::Margin { 0 },
        // Not expanding vertically: that is what makes AScrollArea::getContentMinimumHeight report
        // the rows' height instead of 0, so the popup -- which is packed to its minimum as soon as it
        // is laid out -- is tall enough for the rows plus this container's own padding and border.
        // Without it the popup is a couple of pixels short of its last row, scrollbar and all.
        ass::Expanding { 1, 0 },
        // ... so the width still has to be stated outright; the height needs no statement.
        ass::MinSize { AMetric(getWidth(), AMetric::T_PX), {} },
    };
    scrollArea << ".combobox_list";

    auto content = _new<AViewContainer>();
    content->setLayout(std::make_unique<AVerticalLayout>());
    content->addView(scrollArea);

    // Height must cover the whole popup content, so anything added above the list later (the filter
    // field) has to go in before this is computed -- it lands in `content` and is counted here.
    const int popupHeight = content->getMinimumHeight() + 2; // bias
    auto comboBoxPos = getPositionInWindow();
    unsigned usedPositionIndex = 0;

    auto comboWindow = parentWindow->createOverlappingSurface(
        [&](unsigned attempt) -> AOptional<glm::ivec2> {
            usedPositionIndex = attempt;
            switch (attempt) {
                case 0: return comboBoxPos + glm::ivec2(0, getHeight());
                case 1: return comboBoxPos - glm::ivec2(0, popupHeight - 1);
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