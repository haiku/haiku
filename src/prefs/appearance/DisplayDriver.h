#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <GraphicsCard.h>
#include <SupportDefs.h>
#include <OS.h>

#include <Rect.h>
#include "RGBColor.h"

class ServerBitmap;
class ServerCursor;
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
	virtual void Shutdown(void)=0;
	virtual void CopyBits(BRect src, BRect dest)=0;
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest)=0;
	virtual void DrawChar(char c, BPoint pt)=0;
//	virtual void DrawPicture(SPicture *pic, BPoint pt)=0;
//	virtual void DrawString(const char *string, int32 length, BPoint pt, escapement_delta *delta=NULL)=0;
	virtual void InvertRect(BRect r)=0;
	virtual void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)=0;
	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat)=0;
	virtual void StrokeEllipse(BRect r, LayerData *d, int8 *pat)=0;
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat)=0;
	virtual void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)=0;
	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)=0;
	virtual void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)=0;
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)=0;
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat)=0;
	virtual void StrokeRect(BRect r, LayerData *d, int8 *pat)=0;
	virtual void FillRect(BRect r, LayerData *d, int8 *pat)=0;
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)=0;
	virtual void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)=0;
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat)=0;
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat)=0;
	virtual void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)=0;
	virtual void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)=0;
	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)=0;
	virtual void SetMode(int32 mode)=0;
	virtual bool DumpToFile(const char *path)=0;

	uint8 GetDepth(void);
	uint16 GetHeight(void);
	uint16 GetWidth(void);
	int32 GetMode(void);

protected:
	void Lock(void);
	void Unlock(void);
	void SetDepthInternal(uint8 d);
	void SetHeightInternal(uint16 h);
	void SetWidthInternal(uint16 w);
	void SetModeInternal(int32 m);

private:
	sem_id lock_sem;
	uint8 buffer_depth;
	uint16 buffer_width;
	uint16 buffer_height;
	int32 buffer_mode;
};

#endif
