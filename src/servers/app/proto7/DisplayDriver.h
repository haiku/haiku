#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <GraphicsCard.h>
#include <SupportDefs.h>
#include <OS.h>

#include <Font.h>
#include <Rect.h>
#include "RGBColor.h"

class ServerCursor;
class ServerBitmap;
class LayerData;

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

#define DRAW_COPY 0
#define DRAW_OVER 1
#define DRAW_ERASE 2	
#define DRAW_INVERT 3
#define DRAW_ADD 4
#define DRAW_SUBTRACT 5
#define DRAW_BLEND 6
#define DRAW_MIN 7
#define DRAW_MAX 8
#define DRAW_SELECT 9
#define DRAW_ALPHA 10

class DisplayDriver
{
public:
	DisplayDriver(void);
	virtual ~DisplayDriver(void);
	virtual bool Initialize(void);
	virtual void Shutdown(void);
	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest);
	virtual void DrawChar(char c, BPoint pt, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	virtual void FillRect(BRect r, LayerData *d, int8 *pat);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);

	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerBitmap *cursor, const BPoint &hotspot);

	virtual void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	virtual void StrokeRect(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	virtual void SetMode(int32 mode);
	virtual float StringWidth(const char *string, int32 length, LayerData *d);
	virtual float StringHeight(const char *string, int32 length, LayerData *d);
	virtual bool DumpToFile(const char *path);

	uint8 GetDepth(void);
	uint16 GetHeight(void);
	uint16 GetWidth(void);
	int32 GetMode(void);
	void SetHotSpot(const BPoint &pt);
	BPoint GetHotSpot(void);

protected:
	void Lock(void);
	void Unlock(void);
	void SetDepthInternal(uint8 d);
	void SetHeightInternal(uint16 h);
	void SetWidthInternal(uint16 w);
	void SetModeInternal(int32 m);
	void SetCursorHidden(bool state);
	void SetCursorObscured(bool state);
	bool IsCursorObscured(bool state);
	bool CursorStateChanged(void);

private:
	sem_id lock_sem;
	uint8 buffer_depth;
	uint16 buffer_width;
	uint16 buffer_height;
	int32 buffer_mode;
	drawing_mode drawmode;
	int32 is_cursor_hidden;
	bool is_cursor_obscured, cursor_state_changed;
	BPoint cursor_hotspot;
};

#endif