/*
 * Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
 *   Digital Ltd. LLC
 * Distributed under the terms of the MIT License.
 */

#include "EXRGamma.h"
#include "ImathFun.h"
#include "ImathMath.h"

#include <algorithm>

using namespace std;
using Imath::clamp;


float
knee(double x, double f)
{
	return float (Imath::Math<double>::log (x * f + 1) / f);
}


float
findKneeF(float x, float y)
{
	float f0 = 0;
	float f1 = 1;
	
	while (knee (x, f1) > y)
	{
	f0 = f1;
	f1 = f1 * 2;
	}
	
	for (int i = 0; i < 30; ++i)
	{
	float f2 = (f0 + f1) / 2;
	float y2 = knee (x, f2);
	
	if (y2 < y)
		f1 = f2;
	else
		f0 = f2;
	}
	
	return (f0 + f1) / 2;
}


Gamma::Gamma(float gamma,
	float exposure,
	float defog,
	float kneeLow,
	float kneeHigh)
:
	g (gamma),
	m (Imath::Math<float>::pow(2, exposure + 2.47393)),
	d (defog),
	kl (Imath::Math<float>::pow(2, kneeLow)),
	f (findKneeF (Imath::Math<float>::pow(2, kneeHigh) - kl, 
		Imath::Math<float>::pow(2, 3.5) - kl)),
	s (255.0 * Imath::Math<float>::pow(2, -3.5 * g))
{
}


float
Gamma::operator() (half h)
{
	//
	// Defog
	//
	
	float x = max (0.f, (h - d));
	
	//
	// Exposure
	//
	
	x *= m;
	
	//
	// Knee
	//
	
	if (x > kl)
	x = kl + knee (x - kl, f);
	
	//
	// Gamma
	//
	
	x = Imath::Math<float>::pow (x, g);
	
	//
	// Scale and clamp
	//
	
	return clamp (x * s, 0.f, 255.f);
}
