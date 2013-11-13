/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */
#ifndef _POWER_BUTTON_MONITOR_H
#define _POWER_BUTTON_MONITOR_H


class PowerButtonMonitor {
public:
								PowerButtonMonitor();
			 					~PowerButtonMonitor();

			void				HandleEvent();

			int					FD() const { return fFD; }
private:
			int					fFD;
};


#endif // _POWER_BUTTON_MONITOR_H
