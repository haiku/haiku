#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <SupportDefs.h>
#include <Rect.h>
#include <Window.h>
#include "Desktop.h"

class Layer;
class DisplayDriver;

typedef enum { CLICK_NONE=0, CLICK_ZOOM, CLICK_CLOSE, CLICK_MINIMIZE,
	CLICK_TAB, CLICK_DRAG, CLICK_MOVETOBACK, CLICK_MOVETOFRONT,
	
	CLICK_RESIZE, CLICK_RESIZE_L, CLICK_RESIZE_T, 
	CLICK_RESIZE_R, CLICK_RESIZE_B, CLICK_RESIZE_LT, CLICK_RESIZE_RT, 
	CLICK_RESIZE_LB, CLICK_RESIZE_RB } click_type;

class Decorator
{
public:
	Decorator(Layer *lay, uint32 windowflags, window_look wlook);
	virtual ~Decorator(void);
	
	Layer* GetLayer(void);
	
	virtual click_type Clicked(BPoint pt, uint32 buttons);
	virtual void Resize(BRect rect);
	virtual void MoveBy(BPoint pt);
	virtual BRect GetBorderSize(void);
	virtual BPoint GetMinimumSize(void);
	virtual void SetFlags(uint32 flags);
	virtual uint32 Flags(void);
	virtual void SetLook(window_look wlook);
	virtual uint32 Look(void);
	virtual void UpdateTitle(const char *string);
	virtual void UpdateFont(void);
	virtual void SetFocus(bool focused);
	virtual void SetCloseButton(bool down);
	virtual void SetZoomButton(bool down);
	virtual void SetMinimizeButton(bool down);
	bool GetCloseButton(void) const;
	bool GetZoomButton(void) const;
	bool GetMinimizeButton(void) const;
	virtual void Draw(BRect update);
	virtual void Draw(void);
	virtual void DrawTab(void);
	virtual void DrawFrame(void);
	virtual void DrawZoom(BRect r);
	virtual void DrawClose(BRect r);
	virtual void DrawMinimize(BRect r);
protected:
	virtual void CalculateBorders(void);
	Layer *layer;
	uint32 flags;
	BPoint minsize;
	BRect bsize;
	uint32 look;
	bool focused;
	bool zoomstate, closestate, minstate;
	DisplayDriver *driver;
};

// C exports to hide the class-related stuff from the rest of the
// server. Written for each decorator, but don't amount to much.
typedef uint16 get_decorator_version(void);
typedef Decorator *create_decorator(Layer *lay, uint32 dflags, window_look wlook);

#endif
