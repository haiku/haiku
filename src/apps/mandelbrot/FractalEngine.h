/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		kerwizzy
 */
#ifndef FRACTALENGINE_H
#define FRACTALENGINE_H

#include <SupportDefs.h>
#include <Bitmap.h>


BBitmap* FractalEngine(uint32 width, uint32 height, double locationX,
	double locationY, double size);


#endif	/* FRACTALENGINE_H */
