//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved.
//  Distributed under the terms of the MIT license.
//
//	File Name:		DisplayDriverPainter.h
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//
//	Description:	Abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_PAINTER_H_
#define _DISPLAY_DRIVER_PAINTER_H_

#include "DisplayDriver.h"

class HWInterface;
class Painter;

class DisplayDriverPainter : public DisplayDriver {
public:
								DisplayDriverPainter(HWInterface* hwInterface);
	virtual						~DisplayDriverPainter();

	// when implementing, be sure to call the inherited version
	virtual status_t			Initialize();
	virtual void				Shutdown();

	virtual	void				Update();

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

	virtual void				InvertRect(		const BRect &r);

	virtual	void				DrawBitmap(		ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d);

	virtual	void				FillArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d);

	virtual	void				FillBezier(		BPoint *pts,
												const DrawData *d);

	virtual	void				FillEllipse(	const BRect &r,
												const DrawData *d);

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d);

	virtual	void				FillRect(		const BRect &r,
												const RGBColor &color);

	virtual	void				FillRect(		const BRect &r,
												const DrawData *d);

	virtual	void				FillRegion(		BRegion &r,
												const DrawData *d);

	virtual	void				FillRoundRect(	const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d);

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d);

	virtual	void				FillTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d);

	virtual	void				StrokeArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d);

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawData *d);

	virtual	void				StrokeEllipse(	const BRect &r,
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color);

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawData *d);

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color);

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawData *d);

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d,
												bool is_closed=true);

	// this version used by Decorator
	virtual	void				StrokeRect(		const BRect &r,
												const RGBColor &color);

	virtual	void				StrokeRect(		const BRect &r,
												const DrawData *d);

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawData *d);

	virtual	void				StrokeRoundRect(const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d);

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d);

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d);

	// Font-related calls
	
	// DrawData is NOT const because this call updates the pen position in the passed DrawData
	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												DrawData *d);

	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta=NULL);

	virtual	float				StringWidth(	const char *string,
												int32 length,
												const DrawData *d);

	virtual	float				StringWidth(	const char *string,
												int32 length,
												const ServerFont &font);

	virtual	float				StringHeight(	const char *string,
												int32 length,
												const DrawData *d);

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

#endif // _DISPLAY_DRIVER_PAINTER_H_
