/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg (inseculous)
 *		Julun <host.haiku@gmx.de>
 */
#ifndef TIME_H
#define TIME_H


#include <Application.h>


class BMessage;
class TTimeWindow;


class TimeApplication : public BApplication {
	public:
						TimeApplication();
		virtual 		~TimeApplication();

		virtual void 	MessageReceived(BMessage* message);
		virtual void 	ReadyToRun();
		virtual void 	AboutRequested();

	private:
		TTimeWindow 	*fWindow;
};

#endif	// TIME_H

