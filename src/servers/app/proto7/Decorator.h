#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <SupportDefs.h>
#include <Rect.h>
#include <String.h>
#include "ColorSet.h"
#include "LayerData.h"

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
	Decorator(BRect rect, const char *title, int32 wlook, int32 wfeel, int32 wflags,
			DisplayDriver *ddriver);
	virtual ~Decorator(void);
	void SetColors(ColorSet cset);
	void SetDriver(DisplayDriver *d);
	void SetClose(bool is_down);
	void SetMinimize(bool is_down);
	void SetZoom(bool is_down);
	void SetFlags(int32 wflags);
	void SetFeel(int32 wfeel);
	void SetLook(int32 wlook);
	bool GetClose(void);
	bool GetMinimize(void);
	bool GetZoom(void);
	int32 GetLook(void);
	int32 GetFeel(void);
	int32 GetFlags(void);
	virtual void SetTitle(const char *string);
	void SetFocus(bool is_active);
	bool GetFocus(void) { return has_focus; };
	const char *GetTitle(void);
//	void SetFont(SFont *sf);
	ColorSet GetColors(void) { if(colors) return *colors; else return ColorSet(); }
	
	virtual void MoveBy(float x, float y);
	virtual void MoveBy(BPoint pt);
	virtual void ResizeBy(float x, float y);
	virtual void ResizeBy(BPoint pt);
	virtual void Draw(BRect r);
	virtual void Draw(void);
	virtual void DrawClose(void);
	virtual void DrawFrame(void);
	virtual void DrawMinimize(void);
	virtual void DrawTab(void);
	virtual void DrawTitle(void);
	virtual void DrawZoom(void);
	//virtual SRegion GetFootprint(void);
	virtual click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

protected:
	int32 _ClipTitle(float width);
	int32 TitleWidth(void) { return (title_string)?title_string->CountChars():0; }
	virtual void _DrawClose(BRect r);
	virtual void _DrawFrame(BRect r);
	virtual void _DrawMinimize(BRect r);
	virtual void _DrawTab(BRect r);
	virtual void _DrawTitle(BRect r);
	virtual void _DrawZoom(BRect r);
	virtual void _SetFocus(void)=0;
	virtual void _DoLayout(void)=0;
	ColorSet *colors;
	int32 look, feel, flags;
	DisplayDriver *driver;
	LayerData layerdata;
	BRect zoomrect,closerect,minimizerect,tabrect,frame,
		resizerect,borderrect;
private:
	bool close_state, zoom_state, minimize_state;
	bool has_focus;
	BString *title_string;
};

typedef float get_version(void);
typedef Decorator *create_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags);

#endif
