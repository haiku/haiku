#ifndef SCREENVIEW_H
#define SCREENVIEW_H

#include <View.h>

class ScreenView : public BView
{

public:
						ScreenView(BRect frame, char *name);
	virtual void		AttachedToWindow();
	virtual void		Draw(BRect updateRect);
};

#endif