#ifndef __ALERTVIEW_H
#define __ALERTVIEW_H

#include <String.h>
#include <View.h>

class AlertView : public BView
{
public:
	AlertView(BRect frame, char *name);
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	uint32 count;
	
private:	
	BBitmap	*fBitmap;
	BString	fString;
};

#endif
