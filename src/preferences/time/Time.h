/*
 * Copyright 2002-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef _TIME_H
#define _TIME_H


#include <Application.h>


class TTimeWindow;


class TimeApplication : public BApplication {
public:
								TimeApplication();
	virtual						~TimeApplication();

	virtual void				ReadyToRun();
	virtual void				AboutRequested();

private:
			TTimeWindow*		fWindow;
};


#endif	// _TIME_H

