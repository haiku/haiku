#ifndef TIME_H
#define TIME_H

#include <Application.h>

#include "TimeWindow.h"
#include "TimeSettings.h"

class TimeApplication : public BApplication 
{
public:
	TimeApplication();
	virtual ~TimeApplication();
	
	void MessageReceived(BMessage *message);

	void ReadyToRun(void);
	void AboutRequested(void);
	
	void SetWindowCorner(BPoint corner);
	BPoint WindowCorner() const { return f_settings->WindowCorner(); }
private:
	TimeSettings*		f_settings;
	TTimeWindow*		f_window;
};

#endif //TIME_H
