#ifndef VIEW_H
#define VIEW_H

#include "SubPath.h"
#include <View.h>

class BBView : public BView {
	SubPath fPath;
	enum {
		kDrawOutline,
		kStroke
	} fMode;
	int fCurPoint;
	float fWidth;
	
public:
	BBView(BRect rect);
	void Draw(BRect updateRect);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void SetClose(bool close);
};
#endif
