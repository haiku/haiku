#ifndef KEYBOARD_WINDOW_H
#define KEYBOARD_WINDOW_H

#include <Window.h>

#include "KeyboardSettings.h"
#include "KeyboardView.h"

class KeyboardWindow : public BWindow 
{
public:
	KeyboardWindow();
	~KeyboardWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
	void BuildView();
	
private:
	KeyboardView	*fView;

};

#endif
