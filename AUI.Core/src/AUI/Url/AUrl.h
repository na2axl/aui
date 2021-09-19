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
#include "AUI/Common/SharedPtr.h"
#include "AUI/IO/IInputStream.h"
#include <AUI/Common/AMap.h>

class API_AUI_CORE AUrl
{
private:
	AString mProtocol;
	AString mHost;
	AString mPath;


    static AMap<AString, std::function<_<IInputStream>(const AUrl&)>> ourResolvers;

public:
	AUrl(const AString& full);
	inline AUrl(const char* full): AUrl(AString(full)) {}

	_<IInputStream> open() const;
	
	const AString& getHost() const
	{
		return mHost;
	}
	const AString& getPath() const
	{
		return mPath;
	}
	const AString& getProtocol() const
	{
		return mProtocol;
	}

	AString getFull() const {
	    return mProtocol + "://" + mHost + "/" + mPath;
	}

    bool operator<(const AUrl& u) const {
        return getFull() < u.getFull();
    }

	static void registerResolver(const AString& protocol, const std::function<_<IInputStream>(const AUrl&)>& factory);
};



inline AUrl operator"" _url(const char* input, size_t s)
{
    return AUrl({input, input + s});
}