#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Window.h>

/////////////////////////////////
#include "SettingsView.h"
#include "TimeSettings.h"
#include "ZoneView.h"
#include "BaseView.h"

class TTimeWindow : public BWindow 
{
	public:
		TTimeWindow();
		~TTimeWindow();
	
		bool QuitRequested();
		void MessageReceived(BMessage *message);

	private:
		void InitWindow();

	private:
		TTimeBaseView	*f_BaseView;
		TSettingsView 	*fTimeSettings;
		TZoneView		*fTimeZones;
};

#endif
