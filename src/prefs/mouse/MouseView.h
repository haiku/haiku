/*
	
	MouseView.h
	
*/

#ifndef MOUSE_VIEW_H
#define MOUSE_VIEW_H

#include <Box.h>
#include <Bitmap.h>

class MouseView : public BBox
{
public:
		typedef BBox	inherited;

		MouseView(BRect frame);
		virtual void Draw(BRect frame);

private:
		BBox	*fBox;
		
		BBitmap 	*fDoubleClickBitmap;
		BBitmap 	*fSpeedBitmap;
		BBitmap 	*fAccelerationBitmap;
};

#endif
