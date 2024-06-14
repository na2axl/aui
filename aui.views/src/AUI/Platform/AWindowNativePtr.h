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
// Created by Alex2772 on 12/7/2021.
//

#pragma once

#if AUI_PLATFORM_WIN
#include <Windows.h>
using AWindowNativePtr = HWND;

#elif AUI_PLATFORM_LINUX

using AWindowNativePtr = unsigned long;

#elif AUI_PLATFORM_APPLE

using AWindowNativePtr = void*;

#elif AUI_PLATFORM_ANDROID

#include <jni.h>
using AWindowNativePtr = jobject;

#endif
