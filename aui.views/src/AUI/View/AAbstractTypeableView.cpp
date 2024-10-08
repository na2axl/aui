/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2024 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//
// Created by alex2 on 5/23/2021.
//


#include <AUI/Platform/AClipboard.h>
#include "AAbstractTypeableView.h"


#include "AUI/Platform/APlatform.h"
#include "AUI/Render/ARender.h"
#include "AUI/Render/RenderHints.h"
#include "AUI/Util/ARaiiHelper.h"
#include <AUI/Util/AMetric.h>
#include <AUI/Action/AMenu.h>
#include <AUI/Util/kAUI.h>
#include <AUI/i18n/AI18n.h>

_<ATimer> AAbstractTypeableView::blinkTimer()
{
    using namespace std::chrono_literals;
    static _weak<ATimer> t;
    if (auto l = t.lock()) {
        return l;
    }
    auto timer = _new<ATimer>(500ms);
    timer->start();
    t = timer;
    return timer;
}

void AAbstractTypeableView::updateCursorBlinking()
{
    mBlinkTimer->restart();
    mCursorBlinkVisible = true;
    mCursorBlinkCount = 0;
    redraw();
}

void AAbstractTypeableView::updateCursorPos()
{
    auto absoluteCursorPos = -mHorizontalScroll + int(getFontStyle().getWidth(getDisplayText().substr(0, mCursorIndex)));

    const int SCROLL_ADVANCEMENT = getContentWidth() * 4 / 10;

    if (absoluteCursorPos < 0)
    {
        mHorizontalScroll += absoluteCursorPos - SCROLL_ADVANCEMENT;
    }
    else if (absoluteCursorPos >= getContentWidth())
    {
        mHorizontalScroll += absoluteCursorPos - getContentWidth() + SCROLL_ADVANCEMENT;
    }
    mHorizontalScroll = glm::clamp(mHorizontalScroll, 0, glm::max(int(getFontStyle().getWidth(getDisplayText())) - getContentWidth() + 1, 0));
}



AAbstractTypeableView::AAbstractTypeableView()
{
    connect(mBlinkTimer->fired, this, [&]()
    {
        if (hasFocus() && mCursorBlinkCount < 60) {
            mCursorBlinkVisible = !mCursorBlinkVisible;
            mCursorBlinkCount += 1;
            redraw();
        }
    });

}

void AAbstractTypeableView::onFocusAcquired() {
    AView::onFocusAcquired();
    updateCursorBlinking();
}

AAbstractTypeableView::~AAbstractTypeableView()
{
}

bool AAbstractTypeableView::handlesNonMouseNavigation() {
    return true;
}

int AAbstractTypeableView::getContentMinimumHeight(ALayoutDirection layout)
{
    return getFontStyle().size;
}

void AAbstractTypeableView::onKeyDown(AInput::Key key)
{
    AView::onKeyDown(key);
    onKeyRepeat(key);
}

void AAbstractTypeableView::onKeyRepeat(AInput::Key key)
{
    if (AInput::isKeyDown(AInput::LBUTTON))
        return;


    auto fastenSelection = [&]() {
        if (!AInput::isKeyDown(AInput::LSHIFT) && !AInput::isKeyDown(AInput::RSHIFT)) {
            mCursorSelection = -1;
        } else if (mCursorSelection == -1)
        {
            mCursorSelection = mCursorIndex;
        }
    };

    mTextChangedFlag = true;
    switch (key)
    {
        case AInput::DEL:
            if (hasSelection()) {
                auto sel = selection();
                typeableErase(sel.begin, sel.end);
                invalidatePrerenderedString();
                mCursorSelection = -1;
                mCursorIndex = sel.begin;
            } else
            {
                if (mCursorIndex < length())
                {
                    typeableErase(mCursorIndex, mCursorIndex + 1);
                    invalidatePrerenderedString();
                }
            }
            break;

        case AInput::LEFT:
            fastenSelection();
            if (mCursorIndex) {
                if (AInput::isKeyDown(AInput::LCONTROL)) {
                    if (mCursorIndex <= 1) {
                        mCursorIndex = 0;
                    } else {
                        mCursorIndex = typeableReverseFind(' ', mCursorIndex - 2) + 1;
                    }
                } else {
                    mCursorIndex -= 1;
                }
            }
            break;

        case AInput::RIGHT:
            fastenSelection();
            if (mCursorIndex < length()) {
                if (AInput::isKeyDown(AInput::LCONTROL)) {
                    auto index = typeableFind(' ', mCursorIndex);
                    if (index == AString::NPOS) {
                        mCursorIndex = length();
                    } else {
                        mCursorIndex = index + 1;
                    }
                } else {
                    mCursorIndex += 1;
                }
            }
            break;

        case AInput::HOME:
            fastenSelection();
            mCursorIndex = 0;
            break;
        case AInput::END:
            fastenSelection();
            mCursorIndex = length();
            break;

        default:
            if (AInput::isKeyDown(AInput::LCONTROL) || AInput::isKeyDown(AInput::RCONTROL)) {
                switch (key) {
                    case AInput::A: // select all
                        selectAll();
                        break;

                    case AInput::C: // copy
                        copyToClipboard();
                        break;

                    case AInput::X: // cut
                        cutToClipboard();
                        break;

                    case AInput::V: // paste
                        pasteFromClipboard();
                        break;

                    default:
                        return;
                }
            } else {
                return;
            }
    }

    if (textChanging) {
        emit textChanging(text());
    }

    updateCursorPos();
    updateCursorBlinking();

    redraw();
}

void AAbstractTypeableView::pasteFromClipboard() {
    auto pastePos = mCursorIndex;
    AOptional<AString> prevContents;
    if (mCursorSelection != -1) {
        prevContents = text();
        auto sel = selection();
        pastePos = sel.begin;
        typeableErase(sel.begin, sel.end);
    }
    auto toPaste = AClipboard::pasteFromClipboard();
    if (mMaxTextLength <= length())
        return;
    if (!mIsMultiline) {
        toPaste = toPaste.replacedAll("\n", "");
    }
    if (typeableInsert(pastePos, toPaste)) {
        mCursorIndex = pastePos + toPaste.length();
        mCursorSelection = -1;

        invalidatePrerenderedString();
        updateCursorPos();
        emit textChanged;
    } else if (prevContents) {
        setText(*prevContents);
    }
}

void AAbstractTypeableView::cutToClipboard() {
    if (!mIsCopyable)
        return;

    auto sel = selection();
    AClipboard::copyToClipboard(selectedText());
    typeableErase(sel.begin, sel.end);
    mCursorIndex = sel.begin;
    mCursorSelection = -1;
    invalidatePrerenderedString();
}

void AAbstractTypeableView::copyToClipboard() const {
    if (!mIsCopyable)
        return;

    AClipboard::copyToClipboard(selectedText());
}

void AAbstractTypeableView::selectAll() { ACursorSelectable::selectAll(); }

void AAbstractTypeableView::enterChar(char16_t c)
{
    if (AInput::isKeyDown(AInput::LBUTTON) ||
        AInput::isKeyDown(AInput::LCONTROL) ||
        AInput::isKeyDown(AInput::RCONTROL) ||
        c == '\t')
        return;
    if (c == '\r') {
        c = '\n';
    }

    mTextChangedFlag = true;
    auto cursorIndexCopy = mCursorIndex;

    if (hasSelection()) {
        auto sel = selection();
        typeableErase(sel.begin, sel.end);

        switch (c)
        {
            case '\b':
                mCursorIndex = sel.begin;
                break;
            default:
                if (typeableInsert(sel.begin, c)) {
                    mCursorIndex = sel.begin + 1;
                } else {
                    mCursorIndex = sel.begin;
                }
        }
        mCursorSelection = -1;
    } else {
        switch (c)
        {
            case '\b':
                if (mCursorIndex != 0) {
                    typeableErase(mCursorIndex - 1, mCursorIndex);
                    mCursorIndex -= 1;
                }
                break;
            default:
                if (mMaxTextLength <= length())
                    return;
                if (!typeableInsert(mCursorIndex, c)) {
                    return;
                }
                ++mCursorIndex;

        }
    }
    invalidatePrerenderedString();
    updateCursorBlinking();
    updateCursorPos();

    if (!AInput::isKeyDown(AInput::LSHIFT) && !AInput::isKeyDown(AInput::RSHIFT))
    {
        mCursorSelection = -1;
    }

    redraw();
}

void AAbstractTypeableView::onFocusLost()
{
    AView::onFocusLost();
    if (mTextChangedFlag)
    {
        mTextChangedFlag = false;
        if (textChanged) {
            emit textChanged(text());
        }
    }

}

void AAbstractTypeableView::onPointerPressed(const APointerPressedEvent& event)
{
    AView::onPointerPressed(event);
    ACursorSelectable::handleMousePressed(event);
    updateCursorBlinking();
}

void AAbstractTypeableView::onPointerMove(glm::vec2 pos, const APointerMoveEvent& event)
{
    AView::onPointerMove(pos, event);
    ACursorSelectable::handleMouseMove(pos);
}

bool AAbstractTypeableView::isLButtonPressed() {
    return isPressed();
}

void AAbstractTypeableView::onPointerReleased(const APointerReleasedEvent& event)
{
    AView::onPointerReleased(event);
    if (!event.triggerClick) return;

    if (event.pointerIndex != APointerIndex::button(AInput::RBUTTON)) {
        ACursorSelectable::handleMouseReleased(event);
    }
}

AMenuModel AAbstractTypeableView::composeContextMenu() {
    return { { .name = "aui.cut"_i18n, .shortcut = AInput::LCONTROL + AInput::X, .onAction = [&]{cutToClipboard();}, .enabled = hasSelection(), },
             { .name = "aui.copy"_i18n, .shortcut = AInput::LCONTROL + AInput::C, .onAction = [&]{copyToClipboard();}, .enabled = hasSelection() },
             { .name = "aui.paste"_i18n, .shortcut = AInput::LCONTROL + AInput::V, .onAction = [&]{pasteFromClipboard();}, .enabled = !AClipboard::isEmpty() },
             { .type = AMenu::SEPARATOR, },
             { .name = "aui.select_all"_i18n, .shortcut = AInput::LCONTROL + AInput::A, .onAction = [&]{selectAll();}, .enabled = !text().empty() } };
}

void AAbstractTypeableView::setText(const AString& t)
{
    mHorizontalScroll = 0;
    updateSelectionOnTextSet(t);
    updateCursorBlinking();

    invalidatePrerenderedString();
    emit textChanged(t);
}

void AAbstractTypeableView::updateSelectionOnTextSet(const AString& t) {
    mCursorIndex = t.length();
    mCursorSelection = 0;
}

void AAbstractTypeableView::onPointerDoubleClicked(const APointerPressedEvent& event) {
    AView::onPointerDoubleClicked(event);
    ACursorSelectable::handleMouseDoubleClicked(event);
    updateCursorBlinking();
}

glm::ivec2 AAbstractTypeableView::getMouseSelectionPadding() {
    return {mPadding.left, mPadding.top + getVerticalAlignmentOffset() };
}

glm::ivec2 AAbstractTypeableView::getMouseSelectionScroll() {
    return {mHorizontalScroll, 0};
}

AFontStyle AAbstractTypeableView::getMouseSelectionFont() {
    return getFontStyle();
}

AString AAbstractTypeableView::getDisplayText() {
    return text();
}


void AAbstractTypeableView::doRedraw() {
    redraw();
}

void AAbstractTypeableView::onSelectionChanged() {
    if (selectionChanged) emit selectionChanged(selection());
}
