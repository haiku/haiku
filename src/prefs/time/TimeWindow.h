#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Window.h>

#include "BaseView.h"
#include "SettingsView.h"
#include "TimeSettings.h"
#include "ZoneView.h"

class TTimeWindow : public BWindow {
	public:
		TTimeWindow();
		~TTimeWindow();
	
		bool QuitRequested();
		void MessageReceived(BMessage *message);
	private:
		void InitWindow();

		TTimeBaseView *f_BaseView;
		TSettingsView *f_TimeSettings;
		TZoneView *f_TimeZones;
};

#endif
