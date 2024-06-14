﻿/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2024 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "MinesweeperWindow.h"
#include "AUI/Platform/AWindow.h"
#include "AUI/View/ANumberPicker.h"
#include "AUI/View/ALabel.h"

class NewGameWindow: public AWindow
{
private:
	MinesweeperWindow* mMinesweeper;
	_<ANumberPicker> mWidth;
	_<ANumberPicker> mHeight;
	_<ANumberPicker> mMines;
	_<ALabel> mDifficultyLabel;

	void updateMinesMax();
	void updateDifficultyLabel();
	void begin();

public:
	NewGameWindow(MinesweeperWindow* minesweeper);
};
