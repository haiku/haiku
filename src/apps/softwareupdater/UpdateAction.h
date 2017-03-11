/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef UPDATE_ACTION_H
#define UPDATE_ACTION_H


#include "UpdateManager.h"


class UpdateAction {
public:
								UpdateAction();
								~UpdateAction();
		status_t				Perform();

private:

		UpdateManager*			fUpdateManager;
};


#endif // UPDATE_ACTION_H
