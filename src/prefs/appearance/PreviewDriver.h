#ifndef _PREVIEWDRIVER_H_
#define _PREVIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Cursor.h>
#include <Message.h>
#include <Rect.h>
#include <Region.h>
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class ServerCursor;
class ColorSet;

class PVView : public BView
{
public:
	PVView(BRect bounds);
	~PVView(void);
	void AttachedToWindow(void);
	void DetachedFromWindow(void);
	void Draw(BRect rect);

	BBitmap *viewbmp;
	BWindow *win;
};

class PreviewDriver : public DisplayDriver
{
public:
	PreviewDriver(void);
	~PreviewDriver(void);

	virtual bool Initialize(void);
	virtual void Shutdown(void);
	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
	virtual void DrawChar(char c, BPoint pt);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
//	virtual void DrawString(const char *string, int32 length, BPoint pt, escapement_delta *delta=NULL);
	virtual void InvertRect(BRect r);
	virtual void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	virtual void StrokeRect(BRect r, LayerData *d, int8 *pat);
	virtual void FillRect(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	virtual void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	virtual void SetMode(int32 mode);
	virtual bool DumpToFile(const char *path);

	BView *View(void) { return (BView*)view; };
protected:
	BBitmap *framebuffer;
	BView *drawview;

	// Region used to track of invalid areas for the Begin/EndLineArray functions
	BRegion laregion;
	PVView *view;
};

extern BRect preview_bounds;

#endif