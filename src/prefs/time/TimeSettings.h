#ifndef TIME_SETTINGS_H_
#define TIME_SETTINGS_H_

#include <SupportDefs.h>

class TimeSettings {
	public :
		TimeSettings();
		~TimeSettings();
	
		BPoint WindowCorner() const { return fCorner; }
		void SetWindowCorner(BPoint corner);
	private:
		static const char kTimeSettingsFile[];
		BPoint fCorner;

};

#endif //TIME_SETTINGS_H_


