/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */
#ifndef _POWER_BUTTON_MONITOR_H
#define _POWER_BUTTON_MONITOR_H


#include <Handler.h>


#define POLL_POWER_BUTTON_STATUS 'ppbs'


class PowerButtonMonitor : public BHandler {
public:
								PowerButtonMonitor();
	virtual 					~PowerButtonMonitor();

	virtual void				MessageReceived(BMessage *msg);

private:
			int					fPowerButtonFD;
};


#endif // _POWER_BUTTON_MONITOR_H
