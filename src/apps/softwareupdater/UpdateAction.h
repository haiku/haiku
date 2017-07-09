/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 */
#ifndef UPDATE_ACTION_H
#define UPDATE_ACTION_H


#include "UpdateManager.h"


class UpdateAction {
public:
								UpdateAction(bool verbose);
								~UpdateAction();
		status_t				Perform(update_type action_request);

private:
		UpdateManager*			fUpdateManager;
		bool					fVerbose;
};


#endif // UPDATE_ACTION_H
