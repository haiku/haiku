#ifndef _MAC_DECORATOR_H_
#define _MAC_DECORATOR_H_

#include "Decorator.h"
#include "RGBColor.h"




class MacDecorator: public Decorator {
public:
		MacDecorator(DesktopSettings& settings,
			BRect frame,
			window_look look,
			uint32 flags);
		~MacDecorator(void);

	// SetTitle
	// SetLook
	// SetFlags
	
	//void MoveBy(float x, float y);
	void MoveBy(BPoint pt);
//	void ResizeBy(float x, float y);
	void ResizeBy(BPoint pt, BRegion* diirty);
	// SetTabLocation
	// TabLocation
	// SetSettings
	// GetSettings
	void Draw(BRect r);
	void Draw(void);
	//GetSizeLimits
	void GetFootprint(BRegion *region);
	click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

 protected:
	void _DoLayout(void);
	void _DrawFrame(BRect r);
	void _DrawTab(BRect r);
	void _DrawClose(BRect r);
	void _DrawTitle(BRect r);
	void _DrawZoom(BRect r);
	void _DrawMinimize(BRect r);
	// SetFocus
//	void _SetColors(void);
	void DrawBlendedRect(BRect r, bool down);
	uint32 taboffset;
 private:
	RGBColor tab_col;
	RGBColor button_highcol,button_lowcol;
	RGBColor frame_highcol, frame_midcol, frame_lowcol,	frame_lowercol;
	RGBColor textcol,inactive_textcol;
	uint64 solidhigh, solidlow;

	BString				fTruncatedTitle;
	int32				fTruncatedTitleLength;

	bool slidetab;
	int textoffset;
	float titlepixelwidth;
};

#endif
