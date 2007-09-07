/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Window.h>

class TSettingsView;
class TTimeBaseView;
class TZoneView;
class TTimeWindow : public BWindow {
	public:
		TTimeWindow();
	
		bool QuitRequested();
		virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow();

		TTimeBaseView *fBaseView;
		TSettingsView *fTimeSettings;
		TZoneView *fTimeZones;
};

#endif
