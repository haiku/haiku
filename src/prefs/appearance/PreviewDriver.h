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

	bool Initialize(void);
	void Shutdown(void);
	void CopyBits(BRect src, BRect dest);
	void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
	void DrawChar(char c, BPoint pt);
//	void DrawPicture(SPicture *pic, BPoint pt);
	void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);
	void InvertRect(BRect r);
	void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	void FillEllipse(BRect r, LayerData *d, int8 *pat);
	void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	void StrokeRect(BRect r, LayerData *d, int8 *pat);
	void FillRect(BRect r, LayerData *d, int8 *pat);
	void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
	void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
//	void FillShape(SShape *sh, LayerData *d, int8 *pat);
	void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
	void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	void SetMode(int32 mode);
	float StringWidth(const char *string, int32 length, LayerData *d);
	void GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, float maxwidth, char **outstrings);
	bool DumpToFile(const char *path);

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
