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
		
	BBitmap *viewbmp;
	PortLink *serverlink;
	
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
	
	VDView *view;
};

class ViewDriver : public DisplayDriver
{
public:
	ViewDriver(void);
	~ViewDriver(void);

	void Initialize(void);		// Sets the driver
	bool IsInitialized(void);
	void Shutdown(void);		// You never know when you'll need this
	
	void SafeMode(void);	// Easy-access functions for common tasks
	void Reset(void);
	void Clear(uint8 red,uint8 green,uint8 blue);
	void Clear(rgb_color col);

	// Settings functions
	void SetScreen(uint32 space);
	int32 GetHeight(void);
	int32 GetWidth(void);
	int GetDepth(void);

	// Drawing functions
	void AddLine(BPoint pt1, BPoint pt2, rgb_color col);
	void BeginLineArray(int32 count);
	void Blit(BRect src, BRect dest);
	void DrawBitmap(ServerBitmap *bitmap);
	void DrawLineArray(int32 count,BPoint *start, BPoint *end, rgb_color *color);
	void DrawChar(char c, BPoint point);
	void DrawString(char *string, int length, BPoint point);
	void EndLineArray(void);

	void FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	void FillBezier(BPoint *points, uint8 *pattern);
	void FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
	void FillRect(BRect rect, uint8 *pattern);
	void FillRect(BRect rect, rgb_color col);
	void FillRegion(BRegion *region);
	void FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	void FillShape(BShape *shape);
	void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
	void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col);

	void HideCursor(void);
	bool IsCursorHidden(void);
	void MoveCursorTo(float x, float y);
	void MovePenTo(BPoint pt);
	void ObscureCursor(void);
	BPoint PenPosition(void);
	float PenSize(void);
	void SetCursor(ServerCursor *cursor);
	drawing_mode GetDrawingMode(void);
	void SetDrawingMode(drawing_mode mode);
	void SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	void SetHighColor(rgb_color col);
	void SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	void SetLowColor(rgb_color col);
	void SetPenSize(float size);
	void SetPixel(int x, int y, uint8 *pattern);
	void ShowCursor(void);

	float StringWidth(const char *string, int32 length);
	void StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	void StrokeBezier(BPoint *points, uint8 *pattern);
	void StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	void StrokeLine(BPoint point, uint8 *pattern);
	void StrokeLine(BPoint pt1, BPoint pt2, rgb_color col);
	void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	void StrokeRect(BRect rect,uint8 *pattern);
	void StrokeRect(BRect rect,rgb_color col);
	void StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	void StrokeShape(BShape *shape);
	void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
	void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col);
	VDWindow *screenwin;
protected:
	int hide_cursor;
	bool obscure_cursor;
	BBitmap *framebuffer;
	BView *drawview;

	PortLink *serverlink;
	// Region used to track of invalid areas for the Begin/EndLineArray functions
	BRegion laregion;
	rgb_color highcolor,lowcolor;
	int32 bufferheight, bufferwidth;
	int bufferdepth;
};

#endif