/*
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 */
#ifndef _CLOCK_APPLICATION_H
#define _CLOCK_APPLICATION_H


#include <Application.h>


class TClockWindow;


class THelloApplication : public BApplication {
	public:
						THelloApplication();
		virtual			~THelloApplication();

		virtual	void	MessageReceived(BMessage *msg);

	private:
		TClockWindow	*myWindow;
};


extern const char *kAppSignature;


#endif	// _CLOCK_APPLICATION_H

