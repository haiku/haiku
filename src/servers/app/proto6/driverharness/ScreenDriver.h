#ifndef _SCREENDRIVER_H_
#define _SCREENDRIVER_H_

#include <Application.h>
#include <WindowScreen.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Message.h>
#include <OS.h>
#include <Locker.h>
#include <Region.h>	// for clipping_rect definition
#include <Bitmap.h>
#include "DisplayDriver.h"

class VDWindow;
class ServerCursor;
class ServerBitmap;


class FrameBuffer : public BWindowScreen
{
public:
	FrameBuffer(const char *title, uint32 space, status_t *st,bool debug);
	~FrameBuffer(void);
	void ScreenConnected(bool connected);
	void MessageReceived(BMessage *msg);
	bool IsConnected(void) const { return is_connected; }
	bool QuitRequested(void);
	
	graphics_card_info gcinfo;
protected:
	bool is_connected;
};

class ScreenDriver : public DisplayDriver
{
public:
	ScreenDriver(void);
	~ScreenDriver(void);

	void Initialize(void);
	void Shutdown(void);
	
	void Clear(uint8 red,uint8 green,uint8 blue);
	void Clear(rgb_color col);

	// Settings functions
	void SetScreen(uint32 space);
/*	int32 GetHeight(void);
	int32 GetWidth(void);
	int GetDepth(void);

	// Drawing functions
	void Blit(BRect src, BRect dest);
*/	void DrawBitmap(ServerBitmap *bitmap, BRect source, BRect dest);
/*	void DrawChar(char c, BPoint point);
	void DrawString(char *string, int length, BPoint point);

	void FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	void FillBezier(BPoint *points, uint8 *pattern);
	void FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
*/	void FillRect(BRect rect, uint8 *pattern);
	void FillRect(BRect rect, rgb_color col);
//	void FillRegion(BRegion *region);
	void FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
//	void FillShape(BShape *shape);

	void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
	void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col);
/*
	void HideCursor(void);
	bool IsCursorHidden(void);
	void MoveCursorTo(float x, float y);
	void ObscureCursor(void);
	float PenSize(void);
	void SetCursor(ServerCursor *cursor);
*/
	void SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	void SetHighColor(rgb_color col);
	void SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	void SetLowColor(rgb_color col);

	void SetPixel(int x, int y, uint8 *pattern);

	void SetPixel32(int x, int y, rgb_color col);
	void SetPixel16(int x, int y, uint16 col);
	void SetPixel8(int x, int y, uint8 col);
	void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);

//	void ShowCursor(void);

//	void StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
//	void StrokeBezier(BPoint *points, uint8 *pattern);
//	void StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	void StrokeLine(BPoint point, uint8 *pattern);
	void StrokeLine(BPoint pt1, BPoint pt2, rgb_color col);
	void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	void StrokeRect(BRect rect,uint8 *pattern);
	void StrokeRect(BRect rect,rgb_color col);
	void StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
//	void StrokeShape(BShape *shape);
	void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);
	void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col);

protected:
	void Line32(BPoint pt, BPoint pt2, uint8 *pattern);
	FrameBuffer *fbuffer;
	int hide_cursor;
	ServerBitmap *cursor;
	int32 drawmode;
};

#endif