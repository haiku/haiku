/*
 * Copyright 2013-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de.
 *		Rene Gollent, rene@gollent.com.
 */
#ifndef _POWER_MONITOR_H
#define _POWER_MONITOR_H


#include <set>


class PowerMonitor {
public:
	virtual	 					~PowerMonitor() {};

	virtual	void				HandleEvent(int fd) = 0;

	virtual	const std::set<int>&
								FDs() const = 0;
};


#endif // _POWER_MONITOR_H
