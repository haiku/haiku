#ifndef _DIRECTDRIVER_H_
#define _DIRECTDRIVER_H_

#include <Application.h>
#include <DirectWindow.h>
#include <GraphicsCard.h>
#include <Message.h>
#include <OS.h>
#include <Locker.h>
#include <Region.h>	// for clipping_rect definition
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class VDWindow;
class ServerCursor;
class PortLink;
class ServerBitmap;

class DDriverWin : public BDirectWindow
{
public:
	DDriverWin(BRect frame);
	~DDriverWin(void);
	bool QuitRequested(void);
	void WindowActivated(bool active);
	void DirectConnected(direct_buffer_info *info);
	static int32 DrawingThread(void *data);
	port_id msgport;

	void SafeMode(void);
	void Reset(void);
	void SetScreen(uint32 space);
	void Clear(clipping_rect *rect, uint8 red,uint8 green,uint8 blue);
	ServerBitmap *framebuffer;
protected:
	BLocker *locker;
	bool fConnected, fConnectionDisabled, fDirty;
	clipping_rect *fClipList,fBounds;
	thread_id fDrawThreadID;
	color_space fFormat;
	uint8 *fBits;
	int32 fNumClipRects, fRowBytes;
};

class DirectDriver : public DisplayDriver
{
public:
	DirectDriver(void);
	virtual ~DirectDriver(void);

	virtual void Initialize(void);
	virtual bool IsInitialized(void);
	virtual void Shutdown(void);
	
	virtual void SafeMode(void);
	virtual void Reset(void);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);
	virtual void Clear(rgb_color col);

/*	// Settings functions
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
*/
protected:
	DDriverWin *driverwin;
	int hide_cursor;
	PortLink *drawlink;
};

#endif