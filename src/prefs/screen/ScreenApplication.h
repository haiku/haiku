#ifndef __SCREENAPPLICATION_H
#define __SCREENAPPLICATION_H

#include "ScreenWindow.h"

class ScreenApplication : public BApplication
{
public:
	ScreenApplication();
	virtual void MessageReceived(BMessage *message);
	virtual void AboutRequested();
private:
	ScreenWindow *fScreenWindow;
};

#endif
