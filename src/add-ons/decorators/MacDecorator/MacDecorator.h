#ifndef _MAC_DECORATOR_H_
#define _MAC_DECORATOR_H_

#include "Decorator.h"

class MacDecorator: public Decorator
{
public:
	MacDecorator(BRect frame, int32 wlook, int32 wfeel, int32 wflags);
	~MacDecorator(void);
	
	void MoveBy(float x, float y);
	void MoveBy(BPoint pt);
//	void ResizeBy(float x, float y);
//	void ResizeBy(BPoint pt);
	void Draw(BRect r);
	void Draw(void);
	void GetFootprint(BRegion *region);
	click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

protected:
	void _DrawClose(BRect r);
	void _DrawFrame(BRect r);
	void _DrawTab(BRect r);
	void _DrawTitle(BRect r);
	void _DrawZoom(BRect r);
	void _DrawMinimize(BRect r);
	void _DoLayout(void);
//	void _SetColors(void);
	void DrawBlendedRect(BRect r, bool down);
	uint32 taboffset;

	RGBColor tab_col;
	RGBColor button_highcol,button_lowcol;
	RGBColor frame_highcol, frame_midcol, frame_lowcol,	frame_lowercol;
	RGBColor textcol,inactive_textcol;
	uint64 solidhigh, solidlow;

	bool slidetab;
	int textoffset;
	float titlepixelwidth;
};

#endif