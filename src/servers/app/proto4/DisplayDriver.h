#ifndef _GFX_DRIVER_H_
#define _GFX_DRIVER_H_

#include <GraphicsCard.h>
#include <Rect.h>
#include <Locker.h>
#include "Desktop.h"

class ServerBitmap;
class ServerCursor;

#ifndef ROUND
	#define ROUND(a)	( (a-long(a))>=.5)?(long(a)+1):(long(a))
#endif

typedef struct
{
	uchar *xormask, *andmask;
	int32 width, height;
	int32 hotx, hoty;

} cursor_data;

#ifndef HOOK_DEFINE_CURSOR

#define HOOK_DEFINE_CURSOR		0
#define HOOK_MOVE_CURSOR		1
#define HOOK_SHOW_CURSOR		2
#define HOOK_DRAW_LINE_8BIT		3
#define HOOK_DRAW_LINE_16BIT	12
#define HOOK_DRAW_LINE_32BIT	4
#define HOOK_DRAW_RECT_8BIT		5
#define HOOK_DRAW_RECT_16BIT	13
#define HOOK_DRAW_RECT_32BIT	6
#define HOOK_BLIT				7
#define HOOK_DRAW_ARRAY_8BIT	8
#define HOOK_DRAW_ARRAY_16BIT	14	// Not implemented in current R5 drivers
#define HOOK_DRAW_ARRAY_32BIT	9
#define HOOK_SYNC				10
#define HOOK_INVERT_RECT		11

#endif

class ServerCursor;

class DisplayDriver
{
public:
	DisplayDriver(void);
	virtual ~DisplayDriver(void);

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
	virtual void Blit(BPoint dest, ServerBitmap *src, ServerBitmap *dest);
	virtual void DrawBitmap(ServerBitmap *bitmap);
	virtual void DrawChar(char c, BPoint point);
	virtual void DrawString(char *string, int length, BPoint point);

	virtual void FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void FillBezier(BPoint *points, uint8 *pattern);
	virtual void FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void FillRect(BRect rect, uint8 *pattern);
	virtual void FillRegion(BRegion *region);
	virtual void FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void FillShape(BShape *shape);
	virtual void FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);

//	virtual void GetBlendingMode(source_alpha *srcmode, alpha_function *funcmode);
//	virtual drawing_mode GetDrawingMode(void);
	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void MovePenTo(BPoint pt);
	virtual void ObscureCursor(void);
	virtual BPoint PenPosition(void);
	virtual float PenSize(void);
//	virtual void SetBlendingMode(source_alpha srcmode, alpha_function funcmode);
	virtual void SetCursor(int32 value);
	virtual void SetCursor(ServerCursor *cursor);
//	virtual void SetDrawingMode(drawing_mode mode);
	virtual void ShowCursor(void);
	virtual void SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255);
	virtual void SetPenSize(float size);
	virtual void SetPixel(int x, int y, uint8 *pattern);

	virtual void StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern);
	virtual void StrokeBezier(BPoint *points, uint8 *pattern);
	virtual void StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern);
	virtual void StrokeLine(BPoint point, uint8 *pattern);
	virtual void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void StrokeRect(BRect rect,uint8 *pattern);
	virtual void StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern);
	virtual void StrokeShape(BShape *shape);
	virtual void StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern);

	graphics_card_hook ghooks[48];
	graphics_card_info *ginfo;

protected:
	bool is_initialized, cursor_visible, show_on_move;
	ServerCursor *current_cursor;
	BLocker *locker;
	rgb_color highcol, lowcol;
	BPoint penpos;
	float pensize;
};

#endif