/*
	
	KeyboardView.h
	
*/

#ifndef KEYBOARD_VIEW_H
#define KEYBOARD_VIEW_H


#include <Box.h>
#include <Slider.h>
#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <Application.h>

class KeyboardView : public BBox
{
public:
		typedef BBox	inherited;

		KeyboardView(BRect frame);
		virtual void	Draw(BRect frame);

private:
		BBitmap 		*fIconBitmap;
		BBitmap 		*fClockBitmap;
		BBox			*fBox;

};

#endif
