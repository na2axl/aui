﻿#pragma once
#include <cstdio>
#include "IOutputStream.h"
#include "AUI/Core.h"

class AString;

class API_AUI_CORE FileOutputStream : public IOutputStream
{
private:
	FILE* mFile;

public:
	FileOutputStream(const AString& path, bool append = false);
	virtual ~FileOutputStream();

	int write(const char* src, int size) override;
	void close();
};
