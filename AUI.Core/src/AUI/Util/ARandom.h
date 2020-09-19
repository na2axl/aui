﻿#pragma once
#include <random>
#include <glm/glm.hpp>
#include "AUI/Core.h"
#include "AUI/Common/AByteBuffer.h"

class API_AUI_CORE ARandom
{
private:
	std::default_random_engine mRandom;

public:
	ARandom();

	int nextInt();
	float nextFloat();
	float nextFloat(float min, float max);
	glm::vec2 nextVec2();
	glm::vec2 nextVec2(float min, float max);

	_<AByteBuffer> nextBytes(unsigned count);
};
