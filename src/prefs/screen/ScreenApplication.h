#ifndef SCREENAPPLICATION_H
#define SCREENAPPLICATION_H

#include "ScreenWindow.h"

class ScreenApplication : public BApplication
{

public:
					ScreenApplication();
	virtual void	MessageReceived(BMessage *message);
	virtual void	AboutRequested();
	
private:
	ScreenWindow	*fScreenWindow;
	bool			NeedWindow;
};

#endif