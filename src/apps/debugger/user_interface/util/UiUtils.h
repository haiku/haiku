/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UI_UTILS_H
#define UI_UTILS_H


#include "Thread.h"


class UiUtils {
public:
	static	const char*			ThreadStateToString(int state,
									int stoppedReason);
};


#endif	// UI_UTILS_H
