/*
 * Copyright 2009 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */
#ifndef GTF_H
#define GTF_H


#include <Accelerant.h>


void ComputeGTFVideoTiming(int width, int lines, double refreshRate,
	display_timing& modeTiming);

#endif	// GTF_H
