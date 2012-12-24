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
class BTabView;
class ClockView;
class DateTimeView;
class NetworkTimeView;
class TimeZoneView;
class TTimeBaseView;


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

			BTabView*			fTabView;
			DateTimeView*		fDateTimeView;
			TimeZoneView*		fTimeZoneView;
			NetworkTimeView*	fNetworkTimeView;
			ClockView*			fClockView;

			BButton*			fRevertButton;
};


#endif	// _TIME_WINDOW_H

