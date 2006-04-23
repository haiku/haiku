#ifndef __CONTROLLER_VIEW_H
#define __CONTROLLER_VIEW_H

#include <View.h>


class Controller;

class ControllerView : public BView
{
public:
					ControllerView(BRect frame, Controller *ctrl);
					~ControllerView();
private:
	void			AttachedToWindow();
	void			MessageReceived(BMessage *msg);
	void			Draw(BRect updateRect);
	
private:
	Controller *	fController;
};

#endif
