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
	virtual void CopyBits(SRect src, SRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, SRect src, SRect dest);
	virtual void DrawChar(char c, SPoint pt);
//	virtual void DrawPicture(SPicture *pic, SPoint pt);
//	virtual void DrawString(const char *string, int32 length, SPoint pt, escapement_delta *delta=NULL);
	virtual void InvertRect(SRect r);
	virtual void StrokeBezier(SPoint *pts, LayerData *d, int8 *pat);
	virtual void FillBezier(SPoint *pts, LayerData *d, int8 *pat);
	virtual void StrokeEllipse(SRect r, LayerData *d, int8 *pat);
	virtual void FillEllipse(SRect r, LayerData *d, int8 *pat);
	virtual void StrokeArc(SRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void FillArc(SRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void StrokeLine(SPoint start, SPoint end, LayerData *d, int8 *pat);
	virtual void StrokePolygon(SPoint *ptlist, int32 numpts, SRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	virtual void FillPolygon(SPoint *ptlist, int32 numpts, SRect rect, LayerData *d, int8 *pat);
	virtual void StrokeRect(SRect r, LayerData *d, int8 *pat);
	virtual void FillRect(SRect r, LayerData *d, int8 *pat);
	virtual void StrokeRoundRect(SRect r, float xrad, float yrad, LayerData *d, int8 *pat);
	virtual void FillRoundRect(SRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void StrokeTriangle(SPoint *pts, SRect r, LayerData *d, int8 *pat);
	virtual void FillTriangle(SPoint *pts, SRect r, LayerData *d, int8 *pat);
	virtual void StrokeLineArray(SPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
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

extern SRect preview_bounds;

#endif