/*
 * Copyright 2005-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Rene Gollent
 *		Nathan Whitehorn
 */
#ifndef _POWER_BUTTON_MONITOR_H
#define _POWER_BUTTON_MONITOR_H


#include "power_monitor.h"


class PowerButtonMonitor : public PowerMonitor {
public:
								PowerButtonMonitor();
	virtual	 					~PowerButtonMonitor();

	virtual void				HandleEvent(int fd);

	virtual const std::set<int>&
								FDs() const { return fFDs; }
private:
			std::set<int>		fFDs;
};


#endif // _POWER_BUTTON_MONITOR_H
