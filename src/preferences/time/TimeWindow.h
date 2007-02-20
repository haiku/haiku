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
