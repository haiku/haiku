#ifndef _GFX_DRIVER_H_
#define _GFX_DRIVER_H_

#include <GraphicsCard.h>
#include <Rect.h>
#include <Locker.h>
#include "Desktop.h"

class ServerBitmap;
class ServerRegion;
class ServerShape;
class ServerRect;
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
	virtual void Shutdown(void);		// You never know when you'll need this
	
	virtual void SafeMode(void);	// Easy-access functions for common tasks
	virtual void Reset(void);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);

	// Settings functions
	virtual void SetScreen(uint32 space);
	virtual int32 GetHeight(void);
	virtual int32 GetWidth(void);
	virtual int GetDepth(void);

	// Drawing functions
	virtual void SetPixel(int x, int y, rgb_color color);
	virtual void DrawBitmap(ServerBitmap *bitmap);
	virtual void DrawChar(char c, int x, int y);
	virtual void DrawString(char *string, int length, int x, int y);
	virtual void StrokeRect(BRect rect,rgb_color color);
	virtual void FillRect(BRect rect, rgb_color color);
	virtual void StrokeEllipse(BRect rect,rgb_color color);
	virtual void FillEllipse(BRect rect,rgb_color color);
	virtual void StrokeArc(int centerx, int centery, int xradius, int yradius);
	virtual void FillArc(int centerx, int centery, int xradius, int yradius);
	virtual void StrokePolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void FillPolygon(int *x, int *y, int numpoints, bool is_closed);
	virtual void StrokeRegion(ServerRegion *region);
	virtual void FillRegion(ServerRegion *region);
	virtual void StrokeRoundRect(ServerRect *rect);
	virtual void FillRoundRect(ServerRect *rect);
	virtual void StrokeShape(ServerShape *shape);
	virtual void FillShape(ServerShape *shape);
	virtual void StrokeTriangle(int *x, int *y);
	virtual void FillTriangle(int *x, int *y);
	virtual void MoveTo(int x, int y);
	virtual void Blit(BPoint dest, ServerBitmap *src, ServerBitmap *dest);
		
	virtual void SetCursor(int32 value);
	virtual void SetCursor(ServerCursor *cursor);
	virtual void ShowCursor(void);
	virtual void MoveCursorTo(int x, int y);
	virtual void HideCursor(void);
	virtual void ObscureCursor(void);
	virtual bool IsCursorHidden(void);

	
	virtual bool IsInitialized(void);
	graphics_card_hook ghooks[48];
	graphics_card_info *ginfo;

protected:
	bool is_initialized, cursor_visible, show_on_move;
	ServerCursor *current_cursor;
	BLocker *locker;
};

#endif