#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <SupportDefs.h>
#include <Rect.h>
//#include <Window.h>
#include "Desktop.h"

class Layer;
class DisplayDriver;

typedef enum { CLICK_NONE=0, CLICK_ZOOM, CLICK_CLOSE, CLICK_MINIMIZE,
	CLICK_TAB, CLICK_DRAG, CLICK_MOVETOBACK, CLICK_MOVETOFRONT,
	
	CLICK_RESIZE, CLICK_RESIZE_L, CLICK_RESIZE_T, 
	CLICK_RESIZE_R, CLICK_RESIZE_B, CLICK_RESIZE_LT, CLICK_RESIZE_RT, 
	CLICK_RESIZE_LB, CLICK_RESIZE_RB } click_type;

// Definitions which are used in place of including Window.h
// window_look and window_feel are enumerated types, so we convert them
// to uint32's in order to ensure a constant size when sending via PortLink
// instead of depending on magic numbers in the header file.

#define WLOOK_NO_BORDER 0
#define WLOOK_BORDERED 1
#define WLOOK_TITLED 2
#define WLOOK_DOCUMENT 3
#define WLOOK_MODAL 4
#define WLOOK_FLOATING 5

#define WFEEL_NORMAL 0
#define WFEEL_MODAL_SUBSET 1
#define WFEEL_MODAL_APP 2
#define WFEEL_MODAL_WINDOW 3
#define WFEEL_FLOATING_SUBSET 4
#define WFEEL_FLOATING_APP 5
#define WFEEL_FLOATING_WINDOW 6

#define NOT_MOVABLE					0x00000001
#define NOT_CLOSABLE				0x00000020
#define NOT_ZOOMABLE				0x00000040
#define NOT_MINIMIZABLE				0x00004000
#define NOT_RESIZABLE				0x00000002
#define NOT_H_RESIZABLE				0x00000004
#define NOT_V_RESIZABLE				0x00000008
#define AVOID_FRONT					0x00000080
#define AVOID_FOCUS					0x00002000
#define WILL_ACCEPT_FIRST_CLICK		0x00000010
#define OUTLINE_RESIZE				0x00001000
#define NO_WORKSPACE_ACTIVATION		0x00000100
#define NOT_ANCHORED_ON_ACTIVATE	0x00020000
#define ASYNCHRONOUS_CONTROLS		0x00080000
#define QUIT_ON_WINDOW_CLOSE		0x00100000

class Decorator
{
public:
	Decorator(Layer *lay, uint32 windowflags, uint32 wlook);
	virtual ~Decorator(void);
	
	Layer* GetLayer(void);
	
	virtual click_type Clicked(BPoint pt, uint32 buttons);
	virtual void Resize(BRect rect);
	virtual void MoveBy(BPoint pt);
	virtual BRect GetBorderSize(void);
	virtual BPoint GetMinimumSize(void);
	virtual BRegion *GetFootprint(void);
	virtual void SetFlags(uint32 flags);
	virtual uint32 Flags(void);
	virtual void SetLook(uint32 wlook);
	virtual uint32 Look(void);
	virtual void SetFeel(uint32 wfeel);
	virtual uint32 Feel(void);
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
	BPoint minsize;
	uint32 dlook, dfeel, dflags;
	BRect bsize;
	bool focused;
	bool zoomstate, closestate, minstate;
	DisplayDriver *driver;
};

// C exports to hide the class-related stuff from the rest of the
// server. Written for each decorator, but don't amount to much.
typedef uint16 get_decorator_version(void);
typedef Decorator *create_decorator(Layer *lay, uint32 dflags, uint32 wlook);

#endif
