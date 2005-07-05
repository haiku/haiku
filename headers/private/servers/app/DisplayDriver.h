//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved
//  Distributed under the terms of the MIT license.
//
//	File Name:		DisplayDriver.h
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//
//	Description:	Abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>
#include <Point.h>

class BPoint;
class BRect;
class BRegion;

class DrawData;
class HWInterface;
class RGBColor;
class ServerBitmap;
class ServerCursor;
class ServerFont;

/*!
	\brief Data structure for passing cursor information to hardware drivers.
*/
/*typedef struct
{
	uchar *xormask, *andmask;
	int32 width, height;
	int32 hotx, hoty;

} cursor_data;*/

typedef struct
{
	BPoint pt1;
	BPoint pt2;
	rgb_color color;

} LineArrayData;

class DisplayDriver {
public:
								DisplayDriver();
	virtual						~DisplayDriver();

	// when implementing, be sure to call the inherited version
	virtual status_t			Initialize();
	virtual void				Shutdown();

	// call this on mode changes!
	virtual	void				Update() = 0;

	virtual void				SetHWInterface(HWInterface* interface) = 0;

	// clipping for all drawing functions
	virtual	void				ConstrainClippingRegion(BRegion* region) = 0;

	// Graphics calls implemented in DisplayDriver
	virtual	void				CopyRegion(		/*const*/ BRegion* region,
												int32 xOffset,
												int32 yOffset) = 0;

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg) = 0;

	virtual void				InvertRect(		BRect r) = 0;

	virtual	void				DrawBitmap(		ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d) = 0;

	virtual	void				FillArc(		BRect r,
												const float &angle,
												const float &span,
												const DrawData *d) = 0;

	virtual	void				FillBezier(		BPoint *pts,
												const DrawData *d) = 0;

	virtual	void				FillEllipse(	BRect r,
												const DrawData *d) = 0;

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												BRect bounds,
												const DrawData *d) = 0;

	virtual	void				FillRect(		BRect r,
												const RGBColor &color) = 0;

	virtual	void				FillRect(		BRect r,
												const DrawData *d) = 0;

	virtual	void				FillRegion(		BRegion &r,
												const DrawData *d) = 0;

	virtual	void				FillRoundRect(	BRect r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) = 0;

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) = 0;

	virtual	void				FillTriangle(	BPoint *pts,
												BRect bounds,
												const DrawData *d) = 0;

	virtual	void				StrokeArc(		BRect r,
												const float &angle,
												const float &span,
												const DrawData *d) = 0;

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawData *d) = 0;

	virtual	void				StrokeEllipse(	BRect r,
												const DrawData *d) = 0;

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color) = 0;

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawData *d) = 0;

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d) = 0;

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color) = 0;

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawData *d) = 0;

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												BRect bounds,
												const DrawData *d,
												bool is_closed=true) = 0;

	// this version used by Decorator
	virtual	void				StrokeRect(		BRect r,
												const RGBColor &color) = 0;

	virtual	void				StrokeRect(		BRect r,
												const DrawData *d) = 0;

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawData *d) = 0;

	virtual	void				StrokeRoundRect(BRect r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) = 0;

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) = 0;

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d) = 0;

	// Font-related calls
	
	// DrawData is NOT const because this call updates the pen position in the passed DrawData
	virtual	void				DrawString(		const char* string,
												int32 length,
												const BPoint& pt,
												DrawData* d,
												escapement_delta *delta = NULL) = 0;

/*	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta = NULL) = 0;*/

	virtual	float				StringWidth(	const char* string,
												int32 length,
												const DrawData* d,
												escapement_delta *delta = NULL) = 0;

	virtual	float				StringWidth(	const char* string,
												int32 length,
												const ServerFont& font,
												escapement_delta *delta = NULL) = 0;

	virtual	float				StringHeight(	const char *string,
												int32 length,
												const DrawData *d) = 0;

	virtual bool				Lock() = 0;
	virtual void				Unlock() = 0;

	virtual bool				DumpToFile(const char *path) = 0;
	virtual ServerBitmap*		DumpToBitmap() = 0;

};

#endif
