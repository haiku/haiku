/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */
#ifndef _LID_MONITOR_H
#define _LID_MONITOR_H


#include "power_monitor.h"


class LidMonitor : public PowerMonitor {
public:
								LidMonitor();
	virtual	 					~LidMonitor();

	virtual	void				HandleEvent(int fd);

	virtual	const std::set<int>&
								FDs() const { return fFDs; }
private:
			std::set<int>		fFDs;
};


#endif // _LID_MONITOR_H
