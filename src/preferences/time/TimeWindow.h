/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		mccall@@digitalparadise.co.uk
 *		Julun <host.haiku@gmx.de>
 */
#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H


#include <Window.h>


class BMessage;
class TSettingsView;
class TTimeBaseView;
class TZoneView;


class TTimeWindow : public BWindow {
	public:
						TTimeWindow(const BPoint topLeft);
		virtual			~TTimeWindow() {}

		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage *message);

	private:
		void 			_InitWindow();

		TTimeBaseView 	*fBaseView;
		TSettingsView 	*fTimeSettings;
		TZoneView 		*fTimeZones;
};

#endif	// TIME_WINDOW_H

