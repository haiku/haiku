/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MOUSEAREAVIEW_H
#define MOUSEAREAVIEW_H

#include <View.h>
#include "ScreenSaverPrefs.h"

class MouseAreaView : public BView
{
public:	
	MouseAreaView(BRect frame, const char *name);

	virtual void Draw(BRect update); 
	virtual void MouseUp(BPoint point);
	void DrawArrow();
	inline arrowDirection getDirection(void) { return fCurrentDirection;}
	void setDirection(arrowDirection direction) { fCurrentDirection = direction; Invalidate(); }
private:
	BRect fScreenArea;
	arrowDirection fCurrentDirection;
};

#endif // MOUSEAREAVIEW_H
