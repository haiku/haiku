#ifndef MOUSE_WINDOW_H
#define MOUSE_WINDOW_H

#include <Window.h>

#include "MouseSettings.h"
#include "MouseView.h"

class MouseWindow : public BWindow 
{
public:
	MouseWindow();
	~MouseWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
	void BuildView();
	
private:
	MouseView	*fView;

};

#endif
