#ifndef _BEOS_DECORATOR_H_
#define _BEOS_DECORATOR_H_

#include "Decorator.h"

class WinDecorator: public Decorator
{
public:
				WinDecorator(DesktopSettings& settings,
					BRect frame, window_look wlook,
					uint32 wflags);
				~WinDecorator(void);

	void		SetTitle(const char* string,
	 				BRegion* updateRegion = NULL);
	void		FontsChanged(DesktopSettings& settings,
					BRegion* updateRegion);
	
	void MoveBy(BPoint pt);
	void ResizeBy(BPoint pt, BRegion* dirty);
	void Draw(BRect r);
	void Draw(void);
	void GetFootprint(BRegion *region);
	click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

protected:
	void		_UpdateFont(DesktopSettings& settings);
	void _DrawClose(BRect r);
	void _DrawFrame(BRect r);
	void _DrawTab(BRect r);
	void _DrawTitle(BRect r);
	void _DrawZoom(BRect r);
	void _DrawMinimize(BRect r);
	void _DoLayout(void);
	void _SetFocus(void);
	void _SetColors(void);
	void DrawBeveledRect(BRect r, bool down);
	uint32 taboffset;

	rgb_color tab_highcol;
	rgb_color tab_lowcol;
	rgb_color frame_highcol;
	rgb_color frame_midcol;
	rgb_color frame_lowcol;
	rgb_color frame_lowercol;
	rgb_color textcol;
	rgb_color			fFocusTabColor;
	rgb_color			fNonFocusTabColor;
	rgb_color			fFocusTextColor;
	rgb_color			fNonFocusTextColor;
	uint64 solidhigh, solidlow;

	BString				fTruncatedTitle;
	int32				fTruncatedTitleLength;

	bool slidetab;
	int textoffset;
};

#endif
