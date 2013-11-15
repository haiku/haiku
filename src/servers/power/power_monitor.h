/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de.
 */
#ifndef _POWER_MONITOR_H
#define _POWER_MONITOR_H


class PowerMonitor {
public:
	virtual	 					~PowerMonitor() {};

	virtual	void				HandleEvent() = 0;

	virtual	int					FD() const = 0;
};


#endif // _POWER_MONITOR_H
