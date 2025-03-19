/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <BeBuild.h>
#include <math.h>


extern int			__fpclassifyf(float value);
extern int			__fpclassify(double x);
extern int			__fpclassifyl(long double value);


// #pragma mark - finite


int
__finitef(float value)
{
	return __fpclassifyf(value) > FP_INFINITE;
}


int
__finite(double value)
{
	return __fpclassify(value) > FP_INFINITE;
}


int
__finitel(long double value)
{
	return __fpclassifyl(value) > FP_INFINITE;
}


B_DEFINE_WEAK_ALIAS(__finitef, finitef);
B_DEFINE_WEAK_ALIAS(__finite, finite);
B_DEFINE_WEAK_ALIAS(__finitel, finitel);


// #pragma mark - nan


int
__isnanf(float value)
{
	return __fpclassifyf(value) == FP_NAN;
}


int
__isnan(double value)
{
	return __fpclassify(value) == FP_NAN;
}


int
__isnanl(long double value)
{
	return __fpclassifyl(value) == FP_NAN;
}


B_DEFINE_WEAK_ALIAS(__isnanf, isnanf);
B_DEFINE_WEAK_ALIAS(__isnan, isnan);
B_DEFINE_WEAK_ALIAS(__isnanl, isnanl);


// #pragma mark - inf


int
__isinff(float value)
{
	return __fpclassifyf(value) == FP_INFINITE;
}


int
__isinf(double value)
{
	return __fpclassify(value) == FP_INFINITE;
}


int
__isinfl(long double value)
{
	return __fpclassifyl(value) == FP_INFINITE;
}


B_DEFINE_WEAK_ALIAS(__isinff, isinff);
B_DEFINE_WEAK_ALIAS(__isinf, isinf);
B_DEFINE_WEAK_ALIAS(__isinfl, isinfl);
