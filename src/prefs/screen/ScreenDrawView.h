#ifndef SCREENDRAWVIEW_H
#define SCREENDRAWVIEW_H

#include <View.h>

class ScreenDrawView : public BView
{

public:
						ScreenDrawView(BRect frame, char *name);
	virtual void		AttachedToWindow();
	virtual void		Draw(BRect updateRect);
	virtual void		MessageReceived(BMessage *message);
	virtual void		MouseDown(BPoint point);

private:
	BScreen				*fScreen;
	rgb_color			desktopColor;
	int32				fResolution;
};

#endif