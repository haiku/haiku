#ifndef _BEOS_DECORATOR_H_
#define _BEOS_DECORATOR_H_

#include <Region.h>
#include "Decorator.h"

class BeDecorator: public Decorator
{
public:
	BeDecorator(BRect frame, const char *title, int32 wlook, int32 wfeel, int32 wflags,
		DisplayDriver *ddriver);
	~BeDecorator(void);
	
	void SetTitle(const char *string);
	void MoveBy(float x, float y);
	void MoveBy(BPoint pt);
	void ResizeBy(float x, float y);
	void ResizeBy(BPoint pt);
	void Draw(BRect r);
	void Draw(void);
	BRegion *GetFootprint(void);
	click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);
	BRect SlideTab(float dx, float dy=0);

protected:
	void _DrawClose(BRect r);
	void _DrawFrame(BRect r);
	void _DrawTab(BRect r);
	void _DrawTitle(BRect r);
	void _DrawZoom(BRect r);
	void _DoLayout(void);
	void _SetFocus(void);
	void DrawBlendedRect(BRect r, bool down);
	int32 taboffset;

	RGBColor tab_highcol, tab_lowcol;
	RGBColor button_highcol, button_lowcol;
	RGBColor frame_highcol, frame_midcol, frame_lowcol, frame_highercol,
		frame_lowercol;
	RGBColor textcol;
	uint64 solidhigh, solidlow;
	float titlewidth;

	int textoffset, titlechars;
};

#endif