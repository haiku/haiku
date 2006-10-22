/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg
 */
#ifndef TIME_H
#define TIME_H


#include "TimeSettings.h"
#include "TimeWindow.h"

#include <Application.h>


class TimeApplication : public BApplication {
	public:
		TimeApplication();
		virtual ~TimeApplication();

		void MessageReceived(BMessage* message);

		void ReadyToRun();
		void AboutRequested();

		void SetWindowCorner(BPoint corner);
		BPoint WindowCorner() const 
			{ return fSettings->WindowCorner(); }

	private:
		TimeSettings *fSettings;
		TTimeWindow *fWindow;
};

#endif	// TIME_H
