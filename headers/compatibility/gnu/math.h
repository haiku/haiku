/*
 * Copyright 2018-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_MATH_H_
#define _GNU_MATH_H_


#include_next <math.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif


void sincos(double x, double *sin, double *cos);
void sincosf(float x, float *sin, float *cos);
void sincosl(long double x, long double *sin, long double *cos);


double exp10(double);
float exp10f(float);
long double exp10l(long double);


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_MATH_H_ */
