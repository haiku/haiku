#ifndef _VIEWDRIVER_H_
#define _VIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <GraphicsDefs.h>	// for pattern struct
#include <Cursor.h>
#include <Message.h>
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class VDWindow;
class ServerCursor;

class VDView : public BView
{
public:
	VDView(BRect bounds);
	~VDView(void);
	void AttachedToWindow(void);
	void Draw(BRect rect);
	void MouseDown(BPoint pt);
	void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint pt);
	void SetMode(int16 width, int16 height, uint8 bpp);
		
	BBitmap *viewbmp;
	BView *drawview;
	PortLink *serverlink;

protected:
	friend VDWindow;
	
	int hide_cursor;
	BBitmap *cursor;
	
	BRect cursorframe, oldcursorframe;
	bool obscure_cursor;
};

class VDWindow : public BWindow
{
public:
	VDWindow(void);
	~VDWindow(void);
	void MessageReceived(BMessage *msg);
	bool QuitRequested(void);
	void WindowActivated(bool active);
	
	void SafeMode(void);
	void Reset(void);
	void SetScreen(uint32 space);
	void Clear(uint8 red,uint8 green,uint8 blue);
	VDView *view;
};

class ViewDriver : public DisplayDriver
{
public:
	ViewDriver(void);
	virtual ~ViewDriver(void);

	virtual void Initialize(void);		// Sets the driver
	virtual bool IsInitialized(void);
	virtual void Shutdown(void);		// You never know when you'll need this
	
	virtual void SafeMode(void);	// Easy-access functions for common tasks
	virtual void Reset(void);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);
	virtual void Clear(rgb_color col);

	// Settings functions
	virtual void SetScreen(uint32 space);
	virtual int32 GetHeight(void);
	virtual int32 GetWidth(void);
	virtual int GetDepth(void);

	// Drawing functions
	virtual void Blit(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bitmap);
	virtual void DrawChar(char c, BPoint point);
	virtual void DrawString(char *string, int length, BPoint point);

	virtual void FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void FillBezier(BPoint *points, uint8 *pattern);
	virtual void FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void FillRect(BRect rect, uint8 *pattern);
	virtual void FillRect(BRect rect, rgb_color col);
	virtual void FillRegion(BRegion *region);
	virtual void FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void FillShape(BShape *shape);
	virtual void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);

	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void MovePenTo(BPoint pt);
	virtual void ObscureCursor(void);
	virtual BPoint PenPosition(void);
	virtual float PenSize(void);
	virtual void SetCursor(int32 value);
	virtual void SetCursor(ServerCursor *cursor);
	virtual void SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetPenSize(float size);
	virtual void SetPixel(int x, int y, uint8 *pattern);
	virtual void ShowCursor(void);

	virtual void StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void StrokeBezier(BPoint *points, uint8 *pattern);
	virtual void StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void StrokeLine(BPoint point, uint8 *pattern);
	virtual void StrokeLine(BPoint pt1, BPoint pt2, rgb_color col);
	virtual void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void StrokeRect(BRect rect,uint8 *pattern);
	virtual void StrokeRect(BRect rect,rgb_color col);
	virtual void StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void StrokeShape(BShape *shape);
	virtual void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
	VDWindow *screenwin;
protected:
	int hide_cursor;
};

#endif