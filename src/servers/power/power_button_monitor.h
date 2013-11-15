/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */
#ifndef _POWER_BUTTON_MONITOR_H
#define _POWER_BUTTON_MONITOR_H


#include "power_monitor.h"


class PowerButtonMonitor : public PowerMonitor {
public:
								PowerButtonMonitor();
	virtual	 					~PowerButtonMonitor();

	virtual void				HandleEvent();

	virtual int					FD() const { return fFD; }
private:
			int					fFD;
};


#endif // _POWER_BUTTON_MONITOR_H
