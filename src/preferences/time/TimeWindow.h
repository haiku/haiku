/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef _TIME_WINDOW_H
#define _TIME_WINDOW_H


#include <Window.h>


class BMessage;
class DateTimeView;
class TTimeBaseView;
class TimeZoneView;
class NetworkTimeView;


class TTimeWindow : public BWindow {
public:
								TTimeWindow();
	virtual						~TTimeWindow();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_InitWindow();
			void				_AlignWindow();
			void				_SendTimeChangeFinished();
			void				_SetRevertStatus();

			TTimeBaseView*		fBaseView;
			DateTimeView*		fDateTimeView;
			TimeZoneView*		fTimeZoneView;
			NetworkTimeView*	fNetworkTimeView;
			BButton*			fRevertButton;
};


#endif	// _TIME_WINDOW_H

