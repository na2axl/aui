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

#pragma once

#include <AUI/IO/IOutputStream.h>
#include "Pipe.h"

class PipeOutputStream: public IOutputStream {
public:
    explicit PipeOutputStream(Pipe pipe);
    ~PipeOutputStream();

    void write(const char* src, size_t size) override;

private:
    Pipe mPipe;

#if AUI_PLATFORM_UNIX
    FILE* mFileHandle;
#endif
};
