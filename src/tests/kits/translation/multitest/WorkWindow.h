// WorkWindow.h

#ifndef WORK_WINDOW_H
#define WORK_WINDOW_H

#include <Window.h>

class WorkWindow : public BWindow {
public:
	WorkWindow(BRect rect);
	
	virtual void MessageReceived(BMessage *pMsg);
};

#endif