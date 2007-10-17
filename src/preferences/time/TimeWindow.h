/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 *
 */
#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H


#include <Window.h>


class BMessage;
class DateTimeView;
class TTimeBaseView;
class TZoneView;
class SettingsView;


class TTimeWindow : public BWindow {
	public:
						TTimeWindow(const BPoint topLeft);
		virtual			~TTimeWindow() {}

		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage *message);

	private:
		void 			_InitWindow();

		TTimeBaseView 	*fBaseView;
		DateTimeView 	*fDateTimeView;
		TZoneView 		*fTimeZones;
		SettingsView	*fSettingsView;
};

#endif	// TIME_WINDOW_H

