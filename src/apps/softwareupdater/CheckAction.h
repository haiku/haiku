/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef CHECK_ACTION_H
#define CHECK_ACTION_H


#include "CheckManager.h"


class CheckAction {
public:
								CheckAction();
								~CheckAction();
		status_t				Perform();

private:
		CheckManager*			fCheckManager;
};


#endif // CHECK_ACTION_H
