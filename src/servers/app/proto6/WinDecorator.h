#ifndef _WIN_DECORATOR_H_
#define _WIN_DECORATOR_H_

#include "Decorator.h"

class WinDecorator: public Decorator
{
public:
	WinDecorator(Layer *lay, uint32 dflags, uint32 wlook);
	~WinDecorator(void);
	
	click_type Clicked(BPoint pt, uint32 buttons);
	void Resize(BRect rect);
	void MoveBy(BPoint pt);
	BPoint GetMinimumSize(void);
	void SetTitle(const char *newtitle);
	void SetFlags(uint32 flags);
	void SetLook(uint32 wlook);
	void UpdateFont(void);
	void UpdateTitle(const char *string);
	void SetFocus(bool focused);
	void Draw(void);
	void Draw(BRect update);
	void DrawZoom(BRect r);
	void DrawClose(BRect r);
	void DrawMinimize(BRect r);
	void DrawTab(void);
	void DrawFrame(void);
	void CalculateBorders(void);

	void DrawBlendedRect(BRect r, bool down);
	uint32 taboffset;

	rgb_color tab_highcol, tab_lowcol;
	rgb_color button_highercol, button_lowercol;
	rgb_color button_highcol, button_midcol, button_lowcol;
	rgb_color frame_highcol, frame_midcol, frame_lowcol, frame_highercol,
		frame_lowercol;
	rgb_color textcol;

	BRect zoomrect,closerect,minrect,tabrect,frame,borderrect;
	int textoffset;
};

#endif