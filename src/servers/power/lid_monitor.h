/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */
#ifndef _LID_MONITOR_H
#define _LID_MONITOR_H


class LidMonitor {
public:
								LidMonitor();
			 					~LidMonitor();

			void				HandleEvent();

			int					FD() const { return fFD; }
private:
			int					fFD;
};


#endif // _LID_MONITOR_H
