#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Window.h>

/////////////////////////////////
#include "SettingsView.h"
#include "TimeSettings.h"
#include "ZoneView.h"

class TimeWindow : public BWindow 
{
public:
	TimeWindow();
	~TimeWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
	void BuildViews();
	
private:
	SettingsView 	*fTimeSettings;
	ZoneView		*fTimeZones;
};

#endif

