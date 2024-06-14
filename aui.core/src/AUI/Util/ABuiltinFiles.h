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

#include "AUI/Core.h"
#include "AUI/Common/AByteBuffer.h"
#include "AUI/Common/AMap.h"
#include "AUI/Common/SharedPtr.h"
#include "AUI/IO/IInputStream.h"
#include <optional>

class AString;

class API_AUI_CORE ABuiltinFiles
{
private:
	AMap<AString, AByteBuffer> mBuffers;

	static ABuiltinFiles& inst();
	ABuiltinFiles() = default;

public:
	static void loadBuffer(AByteBuffer& data);
	static void load(const unsigned char* data, size_t size);
	static _<IInputStream> open(const AString& file);
    static AOptional<AByteBufferView> getBuffer(const AString& file);
    static bool contains(const AString& file);
};
