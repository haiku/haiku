#ifndef _BEOS_DECORATOR_H_
#define _BEOS_DECORATOR_H_

#include "Decorator.h"
#include "SRect.h"
#include "SPoint.h"

class WinDecorator: public Decorator
{
public:
	WinDecorator(SRect frame, int32 wlook, int32 wfeel, int32 wflags);
	~WinDecorator(void);
	
	void MoveBy(float x, float y);
	void MoveBy(SPoint pt);
//	void ResizeBy(float x, float y);
//	void ResizeBy(SPoint pt);
	void Draw(SRect r);
	void Draw(void);
	//SRegion GetFootprint(void);
	click_type Clicked(SPoint pt, int32 buttons, int32 modifiers);

protected:
	void _DrawClose(SRect r);
	void _DrawFrame(SRect r);
	void _DrawTab(SRect r);
	void _DrawTitle(SRect r);
	void _DrawZoom(SRect r);
	void _DrawMinimize(SRect r);
	void _DoLayout(void);
	void _SetFocus(void);
	void DrawBlendedRect(SRect r, bool down);
	uint32 taboffset;

	RGBColor tab_highcol, tab_lowcol;
	RGBColor button_highcol, button_lowcol;
	RGBColor button_highercol, button_lowercol, button_midcol;
	RGBColor frame_highcol, frame_midcol, frame_lowcol, frame_highercol,
		frame_lowercol;
	RGBColor textcol;
	uint64 solidhigh, solidlow;

	bool slidetab;
	int textoffset;
};

#endif