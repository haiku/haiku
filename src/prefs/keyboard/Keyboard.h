#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Application.h>

#include "KeyboardWindow.h"
#include "KeyboardSettings.h"

class KeyboardApplication : public BApplication 
{
public:
	KeyboardApplication();
	virtual ~KeyboardApplication();
	
	void MessageReceived(BMessage *message);
	BPoint WindowCorner() const {return fSettings->WindowCorner(); }
	void SetWindowCorner(BPoint corner);
	int32 KeyboardRepeatRate() const {return fSettings->KeyboardRepeatRate(); }
	void SetKeyboardRepeatRate(int32 rate);
	int32 KeyboardRepeatDelay() const {return fSettings->KeyboardRepeatDelay(); }
	void SetKeyboardRepeatDelay(int32 rate);

	void AboutRequested(void);
	
private:
	
	static const char kKeyboardApplicationSig[];

	KeyboardSettings		*fSettings;

};

#endif