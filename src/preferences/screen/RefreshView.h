#ifndef __REFRESHVIEW_H
#define __REFRESHVIEW_H

#include <Box.h>

class RefreshView : public BBox
{
public:
	RefreshView(BRect frame, char *name);
	virtual void Draw(BRect updateRect);
};

#endif
