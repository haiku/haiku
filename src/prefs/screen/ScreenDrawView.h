#ifndef __SCREENDRAWVIEW_H
#define __SCREENDRAWVIEW_H

#include <Box.h>

class ScreenDrawView : public BBox
{
public:
	ScreenDrawView(BRect frame, char *name);
	~ScreenDrawView();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);
	virtual void MouseDown(BPoint point);

private:
	rgb_color desktopColor;
	int32 fWidth;
	int32 fHeight;
};

#endif
