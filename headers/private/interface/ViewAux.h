#ifndef _VIEWAUX_H_
#define _VIEWAUX_H_

#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Font.h>
/*
	URGENT: Move this structure in a private header.
		This one is copied from Shape.cpp.
*/
struct shape_data {
	uint32	*opList;
	int32	opCount;
	int32	opSize;
	int32	opBlockSize;
	BPoint	*ptList;
	int32	ptCount;
	int32	ptSize;
	int32	ptBlockSize;
};

enum {
	B_VIEW_FONT_BIT				= 0x00000001,
	B_VIEW_COLORS_BIT			= 0x00000002,
	B_VIEW_DRAW_MODE_BIT		= 0x00000004,
	B_VIEW_CLIP_REGION_BIT		= 0x00000008,
	B_VIEW_LINE_MODES_BIT		= 0x00000010,
	B_VIEW_BLENDING_BIT			= 0x00000020,
	B_VIEW_SCALE_BIT			= 0x00000040,
	B_VIEW_FONT_ALIASING_BIT	= 0x00000080,
	B_VIEW_COORD_BIT			= 0x00000100,	
	B_VIEW_ORIGIN_BIT			= 0x00000200,	
	B_VIEW_PEN_SIZE_BIT			= 0x00000400,
	B_VIEW_PEN_LOC_BIT			= 0x00000800,
	
		// used for archiving
	B_VIEW_RESIZE_BIT			= 0x00001000,
	B_VIEW_FLAGS_BIT			= 0x00002000,
	B_VIEW_EVMASK_BIT			= 0x00004000
};


//struct _view_attr_{
class ViewAttr
{
public:
			ViewAttr(void);
			BFont				font;
			uint16				fontFlags;

			BPoint				penPosition;
			float				penSize;

			rgb_color			highColor;
			rgb_color			lowColor;
			rgb_color			viewColor;
			
			pattern				patt;

			drawing_mode		drawingMode;
				// clipping region for our view
			BRegion				clippingRegion;
				// local coordinate system
			BPoint				coordSysOrigin;
				// line modes
			join_mode			lineJoin;
			cap_mode			lineCap;
			float				miterLimit;
				// alpha blending
			source_alpha		alphaSrcMode;
			alpha_function		alphaFncMode;
				// scale
			float				scale;
				// font aliasing. Used for printing only!
			bool				fontAliasing;
				// flags used for synchronization with app_server
			uint16				flags;
				// flags used for archiving
			uint16				archivingFlags;
};

struct _array_hdr_{
	float			startX;
	float			startY;	
	float			endX;
	float			endY;
	rgb_color		color;
};

struct _array_data_{
		// the max number of points in the array
	uint32			maxCount;
		// the current number of points in the array	
	uint32			count;
		// the array of points
	_array_hdr_*	array;
};

#endif
