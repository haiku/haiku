#ifndef REFRESHVIEW_H
#define REFRESHVIEW_H

#include <View.h>

class RefreshView : public BView
{

public:
						RefreshView(BRect frame, char *name);
	virtual void		AttachedToWindow();
	virtual void		Draw(BRect updateRect);
};

#endif