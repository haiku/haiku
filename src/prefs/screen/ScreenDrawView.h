#ifndef __SCREENDRAWVIEW_H
#define __SCREENDRAWVIEW_H

#include <View.h>

class ScreenDrawView : public BView
{
public:
	ScreenDrawView(BRect frame, char *name);
	~ScreenDrawView();
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);
	virtual void MouseDown(BPoint point);

private:
	rgb_color desktopColor;
	int32 fWidth;
	int32 fHeight;
	BBitmap *fScreen1,
			*fScreen2;
};

#endif
