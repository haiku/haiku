#ifndef MOUSE_H
#define MOUSE_H

#include <Application.h>

#include "MouseWindow.h"
#include "MouseSettings.h"

class MouseApplication : public BApplication 
{
public:
	MouseApplication();
	virtual ~MouseApplication();
	
	void MessageReceived(BMessage *message);
	BPoint WindowCorner() const {return fSettings->WindowCorner(); }
	void SetWindowCorner(BPoint corner);
	int32 MouseType() const {return fSettings->MouseType(); }
	void SetMouseType(mouse_type type);
	bigtime_t ClickSpeed() const {return fSettings->ClickSpeed(); }
	void SetClickSpeed(bigtime_t click_speed);
	int32 MouseSpeed() const {return fSettings->MouseSpeed(); }
	void SetMouseSpeed(int32 speed);

	void AboutRequested(void);
	
private:
	
	static const char kMouseApplicationSig[];

	MouseSettings		*fSettings;

};

#endif