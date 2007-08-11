/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _PICTURE_TEST_CASES_H
#define _PICTURE_TEST_CASES_H

#include "PictureTest.h"

typedef struct {
	const char *name;
	draw_func *func;
} TestCase;

// test NULL-terminated array of test cases
extern TestCase gTestCases[];

#endif
