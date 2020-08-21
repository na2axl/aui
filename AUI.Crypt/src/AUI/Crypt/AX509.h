﻿#pragma once
#include "AUI/Crypt.h"
#include "AUI/Common/ByteBuffer.h"

class API_AUI_CRYPT AX509
{
	friend class AX509Store;
private:
	void* mX509;

	explicit AX509(void* x509);

public:
	~AX509();

	bool isCA() const;
	bool checkHost(const AString& name) const;
	bool checkTrust() const;
	
	static _<AX509> fromPEM(_<ByteBuffer> buffer);
};
