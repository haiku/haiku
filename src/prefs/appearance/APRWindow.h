#ifndef APR_WINDOW_H
#define APR_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <Message.h>

class APRWindow : public BWindow 
{
public:
	APRWindow(BRect frame); 
	virtual	bool QuitRequested();
};

#endif
