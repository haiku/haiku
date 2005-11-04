/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DRAWING_ENGINE_H_
#define DRAWING_ENGINE_H_


#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>
#include <Point.h>

class BPoint;
class BRect;
class BRegion;

class DrawState;
class HWInterface;
class Painter;
class RGBColor;
class ServerBitmap;
class ServerCursor;
class ServerFont;

typedef struct
{
	BPoint pt1;
	BPoint pt2;
	rgb_color color;

} LineArrayData;

class DrawingEngine {
public:
								DrawingEngine(HWInterface* interface = NULL);
	virtual						~DrawingEngine();

	// when implementing, be sure to call the inherited version
	virtual status_t			Initialize();
	virtual void				Shutdown();

	virtual	void				Update();

	virtual void				SetHWInterface(HWInterface* interface);

	// clipping for all drawing functions, passing a NULL region
	// will remove any clipping (drawing allowed everywhere)
	virtual	void				ConstrainClippingRegion(BRegion* region);

	// drawing functions
	virtual	void				CopyRegion(		/*const*/ BRegion* region,
												int32 xOffset,
												int32 yOffset);

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg);

	virtual void				InvertRect(		BRect r);

	virtual	void				DrawBitmap(		ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawState *d);

	virtual	void				FillArc(		BRect r,
												const float &angle,
												const float &span,
												const DrawState *d);

	virtual	void				FillBezier(		BPoint *pts,
												const DrawState *d);

	virtual	void				FillEllipse(	BRect r,
												const DrawState *d);

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												BRect bounds,
												const DrawState *d);

	virtual	void				FillRect(		BRect r,
												const RGBColor &color);

	virtual	void				FillRect(		BRect r,
												const DrawState *d);

	virtual	void				FillRegion(		BRegion &r,
												const DrawState *d);

	virtual	void				FillRoundRect(	BRect r,
												const float &xrad,
												const float &yrad,
												const DrawState *d);

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawState *d);

	virtual	void				FillTriangle(	BPoint *pts,
												BRect bounds,
												const DrawState *d);

	virtual	void				StrokeArc(		BRect r,
												const float &angle,
												const float &span,
												const DrawState *d);

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawState *d);

	virtual	void				StrokeEllipse(	BRect r,
												const DrawState *d);

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color);

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawState *d);

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawState *d);

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color);

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawState *d);

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												BRect bounds,
												const DrawState *d,
												bool is_closed=true);

	// this version used by Decorator
	virtual	void				StrokeRect(		BRect r,
												const RGBColor &color);

	virtual	void				StrokeRect(		BRect r,
												const DrawState *d);

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawState *d);

	virtual	void				StrokeRoundRect(BRect r,
												const float &xrad,
												const float &yrad,
												const DrawState *d);

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawState *d);

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawState *d);

	// Font-related calls
	
	// DrawState is NOT const because this call updates the pen position in the passed DrawState
	virtual	void				DrawString(		const char* string,
												int32 length,
												const BPoint& pt,
												DrawState* d,
												escapement_delta* delta = NULL);

/*	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta=NULL);*/

	virtual	float				StringWidth(	const char* string,
												int32 length,
												const DrawState* d,
												escapement_delta* delta = NULL);

	virtual	float				StringWidth(	const char* string,
												int32 length,
												const ServerFont& font,
												escapement_delta* delta = NULL);

	virtual	float				StringHeight(	const char* string,
												int32 length,
												const DrawState* d);

	virtual bool				Lock();
	virtual void				Unlock();

			bool				WriteLock();
			void				WriteUnlock();

	virtual bool				DumpToFile(		const char *path);
	virtual ServerBitmap*		DumpToBitmap();

 private:
			BRect				_CopyRect(		BRect r,
												int32 xOffset,
												int32 yOffset) const;

			void				_CopyRect(		uint8* bits,
												uint32 width,
												uint32 height,
												uint32 bpr,
												int32 xOffset,
												int32 yOffset) const;

			Painter*			fPainter;
			HWInterface*		fGraphicsCard;
			uint32				fAvailableHWAccleration;
};

#endif // DRAWING_ENGINE_H_
