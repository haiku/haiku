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
	BPoint WindowCorner() const {return fSettings->WindowCorner(); }
	void SetWindowCorner(BPoint corner);

	void AboutRequested(void);
	
private:
	
	static const char kTimeApplicationSig[];

	TimeSettings		*fSettings;

};

#endif


