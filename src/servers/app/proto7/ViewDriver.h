#ifndef _VIEWDRIVER_H_
#define _VIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <GraphicsDefs.h>	// for pattern struct
#include <Cursor.h>
#include <Message.h>
#include <Region.h>
#include <Font.h>
#include "DisplayDriver.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class BBitmap;
class PortLink;
class VDWindow;
class LayerData;

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

	bool Initialize(void);		// Sets the driver
	void Shutdown(void);		// You never know when you'll need this
	
	// Drawing functions
	void CopyBits(BRect src, BRect dest);
	void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest);
	void DrawChar(char c, BPoint pt, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	void FillEllipse(BRect r, LayerData *d, int8 *pat);
	void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	void FillRect(BRect r, LayerData *d, int8 *pat);
	void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	void FillShape(SShape *sh, LayerData *d, int8 *pat);
	void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);

	void HideCursor(void);
	void InvertRect(BRect r);
	bool IsCursorHidden(void);
	void MoveCursorTo(float x, float y);
//	void MovePenTo(BPoint pt);
	void ObscureCursor(void);
//	BPoint PenPosition(void);
//	float PenSize(void);
	void SetCursor(ServerBitmap *cursor, const BPoint &spot);
//	drawing_mode GetDrawingMode(void);
//	void SetDrawingMode(drawing_mode mode);
	void ShowCursor(void);

	void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	void StrokeRect(BRect r, LayerData *d, int8 *pat);
	void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
	void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	void SetMode(int32 mode);
//	bool DumpToFile(const char *path);
	VDWindow *screenwin;
protected:
	void SetLayerData(LayerData *d, bool set_font_data=false);
	void BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	int hide_cursor;
	bool obscure_cursor;
	BBitmap *framebuffer;
	BView *drawview;
	BRegion laregion;

	PortLink *serverlink;
//	drawing_mode drawmode;
	
	rgb_color highcolor,lowcolor;
	bool is_initialized;
};

#endif