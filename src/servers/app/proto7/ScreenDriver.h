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
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

class VDWindow;
class ServerCursor;
class ServerBitmap;
class RGBColor;
class PortLink;


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
	PortLink *serverlink;
	BPoint mousepos;
	int32 buttons;
};

class ScreenDriver : public DisplayDriver
{
public:
	ScreenDriver(void);
	~ScreenDriver(void);

	bool Initialize(void);
	void Shutdown(void);
	
	// Settings functions
//	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

//	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
//	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat);
//	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	virtual void FillRect(BRect r, LayerData *d, int8 *pat);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);

	virtual void HideCursor(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerBitmap *cursor, const BPoint &spot);

	virtual void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	virtual void StrokeRect(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
//	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	virtual void SetMode(int32 mode);
	float StringWidth(const char *string, int32 length, LayerData *d);
	float StringHeight(const char *string, int32 length, LayerData *d);
//	virtual bool DumpToFile(const char *path);
protected:
	void BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect);
	void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);
	void Line(BPoint start, BPoint end, LayerData *d, int8 *pat);
	void HLine(int32 x1, int32 x2, int32 y, RGBColor color);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	void SetPixel(int x, int y, RGBColor col);
	void SetPixel32(int x, int y, rgb_color col);
	void SetPixel16(int x, int y, uint16 col);
	void SetPixel8(int x, int y, uint8 col);
	void SetThickPixel(int x, int y, int thick, RGBColor col);
	void SetThickPixel32(int x, int y, int thick, rgb_color col);
	void SetThickPixel16(int x, int y, int thick, uint16 col);
	void SetThickPixel8(int x, int y, int thick, uint8 col);
	FrameBuffer *fbuffer;
	int hide_cursor;
	ServerBitmap *cursor, *under_cursor;
	int32 drawmode;
	BRect cursorframe, oldcursorframe;
};

#endif
