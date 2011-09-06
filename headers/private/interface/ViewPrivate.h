/*
 * Copyright 2003-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef VIEW_PRIVATE_H
#define VIEW_PRIVATE_H


#include <Font.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>
#include <ServerProtocolStructs.h>
#include <View.h>

const static uint32 kDeleteReplicant = 'JAHA';

const static uint32 kWorkspacesViewFlag = 0x40000000UL;
	// was/is _B_RESERVED1_ in View.h

enum {
	B_VIEW_FONT_BIT				= 0x00000001,
	B_VIEW_HIGH_COLOR_BIT		= 0x00000002,
	B_VIEW_DRAWING_MODE_BIT		= 0x00000004,
	B_VIEW_CLIP_REGION_BIT		= 0x00000008,
	B_VIEW_LINE_MODES_BIT		= 0x00000010,
	B_VIEW_BLENDING_BIT			= 0x00000020,
	B_VIEW_SCALE_BIT			= 0x00000040,
	B_VIEW_FONT_ALIASING_BIT	= 0x00000080,
	B_VIEW_FRAME_BIT			= 0x00000100,
	B_VIEW_ORIGIN_BIT			= 0x00000200,
	B_VIEW_PEN_SIZE_BIT			= 0x00000400,
	B_VIEW_PEN_LOCATION_BIT		= 0x00000800,
	B_VIEW_LOW_COLOR_BIT		= 0x00008000,
	B_VIEW_VIEW_COLOR_BIT		= 0x00010000,
	B_VIEW_PATTERN_BIT			= 0x00020000,

	B_VIEW_ALL_BITS				= 0x0003ffff,

	// these used for archiving only
	B_VIEW_RESIZE_BIT			= 0x00001000,
	B_VIEW_FLAGS_BIT			= 0x00002000,
	B_VIEW_EVENT_MASK_BIT		= 0x00004000
};


class BView::Private {
public:
	Private(BView* view)
		:
		fView(view)
	{
	}

	int16 ShowLevel()
	{
		return fView->fShowLevel;
	}

	// defined in View.cpp
	bool	WillLayout();
	bool	MinMaxValid();

	bool RemoveSelf()
	{
		return fView->_RemoveSelf();
	}

	BView* fView;
};


namespace BPrivate {

class PortLink;

class ViewState {
	public:
		ViewState();

		inline bool IsValid(uint32 bit) const;
		inline bool IsAllValid() const;

		void UpdateServerFontState(BPrivate::PortLink &link);
		void UpdateServerState(BPrivate::PortLink &link);

		void UpdateFrom(BPrivate::PortLink &link);

	public:
		BPoint				pen_location;
		float				pen_size;

		rgb_color			high_color;
		rgb_color			low_color;

		// This one is not affected by pop state/push state
		rgb_color			view_color;

		::pattern			pattern;

		::drawing_mode		drawing_mode;
		BRegion				clipping_region;
		bool				clipping_region_used;
		BPoint				origin;

		// line modes
		join_mode			line_join;
		cap_mode			line_cap;
		float				miter_limit;

		// alpha blending
		source_alpha		alpha_source_mode;
		alpha_function		alpha_function_mode;

		float				scale;

		// fonts
		BFont				font;
		uint16				font_flags;
		bool				font_aliasing;
			// font aliasing. Used for printing only!

		// flags used for synchronization with app_server
		uint32				valid_flags;
		// flags used for archiving
		uint32				archiving_flags;

		// maintain our own rect as seen from the app while printing
		BRect				print_rect;
};

inline bool
ViewState::IsValid(uint32 bit) const
{
	return valid_flags & bit;
}

inline bool
ViewState::IsAllValid() const
{
	return (valid_flags & B_VIEW_ALL_BITS & ~B_VIEW_CLIP_REGION_BIT)
		== (B_VIEW_ALL_BITS & ~B_VIEW_CLIP_REGION_BIT);
}

}	// namespace BPrivate

struct _array_data_{
		// the max number of points in the array
	uint32				maxCount;
		// the current number of points in the array
	uint32				count;
		// the array of points
	ViewLineArrayInfo*	array;
};

#endif	/* VIEW_PRIVATE_H */
