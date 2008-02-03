/*
 * Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
 *   Digital Ltd. LLC
 * Distributed under the terms of the MIT License.
 */
#ifndef EXR_GAMMA_H
#define EXR_GAMMA_H

#include "halfFunction.h"

struct Gamma
{
	float g, m, d, kl, f, s;
	
	Gamma(float gamma,
		float exposure,
		float defog,
		float kneeLow,
		float kneeHigh);
	
	float operator() (half h);
};

#endif
