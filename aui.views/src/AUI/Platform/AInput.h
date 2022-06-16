﻿/**
 * =====================================================================================================================
 * Copyright (c) 2021 Alex2772
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 * Original code located at https://github.com/aui-framework/aui
 * =====================================================================================================================
 */

#pragma once
#include "AUI/Common/AString.h"
#include "AUI/Util/EnumUtil.h"

namespace AInput
{
	enum Key {
        /* Based on SFML */
		UNKNOWN = -1, ///< Unhandled key
		A = 0,        ///< The A key
		B,            ///< The B key
		C,            ///< The C key
		D,            ///< The D key
		E,            ///< The E key
		F,            ///< The F key
		G,            ///< The G key
		H,            ///< The H key
		I,            ///< The I key
		J,            ///< The J key
		K,            ///< The K key
		L,            ///< The L key
		M,            ///< The M key
		N,            ///< The N key
		O,            ///< The O key
		P,            ///< The P key
		Q,            ///< The Q key
		R,            ///< The R key
		S,            ///< The S key
		T,            ///< The T key
		U,            ///< The U key
		V,            ///< The V key
		W,            ///< The W key
		X,            ///< The X key
		Y,            ///< The Y key
		Z,            ///< The Z key
		NUM0,         ///< The 0 key
		NUM1,         ///< The 1 key
		NUM2,         ///< The 2 key
		NUM3,         ///< The 3 key
		NUM4,         ///< The 4 key
		NUM5,         ///< The 5 key
		NUM6,         ///< The 6 key
		NUM7,         ///< The 7 key
		NUM8,         ///< The 8 key
		NUM9,         ///< The 9 key
		ESCAPE,       ///< The Escape key
		LCONTROL,     ///< The left Control key
		LSHIFT,       ///< The left Shift key
		LALT,         ///< The left Alt key
		LSYSTEM,      ///< The left OS specific key: window (Windows and Linux), apple (MacOS X), ...
		RCONTROL,     ///< The right Control key
		RSHIFT,       ///< The right Shift key
		RALT,         ///< The right Alt key
		RSYSTEM,      ///< The right OS specific key: window (Windows and Linux), apple (MacOS X), ...
		MENU,         ///< The Menu key
		LBRACKET,     ///< The [ key
		RBRACKET,     ///< The ] key
		SEMICOLON,    ///< The ; key
		COMMA,        ///< The , key
		PERIOD,       ///< The . key
		QUOTE,        ///< The ' key
		SLASH,        ///< The / key
		BACKSLASH,    ///< The \ key
		TILDE,        ///< The ~ key
		EQUAL,        ///< The = key
		DASH,         ///< The - key
		SPACE,        ///< The Space key
		RETURN,       ///< The Return key
		BACKSPACE,    ///< The Backspace key
		TAB,          ///< The Tabulation key
		PAGEUP,       ///< The Page up key
		PAGEDOWN,     ///< The Page down key
		END,          ///< The End key
		HOME,         ///< The Home key
		INSERT,       ///< The Insert key
		DEL,          ///< The Delete key
		ADD,          ///< The + key
		SUBTRACT,     ///< The - key
		MULTIPLY,     ///< The * key
		DIVIDE,       ///< The / key
		LEFT,         ///< Left arrow
		RIGHT,        ///< Right arrow
		UP,           ///< Up arrow
		DOWN,         ///< Down arrow
		NUMPAD0,      ///< The numpad 0 key
		NUMPAD1,      ///< The numpad 1 key
		NUMPAD2,      ///< The numpad 2 key
		NUMPAD3,      ///< The numpad 3 key
		NUMPAD4,      ///< The numpad 4 key
		NUMPAD5,      ///< The numpad 5 key
		NUMPAD6,      ///< The numpad 6 key
		NUMPAD7,      ///< The numpad 7 key
		NUMPAD8,      ///< The numpad 8 key
		NUMPAD9,      ///< The numpad 9 key
		F1,           ///< The F1 key
		F2,           ///< The F2 key
		F3,           ///< The F3 key
		F4,           ///< The F4 key
		F5,           ///< The F5 key
		F6,           ///< The F6 key
		F7,           ///< The F7 key
		F8,           ///< The F8 key
		F9,           ///< The F9 key
		F10,          ///< The F10 key
		F11,          ///< The F11 key
		F12,          ///< The F12 key
		F13,          ///< The F13 key
		F14,          ///< The F14 key
		F15,          ///< The F15 key
		PAUSE,        ///< The Pause key
		LBUTTON,      /// Left Mouse Button
		CBUTTON,      /// Center Mouse Button
		RBUTTON,      /// Right Mouse Button
		KEYCOUNT      ///< Keep last -- the total number of keyboard keys
	};
	API_AUI_VIEWS Key fromNative(int w);
    API_AUI_VIEWS int toNative(Key w);
    API_AUI_VIEWS AString getName(Key k);
    API_AUI_VIEWS bool isKeyDown(Key k);
};
