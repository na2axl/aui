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
 
 * Original code located at https://github.com/Alex2772/aui
 * =====================================================================================================================
 */

#pragma once

#include <cstdint>

class AModelIndex
{
private:
	int mRow;
	int mColumn;
	
public:
	AModelIndex(int row, int column)
		: mRow(row),
		  mColumn(column)
	{
	}

	AModelIndex(int row)
		: mRow(row)
	{
	}

	AModelIndex() = default;

	int getRow() const
	{
		return mRow;
	}

	int getColumn() const
	{
		return mColumn;
	}

	inline bool operator==(const AModelIndex& other) const {
	    return mRow == other.mRow && mColumn == other.mColumn;
	}
	inline bool operator!=(const AModelIndex& other) const {
	    return mRow != other.mRow || mColumn != other.mColumn;
	}
    inline bool operator<(const AModelIndex& other) const {
	    return hash() < other.hash();
	}

	[[nodiscard]] inline uint64_t hash() const {
	    uint64_t hash = uint32_t(mRow);
	    hash <<= 32u;
	    hash |= uint32_t(mColumn);
	    return hash;
	}
};
