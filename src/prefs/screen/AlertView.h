#ifndef ALERTVIEW_H
#define ALERTVIEW_H

#include <String.h>
#include <View.h>

class AlertView : public BView
{

public:
					AlertView(BRect frame, char *name);
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect updateRect);
	int32			Count;
	
private:
	BBitmap			*fBitmap;
	BString			fString;
};

#endif