/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Font.h>
#include <ScrollBar.h>
#include <Cursor.h>
#include <Bitmap.h>
#include <Picture.h>
#include <Polygon.h>
#include <Shape.h>
#include <Button.h>
#include <Shelf.h>
#include <MenuBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <Application.h>

#include <AppMisc.h>
#include <ViewAux.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <ColorUtils.h>
#include <AppServerLink.h>
#include <PortLink.h>
#include <ServerProtocol.h>

#ifdef USING_MESSAGE4
#include <MessagePrivate.h>
#endif

#include <math.h>
#include <new>
#include <stdio.h>


using std::nothrow;

//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#	define BVTRACE PrintToStream()
#else
#	define STRACE(x) ;
#	define BVTRACE ;
#endif

#define MAX_ATTACHMENT_SIZE 49152


static property_info sViewPropInfo[] = {
	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the view's frame rectangle.",0 },

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the view's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the view is hidden; false otherwise.",0 },

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the view.",0 },

	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the shelf.",0 },

	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of of child views.",0 },

	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_REVERSE_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_NAME_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 


// General Functions
//------------------------------------------------------------------------------


static inline uint32
get_uint32_color(rgb_color color)
{
	return *(uint32 *)&color;
}


static inline void
set_rgb_color(rgb_color &color, uint8 r, uint8 g, uint8 b, uint8 a = 255)
{
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
}


//	#pragma mark -


namespace BPrivate {

ViewState::ViewState()
{
	pen_location.Set(0, 0);
	pen_size = 1.0;

	// This probably needs to be set to bounds by the owning BView
	clipping_region.MakeEmpty();

	set_rgb_color(high_color, 0, 0, 0);
	set_rgb_color(low_color, 255, 255, 255);
	view_color = low_color;

	pattern = B_SOLID_HIGH;
	drawing_mode = B_OP_COPY;

	origin.Set(0, 0);

	line_join = B_MITER_JOIN;
	line_cap = B_BUTT_CAP;
	miter_limit = B_DEFAULT_MITER_LIMIT;

	alpha_source_mode = B_PIXEL_ALPHA;
	alpha_function_mode = B_ALPHA_OVERLAY;

	scale = 1.0;

	font = *be_plain_font;
	font_flags = font.Flags();
	font_aliasing = false;

	/*
		INFO: We include(invalidate) only B_VIEW_CLIP_REGION_BIT flag
		because we should get the clipping region from app_server.
		The other flags do not need to be included because the data they
		represent is already in sync with app_server - app_server uses the
		same init(default) values.
	*/
	valid_flags = ~B_VIEW_CLIP_REGION_BIT;

	archiving_flags = B_VIEW_FRAME_BIT;
}


void
ViewState::UpdateServerFontState(BPrivate::PortLink &link)
{
	link.StartMessage(AS_LAYER_SET_FONT_STATE);
	link.Attach<uint16>(font_flags);

	// always present.
	if (font_flags & B_FONT_FAMILY_AND_STYLE) {
		uint32 fontID;
		fontID = font.FamilyAndStyle();

		link.Attach<uint32>(fontID);
	}

	if (font_flags & B_FONT_SIZE)
		link.Attach<float>(font.Size());

	if (font_flags & B_FONT_SHEAR)
		link.Attach<float>(font.Shear());

	if (font_flags & B_FONT_ROTATION)
		link.Attach<float>(font.Rotation());

	if (font_flags & B_FONT_SPACING)
		link.Attach<uint8>(font.Spacing());

	if (font_flags & B_FONT_ENCODING)
		link.Attach<uint8>(font.Encoding());

	if (font_flags & B_FONT_FACE)
		link.Attach<uint16>(font.Face());

	if (font_flags & B_FONT_FLAGS)
		link.Attach<uint32>(font.Flags());
}


void
ViewState::UpdateServerState(BPrivate::PortLink &link)
{
	UpdateServerFontState(link);

	link.StartMessage(AS_LAYER_SET_STATE);
	link.Attach<BPoint>(pen_location);
	link.Attach<float>(pen_size);
	link.Attach<rgb_color>(high_color);
	link.Attach<rgb_color>(low_color);
	link.Attach< ::pattern>(pattern);
	link.Attach<int8>((int8)drawing_mode);
	link.Attach<BPoint>(origin);
	link.Attach<int8>((int8)line_join);
	link.Attach<int8>((int8)line_cap);
	link.Attach<float>(miter_limit);
	link.Attach<int8>((int8)alpha_source_mode);
	link.Attach<int8>((int8)alpha_function_mode);
	link.Attach<float>(scale);
	link.Attach<bool>(font_aliasing);

	// we send the 'local' clipping region... if we have one...
	int32 count = clipping_region.CountRects();

	link.Attach<int32>(count);
	for (int32 i = 0; i < count; i++)
		link.Attach<BRect>(clipping_region.RectAt(i));

	//	Although we might have a 'local' clipping region, when we call
	//	BView::GetClippingRegion() we ask for the 'global' one and it
	//	is kept on server, so we must invalidate B_VIEW_CLIP_REGION_BIT flag

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}


void
ViewState::UpdateFrom(BPrivate::PortLink &link)
{
	link.StartMessage(AS_LAYER_GET_STATE);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	uint32 fontID;
	float size;
	float shear;
	float rotation;
	uint8 spacing;
	uint8 encoding;
	uint16 face;
	uint32 flags;

	// read and set the font state
	link.Read<int32>((int32 *)&fontID);
	link.Read<float>(&size);
	link.Read<float>(&shear);
	link.Read<float>(&rotation);
	link.Read<int8>((int8 *)&spacing);
	link.Read<int8>((int8 *)&encoding);
	link.Read<int16>((int16 *)&face);
	link.Read<int32>((int32 *)&flags);

	font_flags = B_FONT_ALL;
	font.SetFamilyAndStyle(fontID);
	font.SetSize(size);
	font.SetShear(shear);
	font.SetRotation(rotation);
	font.SetSpacing(spacing);
	font.SetEncoding(encoding);
	font.SetFace(face);
	font.SetFlags(flags);

	// read and set view's state						 	
	link.Read<BPoint>(&pen_location);
	link.Read<float>(&pen_size);	
	link.Read<rgb_color>(&high_color);
	link.Read<rgb_color>(&low_color);
	link.Read< ::pattern>(&pattern);
	link.Read<BPoint>(&origin);

	int8 drawingMode;
	link.Read<int8>((int8 *)&drawingMode);
	drawing_mode = (::drawing_mode)drawingMode;

	link.Read<int8>((int8 *)&line_cap);
	link.Read<int8>((int8 *)&line_join);
	link.Read<float>(&miter_limit);
	link.Read<int8>((int8 *)&alpha_source_mode);
	link.Read<int8>((int8 *)&alpha_function_mode);
	link.Read<float>(&scale);
	link.Read<bool>(&font_aliasing);

	// no need to read the clipping region, as it's invalid
	// next time we need it anyway

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}

}	// namespace BPrivate


//	#pragma mark -


BView::BView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BHandler(name)
{
	_InitData(frame, name, resizingMode, flags);
}


BView::BView(BMessage *archive)
	: BHandler(archive)
{
	uint32 resizingMode;
	uint32 flags;
	BRect frame;

	archive->FindRect("_frame", &frame);
	if (archive->FindInt32("_resize_mode", (int32 *)&resizingMode) != B_OK)
		resizingMode = 0;

	if (archive->FindInt32("_flags", (int32 *)&flags) != B_OK)
		flags = 0;

	_InitData(frame, Name(), resizingMode, flags);

	font_family family;
	font_style style;
	if (archive->FindString("_fname", 0, (const char **)&family) == B_OK
		&& archive->FindString("_fname", 1, (const char **)&style) == B_OK) {
		BFont font;
		font.SetFamilyAndStyle(family, style);

		float size;
		if (archive->FindFloat("_fflt", 0, &size) == B_OK)
			font.SetSize(size);

		float shear;
		if (archive->FindFloat("_fflt", 1, &shear) == B_OK)
			font.SetShear(shear);

		float rotation;
		if (archive->FindFloat("_fflt", 2, &rotation) == B_OK)
		 font.SetRotation(rotation);

		SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
			| B_FONT_SHEAR | B_FONT_ROTATION);
	}

	rgb_color color;
	if (archive->FindInt32("_color", 0, (int32 *)&color) == B_OK)
		SetHighColor(color);
	if (archive->FindInt32("_color", 1, (int32 *)&color) == B_OK)
		SetLowColor(color);
	if (archive->FindInt32("_color", 2, (int32 *)&color) == B_OK)
		SetViewColor(color);

	uint32 evMask;
	uint32 options;
	if (archive->FindInt32("_evmask", 0, (int32 *)&evMask) == B_OK
		&& archive->FindInt32("_evmask", 1, (int32 *)&options) == B_OK)
		SetEventMask(evMask, options);

	BPoint origin;
	if (archive->FindPoint("_origin", &origin) == B_OK)
		SetOrigin(origin);

	float penSize;
	if (archive->FindFloat("_psize", &penSize) == B_OK)
		SetPenSize(penSize);

	BPoint penLocation;
	if (archive->FindPoint("_ploc", &penLocation) == B_OK)
		MovePenTo(penLocation);

	int16 lineCap;
	int16 lineJoin;
	float lineMiter;
	if (archive->FindInt16("_lmcapjoin", 0, &lineCap) == B_OK
		&& archive->FindInt16("_lmcapjoin", 1, &lineJoin) == B_OK
		&& archive->FindFloat("_lmmiter", &lineMiter) == B_OK)
		SetLineMode((cap_mode)lineCap, (join_mode)lineJoin, lineMiter);

	int16 alphaBlend;
	int16 modeBlend;
	if (archive->FindInt16("_blend", 0, &alphaBlend) == B_OK
		&& archive->FindInt16("_blend", 1, &modeBlend) == B_OK)
		SetBlendingMode( (source_alpha)alphaBlend, (alpha_function)modeBlend);

	uint32 drawingMode;
	if (archive->FindInt32("_dmod", (int32 *)&drawingMode) == B_OK)
		SetDrawingMode((drawing_mode)drawingMode);

	BMessage msg;
	int32 i = 0;
	while (archive->FindMessage("_views", i++, &msg) == B_OK) {
		BArchivable *object = instantiate_object(&msg);
		BView *child = dynamic_cast<BView *>(object);
		if (child)
			AddChild(child);
	}
}


BArchivable *
BView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data , "BView"))
		return NULL;

	return new BView(data);
}


status_t
BView::Archive(BMessage *data, bool deep) const
{
	status_t retval = BHandler::Archive(data, deep);
	if (retval != B_OK)
		return retval;

	if (fState->archiving_flags & B_VIEW_FRAME_BIT)
		data->AddRect("_frame", Bounds().OffsetToCopy(fParentOffset));

	if (fState->archiving_flags & B_VIEW_RESIZE_BIT)
		data->AddInt32("_resize_mode", ResizingMode());

	if (fState->archiving_flags & B_VIEW_FLAGS_BIT)
		data->AddInt32("_flags", Flags());

	if (fState->archiving_flags & B_VIEW_EVENT_MASK_BIT) {
		data->AddInt32("_evmask", fEventMask);
		data->AddInt32("_evmask", fEventOptions);
	}

	if (fState->archiving_flags & B_VIEW_FONT_BIT) {
		BFont font;
		GetFont(&font);

		font_family family;
		font_style style;
		font.GetFamilyAndStyle(&family, &style);
		data->AddString("_fname", family);
		data->AddString("_fname", style);

		data->AddFloat("_fflt", font.Size());
		data->AddFloat("_fflt", font.Shear());
		data->AddFloat("_fflt", font.Rotation());
	}

	// colors

	if (fState->archiving_flags & B_VIEW_HIGH_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(HighColor()));

	if (fState->archiving_flags & B_VIEW_LOW_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(LowColor()));

	if (fState->archiving_flags & B_VIEW_VIEW_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(ViewColor()));

//	NOTE: we do not use this flag any more
//	if ( 1 ){
//		data->AddInt32("_dbuf", 1);
//	}

	if (fState->archiving_flags & B_VIEW_ORIGIN_BIT)
		data->AddPoint("_origin", Origin());

	if (fState->archiving_flags & B_VIEW_PEN_SIZE_BIT)
		data->AddFloat("_psize", PenSize());

	if (fState->archiving_flags & B_VIEW_PEN_LOCATION_BIT)
		data->AddPoint("_ploc", PenLocation());

	if (fState->archiving_flags & B_VIEW_LINE_MODES_BIT) {
		data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if (fState->archiving_flags & B_VIEW_BLENDING_BIT) {
		source_alpha alphaSourceMode;
		alpha_function alphaFunctionMode;
		GetBlendingMode(&alphaSourceMode, &alphaFunctionMode);

		data->AddInt16("_blend", (int16)alphaSourceMode);
		data->AddInt16("_blend", (int16)alphaFunctionMode);
	}

	if (fState->archiving_flags & B_VIEW_DRAWING_MODE_BIT)
		data->AddInt32("_dmod", DrawingMode());

	if (deep) {
		int32 i = 0;
		BView *child;

		while ((child = ChildAt(i++)) != NULL) {
			BMessage childArchive;

			retval = child->Archive(&childArchive, deep);
			if (retval == B_OK)
				data->AddMessage("_views", &childArchive);
		}
	}

	return retval;
}


BView::~BView()
{
	STRACE(("BView(%s)::~BView()\n", this->Name()));

	if (fOwner)
		debugger("Trying to delete a view that belongs to a window. Call RemoveSelf first.");

	RemoveSelf();

	// TODO: see about BShelf! must I delete it here? is it deleted by the window?
	
	// we also delete all our children

	BView *child = fFirstChild;
	while (child) {
		BView *nextChild = child->fNextSibling;

		delete child;
		child = nextChild;
	}

	if (fVerScroller)
		fVerScroller->SetTarget((BView*)NULL);
	if (fHorScroller)
		fHorScroller->SetTarget((BView*)NULL);

	SetName(NULL);

	delete fState;
}


BRect
BView::Bounds() const
{
	// do we need to update our bounds?

// TODO: why should our frame be out of sync ever?
/*
	if (!fState->IsValid(B_VIEW_FRAME_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_COORD);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<BPoint>(const_cast<BPoint *>(&fParentOffset));
			fOwner->fLink->Read<BRect>(const_cast<BRect *>(&fBounds));
			fState->valid_flags |= B_VIEW_FRAME_BIT;
		}
	}
*/
	return fBounds;
}


void
BView::ConvertToParent(BPoint *point) const
{
	if (!fParent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our local coordinate transformation
	*point -= Origin();

	// our bounds location within the parent
	*point += fParentOffset;
}


BPoint
BView::ConvertToParent(BPoint point) const
{
	ConvertToParent(&point);

	return point;
}


void
BView::ConvertFromParent(BPoint *point) const
{
	if (!fParent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our bounds location within the parent
	*point -= fParentOffset;

	// our local coordinate transformation
	*point += Origin();
}


BPoint
BView::ConvertFromParent(BPoint point) const
{
	ConvertFromParent(&point);

	return point;
}


void
BView::ConvertToParent(BRect *rect) const
{
	if (!fParent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	BPoint origin = Origin();

	// our local coordinate transformation
	rect->OffsetBy(-origin.x, -origin.y);

	// our bounds location within the parent
	rect->OffsetBy(fParentOffset);
}


BRect
BView::ConvertToParent(BRect rect) const
{
	ConvertToParent(&rect);

	return rect;
}


void
BView::ConvertFromParent(BRect *rect) const
{
	if (!fParent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our bounds location within the parent
	rect->OffsetBy(-fParentOffset.x, -fParentOffset.y);

	// our local coordinate transformation
	rect->OffsetBy(Origin());
}


BRect
BView::ConvertFromParent(BRect rect) const
{
	ConvertFromParent(&rect);

	return rect;
}


void
BView::ConvertToScreen(BPoint *pt) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertToScreen(pt);

		return;
	}

	do_owner_check_no_pick();

	ConvertToParent(pt);
	fParent->ConvertToScreen(pt);
}


BPoint
BView::ConvertToScreen(BPoint pt) const
{
	ConvertToScreen(&pt);

	return pt;
}


void
BView::ConvertFromScreen(BPoint *pt) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertFromScreen(pt);

		return;
	}

	do_owner_check_no_pick();

	ConvertFromParent(pt);
	fParent->ConvertFromScreen(pt);
}


BPoint
BView::ConvertFromScreen(BPoint pt) const
{
	ConvertFromScreen(&pt);

	return pt;
}


void
BView::ConvertToScreen(BRect *rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertToScreen(&offset);
	rect->OffsetBy(offset);
}


BRect
BView::ConvertToScreen(BRect rect) const
{
	ConvertToScreen(&rect);

	return rect;
}


void
BView::ConvertFromScreen(BRect *rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);
	rect->OffsetBy(offset);
}


BRect
BView::ConvertFromScreen(BRect rect) const
{
	ConvertFromScreen(&rect);

	return rect;
}


uint32
BView::Flags() const 
{
	check_lock_no_pick();
	return fFlags & ~_RESIZE_MASK_;
}


void
BView::SetFlags(uint32 flags)
{
	if (Flags() == flags)
		return;

	if (fOwner) {
		if (flags & B_PULSE_NEEDED) {
			check_lock_no_pick();
			if (!fOwner->fPulseEnabled)
				fOwner->SetPulseRate(500000);
		}

		if (flags & (B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
					| B_FRAME_EVENTS | B_SUBPIXEL_PRECISE)) {
			check_lock();

			fOwner->fLink->StartMessage(AS_LAYER_SET_FLAGS);
			fOwner->fLink->Attach<uint32>(flags);
		}
	}

	/* Some useful info:
		fFlags is a unsigned long (32 bits)
		* bits 1-16 are used for BView's flags
		* bits 17-32 are used for BView' resize mask
		* _RESIZE_MASK_ is used for that. Look into View.h to see how
			it's defined
	*/
	fFlags = (flags & ~_RESIZE_MASK_) | (fFlags & _RESIZE_MASK_);

	fState->archiving_flags |= B_VIEW_FLAGS_BIT;
}


BRect
BView::Frame() const 
{
	check_lock_no_pick();

	return Bounds().OffsetToCopy(fParentOffset.x, fParentOffset.y);
}


void
BView::Hide()
{
	if (fOwner && fShowLevel == 0) {
		check_lock();
		fOwner->fLink->StartMessage(AS_LAYER_HIDE);
	}
	fShowLevel++;
}


void
BView::Show()
{
	fShowLevel--;
	if (fOwner && fShowLevel == 0) {
		check_lock();
		fOwner->fLink->StartMessage(AS_LAYER_SHOW);
	}
}


bool
BView::IsFocus() const 
{
	if (fOwner) {
		check_lock_no_pick();
		return fOwner->CurrentFocus() == this;
	} else
		return false;
}


bool 
BView::IsHidden(const BView *lookingFrom) const
{
	if (fShowLevel > 0)
		return true;

	// may we be egocentric?
	if (lookingFrom == this)
		return false;

	// we have the same visibility state as our
	// parent, if there is one
	if (fParent)
		return fParent->IsHidden(lookingFrom);

	// if we're the top view, and we're interested
	// in the "global" view, we're inheriting the
	// state of the window's visibility
	if (fOwner && lookingFrom == NULL)
		return fOwner->IsHidden();

	return false;
}


bool
BView::IsHidden() const
{
	return IsHidden(NULL);
}


bool
BView::IsPrinting() const 
{
	return f_is_printing;
}


BPoint
BView::LeftTop() const 
{
	return Bounds().LeftTop();
}


void
BView::SetOrigin(BPoint pt) 
{
	SetOrigin(pt.x, pt.y);
}


void
BView::SetOrigin(float x, float y)
{
	if (fState->IsValid(B_VIEW_ORIGIN_BIT)
		&& x == fState->origin.x && y == fState->origin.y)
		return;

	if (do_owner_check()) {
		fOwner->fLink->StartMessage(AS_LAYER_SET_ORIGIN);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_ORIGIN_BIT;
	}

	// our local coord system origin has changed, so when archiving we'll add this too
	fState->archiving_flags |= B_VIEW_ORIGIN_BIT;
}


BPoint
BView::Origin() const
{
	if (!fState->IsValid(B_VIEW_ORIGIN_BIT)) {
		do_owner_check();

		fOwner->fLink->StartMessage(AS_LAYER_GET_ORIGIN);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<BPoint>(&fState->origin);

			fState->valid_flags |= B_VIEW_ORIGIN_BIT;
		}
	}

	return fState->origin;
}


void
BView::SetResizingMode(uint32 mode) 
{
	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_RESIZE_MODE);
		fOwner->fLink->Attach<uint32>(mode);
	}

	// look at SetFlags() for more info on the below line
	fFlags = (fFlags & ~_RESIZE_MASK_) | (mode & _RESIZE_MASK_);

	// our resize mode has changed, so when archiving we'll add this too
	fState->archiving_flags |= B_VIEW_RESIZE_BIT;
}


uint32
BView::ResizingMode() const
{
	return fFlags & _RESIZE_MASK_;
}


void
BView::SetViewCursor(const BCursor *cursor, bool sync)
{
	if (!cursor)
		return;

	if (!fOwner)
		debugger("View method requires owner and doesn't have one");

	check_lock();

	fOwner->fLink->StartMessage(AS_LAYER_SET_CURSOR);
	fOwner->fLink->Attach<int32>(cursor->fServerToken);

	if (sync)
		fOwner->fLink->Flush();
}


void
BView::Flush() const 
{
	if (fOwner)
		fOwner->Flush();
}


void
BView::Sync() const 
{
	do_owner_check_no_pick();
	if (fOwner)
		fOwner->Sync();
}


BWindow *
BView::Window() const 
{
	return fOwner;
}


//	#pragma mark -
// Hook Functions


void
BView::AttachedToWindow()
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::AttachedToWindow()\n", Name()));
}


void
BView::AllAttached()
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::AllAttached()\n", Name()));
}


void
BView::DetachedFromWindow()
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::DetachedFromWindow()\n", Name()));
}


void
BView::AllDetached()
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::AllDetached()\n", Name()));
}


void
BView::Draw(BRect updateRect)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::Draw()\n", Name()));
}


void
BView::DrawAfterChildren(BRect r)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DrawAfterChildren()\n", Name()));
}


void
BView::FrameMoved(BPoint newPosition)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::FrameMoved()\n", Name()));
}


void
BView::FrameResized(float newWidth, float newHeight)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::FrameResized()\n", Name()));
}


void
BView::GetPreferredSize(float* _width, float* _height)
{
	STRACE(("\tHOOK: BView(%s)::GetPreferredSize()\n", Name()));

	*_width = fBounds.Width();
	*_height = fBounds.Height();
}


void
BView::ResizeToPreferred()
{
	STRACE(("\tHOOK: BView(%s)::ResizeToPreferred()\n", Name()));

	float width;
	float height;
	GetPreferredSize(&width, &height);

	ResizeTo(width, height); 
}


void
BView::KeyDown(const char* bytes, int32 numBytes)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::KeyDown()\n", Name()));

	if (Window())
		Window()->_KeyboardNavigation();
}


void
BView::KeyUp(const char* bytes, int32 numBytes)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::KeyUp()\n", Name()));
}


void
BView::MouseDown(BPoint where)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::MouseDown()\n", Name()));
}


void
BView::MouseUp(BPoint where)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::MouseUp()\n", Name()));
}


void
BView::MouseMoved(BPoint where, uint32 code, const BMessage* a_message)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::MouseMoved()\n", Name()));
}


void
BView::Pulse()
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::Pulse()\n", Name()));
}


void
BView::TargetedByScrollView(BScrollView* scroll_view)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::TargetedByScrollView()\n", Name()));
}


void
BView::WindowActivated(bool state)
{
	// Hook function
	STRACE(("\tHOOK: BView(%s)::WindowActivated()\n", Name()));
}


//	#pragma mark -
//	Input Functions


void
BView::BeginRectTracking(BRect startRect, uint32 style)
{
	if (do_owner_check()) {
		fOwner->fLink->StartMessage(AS_LAYER_BEGIN_RECT_TRACK);
		fOwner->fLink->Attach<BRect>(startRect);
		fOwner->fLink->Attach<int32>(style);
	}
}


void
BView::EndRectTracking()
{
	if (do_owner_check())
		fOwner->fLink->StartMessage(AS_LAYER_END_RECT_TRACK);
}


void
BView::DragMessage(BMessage *message, BRect dragRect, BHandler *replyTo)
{
	if (!message || !dragRect.IsValid())
		return;

	if (replyTo == NULL)
		replyTo = this;
	
	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();

	BPoint offset;

	if (!message->HasInt32("buttons")) {
		BMessage *msg = fOwner->CurrentMessage();
		uint32 buttons;

		if (msg) {
			if (msg->FindInt32("buttons", (int32 *)&buttons) != B_OK) {
				BPoint point;
				GetMouse(&point, &buttons, false);
			}

			BPoint point;
			if (msg->FindPoint("be:view_where", &point) == B_OK)
				offset = point - dragRect.LeftTop();
		} else {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);		
	}

#ifndef USING_MESSAGE4
	_set_message_reply_(message, BMessenger(replyTo, replyTo->Looper()));
#else
	BMessage::Private(message).SetReply(BMessenger(replyTo, replyTo->Looper()));
#endif

	int32 bufferSize = message->FlattenedSize();
	char* buffer = new (nothrow) char[bufferSize];
	if (buffer) {
		message->Flatten(buffer, bufferSize);
	
		fOwner->fLink->StartMessage(AS_LAYER_DRAG_RECT);
		fOwner->fLink->Attach<BRect>(dragRect);
		fOwner->fLink->Attach<BPoint>(offset);	
		fOwner->fLink->Attach<int32>(bufferSize);
		fOwner->fLink->Attach(buffer, bufferSize);
		fOwner->fLink->Flush();
	
		delete [] buffer;
	} else {
		fprintf(stderr, "BView::DragMessage() - no memory to flatten drag message\n");
	}
}


void
BView::DragMessage(BMessage *message, BBitmap *image, BPoint offset,
	BHandler *replyTo)
{
	DragMessage(message, image, B_OP_COPY, offset, replyTo);
}


void
BView::DragMessage(BMessage *message, BBitmap *image,
	drawing_mode dragMode, BPoint offset, BHandler *replyTo)
{
	// ToDo: is this correct? Isn't \a image allowed to be NULL?
	if (message == NULL || image == NULL)
		return;

	if (replyTo == NULL)
		replyTo = this;

	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();

	if (!message->HasInt32("buttons")) {
		BMessage *msg = fOwner->CurrentMessage();
		uint32 buttons;

		if (msg == NULL || msg->FindInt32("buttons", (int32 *)&buttons) != B_OK) {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);		
	}

#ifndef USING_MESSAGE4
	_set_message_reply_(message, BMessenger(replyTo, replyTo->Looper()));
#else
	BMessage::Private(message).SetReply(BMessenger(replyTo, replyTo->Looper()));
#endif

	int32 bufferSize = message->FlattenedSize();
	char* buffer = new (nothrow) char[bufferSize];
	if (buffer) {
		message->Flatten(buffer, bufferSize);
	
		fOwner->fLink->StartMessage(AS_LAYER_DRAG_IMAGE);
		fOwner->fLink->Attach<int32>(image->get_server_token());
		fOwner->fLink->Attach<int32>((int32)dragMode);
		fOwner->fLink->Attach<BPoint>(offset);
		fOwner->fLink->Attach<int32>(bufferSize);
		fOwner->fLink->Attach(buffer, bufferSize);

		// we need to wait for the server
		// to actually process this message
		// before we can delete the bitmap
		int32 code;
		fOwner->fLink->FlushWithReply(code);
	
		delete [] buffer;
	} else {
		fprintf(stderr, "BView::DragMessage() - no memory to flatten drag message\n");
	}

	// TODO: in app_server the bitmap refCount must be incremented
	// WRITE this into specs!!!!

	delete image;
}


void
BView::GetMouse(BPoint *location, uint32 *buttons, bool checkMessageQueue)
{
	do_owner_check();

	if (checkMessageQueue) {
		BMessageQueue *queue = Window()->MessageQueue();
		queue->Lock();

		Window()->UpdateIfNeeded();

		// Look out for mouse update messages

		BMessage *msg;
		for (int32 i = 0; (msg = queue->FindMessage(i)) != NULL; i++) {
			switch (msg->what) {
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
				case B_MOUSE_MOVED:
					msg->FindPoint("screen_where", location);
					msg->FindInt32("buttons", (int32 *)buttons);

					ConvertFromScreen(location);

					queue->RemoveMessage(msg);
					delete msg;

					queue->Unlock();
					return;
			}
		}
		queue->Unlock();
	}

	// If no mouse update message has been found in the message queue, 
	// we get the current mouse location and buttons from the app_server

	fOwner->fLink->StartMessage(AS_GET_MOUSE);

	int32 code;
	if (fOwner->fLink->FlushWithReply(code) == B_OK
		&& code == B_OK) {
		fOwner->fLink->Read<BPoint>(location);
		fOwner->fLink->Read<uint32>(buttons);

		// TODO: See above comment about coordinates
		ConvertFromScreen(location);
	} else
		buttons = 0;
}


void
BView::MakeFocus(bool focusState)
{
	if (fOwner) {
		// TODO: If this view has focus and focusState==false,
		// will there really be no other view with focus? No
		// cycling to the next one?
		BView *focus = fOwner->CurrentFocus();
		if (focusState) {
			// Unfocus a previous focus view
			if (focus && focus != this)
				focus->MakeFocus(false);
			// if we want to make this view the current focus view
			fOwner->fFocus = this;
			fOwner->SetPreferredHandler(this);
		} else {
			// we want to unfocus this view, but only if it actually has focus
			if (focus == this) {
				fOwner->fFocus = NULL;
				fOwner->SetPreferredHandler(NULL);
			}
		}
	}
}


BScrollBar *
BView::ScrollBar(orientation posture) const
{
	switch (posture) {
		case B_VERTICAL:
			return fVerScroller;

		case B_HORIZONTAL:
			return fHorScroller;

		default:
			return NULL;
	}
}


void
BView::ScrollBy(float dh, float dv)
{
	// scrolling by fractional values is not supported, is it?
	dh = roundf(dh);
	dv = roundf(dv);

	// no reason to process this further if no scroll is intended.
	if (dh == 0 && dv == 0)
		return;

	check_lock();

	// if we're attached to a window tell app_server about this change
	if (fOwner) {
		fOwner->fLink->StartMessage(AS_LAYER_SCROLL);
		fOwner->fLink->Attach<float>(dh);
		fOwner->fLink->Attach<float>(dv);

		fOwner->fLink->Flush();

		fState->valid_flags &= ~(B_VIEW_FRAME_BIT | B_VIEW_ORIGIN_BIT);
	}

	// we modify our bounds rectangle by dh/dv coord units hor/ver.
	fBounds.OffsetBy(dh, dv);

	// then set the new values of the scrollbars
	if (fHorScroller)
		fHorScroller->SetValue(fBounds.left);
	if (fVerScroller)		
		fVerScroller->SetValue(fBounds.top);
}


void
BView::ScrollTo(BPoint where)
{
	ScrollBy(where.x - fBounds.left, where.y - fBounds.top);
}


status_t
BView::SetEventMask(uint32 mask, uint32 options)
{
	if (fEventMask == mask && fEventOptions == options)
		return B_ERROR;

	fEventMask = mask | (fEventMask & 0xFFFF0000);
	fEventOptions = options;

	fState->archiving_flags |= B_VIEW_EVENT_MASK_BIT;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);
	}

	return B_OK;
}


uint32
BView::EventMask()
{
	return fEventMask;
}


status_t
BView::SetMouseEventMask(uint32 mask, uint32 options)
{
	// Just don't do anything if the view is not yet attached
	// or we were called outside of BView::MouseDown()
	if (fOwner != NULL
		&& fOwner->CurrentMessage() != NULL
		&& fOwner->CurrentMessage()->what == B_MOUSE_DOWN) {
		check_lock();
		fOwner->fLink->StartMessage(AS_LAYER_SET_MOUSE_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);

		return B_OK;
	}

	return B_ERROR;
}


//	#pragma mark -
//	Graphic State Functions


void
BView::SetLineMode(cap_mode lineCap, join_mode lineJoin, float miterLimit)
{
	if (fState->IsValid(B_VIEW_LINE_MODES_BIT)
		&& lineCap == fState->line_cap && lineJoin == fState->line_join
		&& miterLimit == fState->miter_limit)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_LINE_MODE);
		fOwner->fLink->Attach<int8>((int8)lineCap);
		fOwner->fLink->Attach<int8>((int8)lineJoin);
		fOwner->fLink->Attach<float>(miterLimit);

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	fState->line_cap = lineCap;
	fState->line_join = lineJoin;
	fState->miter_limit = miterLimit;

	fState->archiving_flags |= B_VIEW_LINE_MODES_BIT;
}	


join_mode
BView::LineJoinMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_join;
}


cap_mode
BView::LineCapMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_cap;
}


float
BView::LineMiterLimit() const
{
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_LINE_MODE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			int8 cap, join;
			fOwner->fLink->Read<int8>((int8 *)&cap);
			fOwner->fLink->Read<int8>((int8 *)&join);
			fOwner->fLink->Read<float>(&fState->miter_limit);

			fState->line_cap = (cap_mode)cap;
			fState->line_join = (join_mode)join;
		}

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	return fState->miter_limit;
}


void
BView::PushState()
{
	do_owner_check();

	fOwner->fLink->StartMessage(AS_LAYER_PUSH_STATE);

	// initialize origin and scale
	fState->valid_flags |= B_VIEW_SCALE_BIT | B_VIEW_ORIGIN_BIT;
	fState->scale = 1.0f;
	fState->origin.Set(0, 0);
}


void
BView::PopState()
{
	do_owner_check();

	fOwner->fLink->StartMessage(AS_LAYER_POP_STATE);

	// invalidate all flags (except those that are not part of pop/push)
	fState->valid_flags = B_VIEW_VIEW_COLOR_BIT;
}


void
BView::SetScale(float scale) const
{
	if (fState->IsValid(B_VIEW_SCALE_BIT) && scale == fState->scale)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_SCALE);
		fOwner->fLink->Attach<float>(scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	fState->scale = scale;
	fState->archiving_flags |= B_VIEW_SCALE_BIT;
}


float
BView::Scale() const
{
	if (!fState->IsValid(B_VIEW_SCALE_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_SCALE);

 		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK)
			fOwner->fLink->Read<float>(&fState->scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}


void
BView::SetDrawingMode(drawing_mode mode)
{
	if (fState->IsValid(B_VIEW_DRAWING_MODE_BIT)
		&& mode == fState->drawing_mode)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_DRAWING_MODE);
		fOwner->fLink->Attach<int8>((int8)mode);

		fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
	}

	fState->drawing_mode = mode;
	fState->archiving_flags |= B_VIEW_DRAWING_MODE_BIT;
}


drawing_mode
BView::DrawingMode() const
{
	if (!fState->IsValid(B_VIEW_DRAWING_MODE_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_DRAWING_MODE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			int8 drawingMode;
			fOwner->fLink->Read<int8>(&drawingMode);

			fState->drawing_mode = (drawing_mode)drawingMode;
			fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
		}
	}

	return fState->drawing_mode;
}


void
BView::SetBlendingMode(source_alpha sourceAlpha, alpha_function alphaFunction)
{
	if (fState->IsValid(B_VIEW_BLENDING_BIT)
		&& sourceAlpha == fState->alpha_source_mode
		&& alphaFunction == fState->alpha_function_mode)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_BLENDING_MODE);
		fOwner->fLink->Attach<int8>((int8)sourceAlpha);
		fOwner->fLink->Attach<int8>((int8)alphaFunction);

		fState->valid_flags |= B_VIEW_BLENDING_BIT;
	}

	fState->alpha_source_mode = sourceAlpha;
	fState->alpha_function_mode = alphaFunction;

	fState->archiving_flags |= B_VIEW_BLENDING_BIT;
}


void
BView::GetBlendingMode(source_alpha *_sourceAlpha,
	alpha_function *_alphaFunction) const
{
	if (!fState->IsValid(B_VIEW_BLENDING_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_BLENDING_MODE);

		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK
 			&& code == B_OK) {
			int8 alphaSourceMode, alphaFunctionMode;
			fOwner->fLink->Read<int8>(&alphaSourceMode);
			fOwner->fLink->Read<int8>(&alphaFunctionMode);

			fState->alpha_source_mode = (source_alpha)alphaSourceMode;
			fState->alpha_function_mode = (alpha_function)alphaFunctionMode;

			fState->valid_flags |= B_VIEW_BLENDING_BIT;
		}
	}

	if (_sourceAlpha)
		*_sourceAlpha = fState->alpha_source_mode;

	if (_alphaFunction)
		*_alphaFunction = fState->alpha_function_mode;
}


void
BView::MovePenTo(BPoint point)
{
	MovePenTo(point.x, point.y);
}


void
BView::MovePenTo(float x, float y)
{
	if (fState->IsValid(B_VIEW_PEN_LOCATION_BIT)
		&& x == fState->pen_location.x && y == fState->pen_location.y)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_PEN_LOC);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
	}

	fState->pen_location.x = x;
	fState->pen_location.y = y;	

	fState->archiving_flags |= B_VIEW_PEN_LOCATION_BIT;
}


void
BView::MovePenBy(float x, float y)
{
	// this will update the pen location if necessary
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT))
		PenLocation();

	MovePenTo(fState->pen_location.x + x, fState->pen_location.y + y);
}


BPoint
BView::PenLocation() const
{
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_PEN_LOC);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<BPoint>(&fState->pen_location);

			fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
		}
	}

	return fState->pen_location;
}


void
BView::SetPenSize(float size)
{
	if (fState->IsValid(B_VIEW_PEN_SIZE_BIT) && size == fState->pen_size)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_PEN_SIZE);
		fOwner->fLink->Attach<float>(size);

		fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;	
	}

	fState->pen_size = size;
	fState->archiving_flags	|= B_VIEW_PEN_SIZE_BIT;
}


float
BView::PenSize() const
{
	if (!fState->IsValid(B_VIEW_PEN_SIZE_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_PEN_SIZE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<float>(&fState->pen_size);

			fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;
		}
	}

	return fState->pen_size;
}


void
BView::SetHighColor(rgb_color color)
{
	// are we up-to-date already?
	if (fState->IsValid(B_VIEW_HIGH_COLOR_BIT)
		&& fState->high_color == color)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_HIGH_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;	
	}

	set_rgb_color(fState->high_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_HIGH_COLOR_BIT;
}


rgb_color
BView::HighColor() const
{
	if (!fState->IsValid(B_VIEW_HIGH_COLOR_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_HIGH_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->high_color);

			fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
		}
	}

	return fState->high_color;
}


void
BView::SetLowColor(rgb_color color)
{
	if (fState->IsValid(B_VIEW_LOW_COLOR_BIT)
		&& fState->low_color == color)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_LOW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
	}

	set_rgb_color(fState->low_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_LOW_COLOR_BIT;
}


rgb_color
BView::LowColor() const
{
	if (!fState->IsValid(B_VIEW_LOW_COLOR_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_LOW_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->low_color);

			fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
		}
	}

	return fState->low_color;
}


void
BView::SetViewColor(rgb_color color)
{
	if (fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && fState->view_color == color)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_VIEW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
	}

	set_rgb_color(fState->view_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_VIEW_COLOR_BIT;
}


rgb_color
BView::ViewColor() const
{
	if (!fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_GET_VIEW_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->view_color);

			fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
		}
	}

	return fState->view_color;
}


void
BView::ForceFontAliasing(bool enable)
{
	if (fState->IsValid(B_VIEW_FONT_ALIASING_BIT) && enable == fState->font_aliasing)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_PRINT_ALIASING);
		fOwner->fLink->Attach<bool>(enable);

		fState->valid_flags |= B_VIEW_FONT_ALIASING_BIT;
	}

	fState->font_aliasing = enable;
	fState->archiving_flags |= B_VIEW_FONT_ALIASING_BIT;
}


void
BView::SetFont(const BFont* font, uint32 mask)
{
	if (!font || mask == 0)
		return;

	if (mask == B_FONT_ALL) {
		fState->font = *font;
	} else {
		// ToDo: move this into a BFont method
		if (mask & B_FONT_FAMILY_AND_STYLE)
			fState->font.SetFamilyAndStyle(font->FamilyAndStyle());

		if (mask & B_FONT_SIZE)
			fState->font.SetSize(font->Size());

		if (mask & B_FONT_SHEAR)
			fState->font.SetShear(font->Shear());

		if (mask & B_FONT_ROTATION)
			fState->font.SetRotation(font->Rotation());		

		if (mask & B_FONT_SPACING)
			fState->font.SetSpacing(font->Spacing());

		if (mask & B_FONT_ENCODING)
			fState->font.SetEncoding(font->Encoding());		

		if (mask & B_FONT_FACE)
			fState->font.SetFace(font->Face());

		if (mask & B_FONT_FLAGS)
			fState->font.SetFlags(font->Flags());
	}

	fState->font_flags = mask;

	if (fOwner) {
		check_lock();
		do_owner_check();

		fState->UpdateServerFontState(*fOwner->fLink);
	}
}


#if !_PR3_COMPATIBLE_
void
BView::GetFont(BFont *font) const
#else
void
BView:GetFont(BFont *font)
#endif
{
	*font = fState->font;
}


void
BView::GetFontHeight(font_height *height) const
{
	fState->font.GetHeight(height);
}


void
BView::SetFontSize(float size)
{
	BFont font;
	font.SetSize(size);

	SetFont(&font, B_FONT_SIZE);
}


float
BView::StringWidth(const char *string) const
{
	return fState->font.StringWidth(string);
}


float
BView::StringWidth(const char* string, int32 length) const
{
	return fState->font.StringWidth(string, length);
}


void
BView::GetStringWidths(char *stringArray[],int32 lengthArray[],
	int32 numStrings, float widthArray[]) const
{
	fState->font.GetStringWidths(const_cast<const char **>(stringArray),
		const_cast<const int32 *>(lengthArray), numStrings, widthArray);
}


void
BView::TruncateString(BString *in_out, uint32 mode, float width) const
{
	fState->font.TruncateString(in_out, mode, width);
}


void
BView::ClipToPicture(BPicture *picture, BPoint where, bool sync)
{
	DoPictureClip(picture, where, false, sync);
}


void
BView::ClipToInversePicture(BPicture *picture,
	BPoint where, bool sync)
{
	DoPictureClip(picture, where, true, sync);
}


void
BView::GetClippingRegion(BRegion* region) const
{
	if (!region)
		return;

	// TODO: For now, the clipping bit is ignored, since the client has no
	// idea when the clipping in the server changed. -Stephan

//	if (!fState->IsValid(B_VIEW_CLIP_REGION_BIT) && fOwner && do_owner_check()) {
	if (fOwner && do_owner_check()) {
		fOwner->fLink->StartMessage(AS_LAYER_GET_CLIP_REGION);

 		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK
 			&& code == B_OK) {
			int32 count;
			fOwner->fLink->Read<int32>(&count);

			fState->clipping_region.MakeEmpty();

			for (int32 i = 0; i < count; i++) {
				BRect rect;
				fOwner->fLink->Read<BRect>(&rect);

				fState->clipping_region.Include(rect);
			}
			fState->valid_flags |= B_VIEW_CLIP_REGION_BIT;
		}
	} else {
		fState->clipping_region.MakeEmpty();
	}

	*region = fState->clipping_region;
}


void
BView::ConstrainClippingRegion(BRegion* region)
{
	if (do_owner_check()) {
		int32 count = 0;
		if (region)
			count = region->CountRects();

		fOwner->fLink->StartMessage(AS_LAYER_SET_CLIP_REGION);

		// '0' means that in the app_server, there won't be any 'local'
		// clipping region (it will be = NULL)

		// TODO: note this in the specs
		fOwner->fLink->Attach<int32>(count);
		for (int32 i = 0; i < count; i++)
			fOwner->fLink->Attach<BRect>(region->RectAt(i));

		// we flush here because app_server waits for all the rects
		fOwner->fLink->Flush();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
		fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
	}
}


//	#pragma mark - Drawing Functions
//---------------------------------------------------------------------------


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BRect srcRect, BRect dstRect)
{
	if (!bitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP);
		fOwner->fLink->Attach<int32>(bitmap->get_server_token());
		fOwner->fLink->Attach<BRect>(dstRect);
		fOwner->fLink->Attach<BRect>(srcRect);

		_FlushIfNotInTransaction();
	}
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BRect dstRect)
{
	DrawBitmapAsync(bitmap, bitmap->Bounds(), dstRect);
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap)
{
	DrawBitmapAsync(bitmap, PenLocation());
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BPoint where)
{
	if (bitmap == NULL)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP);
		fOwner->fLink->Attach<int32>(bitmap->get_server_token());
		BRect src = bitmap->Bounds();
		BRect dst = src.OffsetToCopy(where);
		fOwner->fLink->Attach<BRect>(dst);
		fOwner->fLink->Attach<BRect>(src);

		_FlushIfNotInTransaction();
	}
}


void
BView::DrawBitmap(const BBitmap *bitmap)
{
	DrawBitmap(bitmap, PenLocation());
}


void
BView::DrawBitmap(const BBitmap *bitmap, BPoint where)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, where);
		Sync();
	}
}


void
BView::DrawBitmap(const BBitmap *bitmap, BRect dstRect)
{
	DrawBitmap(bitmap, bitmap->Bounds(), dstRect);
}


void
BView::DrawBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, srcRect, dstRect);
		Sync();
	}
}


void
BView::DrawChar(char c)
{
	DrawString(&c, 1, PenLocation());
}


void
BView::DrawChar(char c, BPoint location)
{
	DrawString(&c, 1, location);
}


void
BView::DrawString(const char *string, escapement_delta *delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), PenLocation(), delta);
}


void
BView::DrawString(const char *string, BPoint location, escapement_delta *delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), location, delta);
}


void
BView::DrawString(const char *string, int32 length, escapement_delta *delta)
{
	DrawString(string, length, PenLocation(), delta);
}


void
BView::DrawString(const char *string, int32 length, BPoint location,
	escapement_delta *delta)
{
	if (string == NULL || length < 1)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_DRAW_STRING);
		fOwner->fLink->Attach<int32>(length);
		fOwner->fLink->Attach<BPoint>(location);

		// Quite often delta will be NULL, so we have to accomodate this.
		if (delta)
			fOwner->fLink->Attach<escapement_delta>(*delta);
		else {
			escapement_delta tdelta;
			tdelta.space = 0;
			tdelta.nonspace = 0;

			fOwner->fLink->Attach<escapement_delta>(tdelta);
		}

		fOwner->fLink->AttachString(string, length);

		_FlushIfNotInTransaction();

		// this modifies our pen location, so we invalidate the flag.
		fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
	}
}


void
BView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
	pattern p)
{
	StrokeEllipse(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), p);
}


void
BView::StrokeEllipse(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ELLIPSE);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::FillEllipse(BPoint center, float xRadius, float yRadius,
	::pattern pattern)
{
	FillEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), pattern);
}


void
BView::FillEllipse(BRect rect, ::pattern pattern) 
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ELLIPSE);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::StrokeArc(BPoint center, float xRadius, float yRadius,
	float startAngle, float arcAngle, pattern p)
{
	StrokeArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, p);
}


void
BView::StrokeArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
BView::FillArc(BPoint center,float xRadius, float yRadius,
	float startAngle, float arcAngle, ::pattern pattern)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
BView::FillArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
BView::StrokeBezier(BPoint *controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
BView::FillBezier(BPoint *controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
BView::StrokePolygon(const BPolygon *polygon, bool closed, pattern p)
{
	if (!polygon)
		return;

	StrokePolygon(polygon->fPts, polygon->fCount, polygon->Frame(), closed, p);
}


void
BView::StrokePolygon(const BPoint *ptArray, int32 numPoints, bool closed, pattern p)
{
	BPolygon polygon(ptArray, numPoints);
	
	StrokePolygon(polygon.fPts, polygon.fCount, polygon.Frame(), closed, p);
}


void
BView::StrokePolygon(const BPoint *ptArray, int32 numPoints, BRect bounds,
	bool closed, ::pattern pattern)
{
	if (!ptArray
		|| numPoints <= 1
		|| fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	BPolygon polygon(ptArray, numPoints);
	polygon.MapTo(polygon.Frame(), bounds);

	if (fOwner->fLink->StartMessage(AS_STROKE_POLYGON,
			polygon.fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(bool) + sizeof(int32))
				== B_OK) {
		fOwner->fLink->Attach<BRect>(polygon.Frame());
		fOwner->fLink->Attach<bool>(closed);
		fOwner->fLink->Attach<int32>(polygon.fCount);
		fOwner->fLink->Attach(polygon.fPts, polygon.fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		// TODO: send via an area
		fprintf(stderr, "ERROR: polygon to big for BPortLink!\n");
	}
}


void
BView::FillPolygon(const BPolygon *polygon, ::pattern pattern)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if (fOwner->fLink->StartMessage(AS_FILL_POLYGON,
			polygon->fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(int32)) == B_OK) {
		fOwner->fLink->Attach<BRect>(polygon->Frame());
		fOwner->fLink->Attach<int32>(polygon->fCount);
		fOwner->fLink->Attach(polygon->fPts, polygon->fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		// TODO: send via an area
		fprintf(stderr, "ERROR: polygon to big for BPortLink!\n");
	}
}


void
BView::FillPolygon(const BPoint *ptArray, int32 numPts, ::pattern pattern)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);
	FillPolygon(&polygon, pattern);
}


void
BView::FillPolygon(const BPoint *ptArray, int32 numPts, BRect bounds,
	pattern p)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, p);
}


void
BView::StrokeRect(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_RECT);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::FillRect(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_RECT);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::StrokeRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ROUNDRECT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);

	_FlushIfNotInTransaction();
}


void
BView::FillRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ROUNDRECT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);

	_FlushIfNotInTransaction();
}


void
BView::FillRegion(BRegion *region, ::pattern pattern)
{
	if (region == NULL || fOwner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	int32 count = region->CountRects();

	if (count * sizeof(BRect) < MAX_ATTACHMENT_SIZE) {
		fOwner->fLink->StartMessage(AS_FILL_REGION);
		fOwner->fLink->Attach<int32>(count);

		for (int32 i = 0; i < count; i++)
			fOwner->fLink->Attach<BRect>(region->RectAt(i));

		_FlushIfNotInTransaction();
	} else {
		// TODO: send via area
	}
}


void
BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_TRIANGLE);
	fOwner->fLink->Attach<BPoint>(pt1);
	fOwner->fLink->Attach<BPoint>(pt2);
	fOwner->fLink->Attach<BPoint>(pt3);
	fOwner->fLink->Attach<BRect>(bounds);

	_FlushIfNotInTransaction();
}


void
BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (fOwner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;

		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		StrokeTriangle(pt1, pt2, pt3, bounds, p);
	}
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (fOwner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		FillTriangle(pt1, pt2, pt3, bounds, p);
	}
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_TRIANGLE);
	fOwner->fLink->Attach<BPoint>(pt1);
	fOwner->fLink->Attach<BPoint>(pt2);
	fOwner->fLink->Attach<BPoint>(pt3);
	fOwner->fLink->Attach<BRect>(bounds);

	_FlushIfNotInTransaction();
}


void
BView::StrokeLine(BPoint toPt, pattern p)
{
	StrokeLine(PenLocation(), toPt, p);
}


void
BView::StrokeLine(BPoint pt0, BPoint pt1, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_LINE);
	fOwner->fLink->Attach<BPoint>(pt0);
	fOwner->fLink->Attach<BPoint>(pt1);

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
BView::StrokeShape(BShape *shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data *sd = (shape_data *)shape->fPrivateData;
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if ((sd->opCount * sizeof(uint32)) + (sd->ptCount * sizeof(BPoint)) < MAX_ATTACHMENT_SIZE) {
		fOwner->fLink->StartMessage(AS_STROKE_SHAPE);
		fOwner->fLink->Attach<BRect>(shape->Bounds());
		fOwner->fLink->Attach<int32>(sd->opCount);
		fOwner->fLink->Attach<int32>(sd->ptCount);
		fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(uint32));
		fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		// TODO: send via an area
	}
}


void
BView::FillShape(BShape *shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data *sd = (shape_data *)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if ((sd->opCount * sizeof(uint32)) + (sd->ptCount * sizeof(BPoint)) < MAX_ATTACHMENT_SIZE) {
		fOwner->fLink->StartMessage(AS_FILL_SHAPE);
		fOwner->fLink->Attach<BRect>(shape->Bounds());
		fOwner->fLink->Attach<int32>(sd->opCount);
		fOwner->fLink->Attach<int32>(sd->ptCount);
		fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(int32));
		fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		// TODO: send via an area
		// BTW, in a perfect world, the fLink API would take care of that -- axeld.
	}
}


void
BView::BeginLineArray(int32 count)
{
	if (fOwner == NULL)
		return;

	if (count <= 0)
		debugger("Calling BeginLineArray with a count <= 0");

	check_lock_no_pick();

	if (comm) {
		debugger("Can't nest BeginLineArray calls");
			// not fatal, but it helps during
			// development of your app and is in
			// line with R5...
		delete [] comm->array;
		delete comm;
	}

	comm = new _array_data_;

	comm->maxCount = count;
	comm->count = 0;
	comm->array = new _array_hdr_[count];
}


void
BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (fOwner == NULL)
		return;

	if (!comm)
		debugger("BeginLineArray must be called before using AddLine");

	check_lock_no_pick();

	if (comm->count < comm->maxCount) {
		comm->array[comm->count].startX = pt0.x;
		comm->array[comm->count].startY = pt0.y;
		comm->array[comm->count].endX = pt1.x;
		comm->array[comm->count].endY = pt1.y;

		set_rgb_color(comm->array[comm->count].color,
			col.red, col.green, col.blue, col.alpha);

		comm->count++;
	}
}


void
BView::EndLineArray()
{
	if (fOwner == NULL)
		return;

	if (!comm)
		debugger("Can't call EndLineArray before BeginLineArray");

	check_lock();

	fOwner->fLink->StartMessage(AS_STROKE_LINEARRAY);
	fOwner->fLink->Attach<int32>(comm->count);
	fOwner->fLink->Attach(comm->array, comm->count * sizeof(_array_hdr_));

	_FlushIfNotInTransaction();

	delete [] comm->array;
	delete comm;
	comm = NULL;
}


void
BView::BeginPicture(BPicture *picture)
{
	if (do_owner_check() && picture && picture->usurped == NULL) {
		picture->usurp(cpicture);
		cpicture = picture;

		fOwner->fLink->StartMessage(AS_LAYER_BEGIN_PICTURE);
	}
}


void
BView::AppendToPicture(BPicture *picture)
{
	check_lock();

	if (picture && picture->usurped == NULL) {
		int32 token = picture->token;

		if (token == -1) {
			BeginPicture(picture);
		} else {
			picture->usurped = cpicture;
			picture->set_token(-1);
			fOwner->fLink->StartMessage(AS_LAYER_APPEND_TO_PICTURE);
			fOwner->fLink->Attach<int32>(token);
		}
	}
}


BPicture *
BView::EndPicture()
{
	if (do_owner_check() && cpicture) {
		int32 token;

		fOwner->fLink->StartMessage(AS_LAYER_END_PICTURE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK
			&& fOwner->fLink->Read<int32>(&token) == B_OK) {
			BPicture *picture = cpicture;
			cpicture = picture->step_down();
			picture->set_token(token);

			return picture;
		}
	}

	return NULL;
}


void
BView::SetViewBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	_SetViewBitmap(bitmap, srcRect, dstRect, followFlags, options);
}


void
BView::SetViewBitmap(const BBitmap *bitmap, uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();

 	rect.OffsetTo(0, 0);

	_SetViewBitmap(bitmap, rect, rect, followFlags, options);
}


void
BView::ClearViewBitmap()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


status_t
BView::SetViewOverlay(const BBitmap *overlay, BRect srcRect, BRect dstRect,
	rgb_color *colorKey, uint32 followFlags, uint32 options)
{
	status_t err = _SetViewBitmap(overlay, srcRect, dstRect, followFlags,
		options | 0x4);

	// TODO: Incomplete?

	// read the color that will be treated as transparent
	fOwner->fLink->Read<rgb_color>(colorKey);

	return err;
}


status_t
BView::SetViewOverlay(const BBitmap *overlay, rgb_color *colorKey,
	uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (overlay)
		rect = overlay->Bounds();

 	rect.OffsetTo(0, 0);

	status_t err = _SetViewBitmap(overlay, rect, rect, followFlags,
			options | 0x4);

	// TODO: Incomplete?

	// read the color that will be treated as transparent
	fOwner->fLink->Read<rgb_color>(colorKey);

	return err;
}


void
BView::ClearViewOverlay()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


void
BView::CopyBits(BRect src, BRect dst)
{
	if (!src.IsValid() || !dst.IsValid())
		return;

	if (do_owner_check()) {
		fOwner->fLink->StartMessage(AS_LAYER_COPY_BITS);
		fOwner->fLink->Attach<BRect>(src);
		fOwner->fLink->Attach<BRect>(dst);

		_FlushIfNotInTransaction();
	}
}


void
BView::DrawPicture(const BPicture *picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
	Sync();
}


void
BView::DrawPicture(const BPicture *picture, BPoint where)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, where);
	Sync();
}


void
BView::DrawPicture(const char *filename, long offset, BPoint where)
{
	if (!filename)
		return;

	DrawPictureAsync(filename, offset, where);
	Sync();
}


void
BView::DrawPictureAsync(const BPicture *picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
}


void
BView::DrawPictureAsync(const BPicture *picture, BPoint where)
{
	if (picture == NULL)
		return;

	if (do_owner_check() && picture->token > 0) {
		fOwner->fLink->StartMessage(AS_LAYER_DRAW_PICTURE);
		fOwner->fLink->Attach<int32>(picture->token);
		fOwner->fLink->Attach<BPoint>(where);

		_FlushIfNotInTransaction();
	}
}


void
BView::DrawPictureAsync(const char *filename, long offset, BPoint where)
{
	if (!filename)
		return;

	// TODO: test and implement
}


void
BView::Invalidate(BRect invalRect)
{
	if (!invalRect.IsValid() || fOwner == NULL)
		return;

	check_lock();

	fOwner->fLink->StartMessage(AS_LAYER_INVALIDATE_RECT);
	fOwner->fLink->Attach<BRect>(invalRect);
	fOwner->fLink->Flush();
}


void
BView::Invalidate(const BRegion *invalRegion)
{
	if (invalRegion == NULL || fOwner == NULL)
		return;

	check_lock();

	int32 count = 0;
	count = const_cast<BRegion*>(invalRegion)->CountRects();

	fOwner->fLink->StartMessage(AS_LAYER_INVALIDATE_REGION);
	fOwner->fLink->Attach<int32>(count);

	for (int32 i = 0; i < count; i++)
		fOwner->fLink->Attach<BRect>( const_cast<BRegion *>(invalRegion)->RectAt(i));

	fOwner->fLink->Flush();
}


void
BView::Invalidate()
{
	Invalidate(Bounds());
}


void
BView::InvertRect(BRect rect)
{
	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_INVERT_RECT);
		fOwner->fLink->Attach<BRect>(rect);

		_FlushIfNotInTransaction();
	}
}


//	#pragma mark -
//	View Hierarchy Functions


void
BView::AddChild(BView *child, BView *before)
{
	STRACE(("BView(%s)::AddChild(child='%s' before='%s')\n",
 		this->Name() ? this->Name(): "NULL",
 		child && child->Name() ? child->Name(): "NULL",
 		before && before->Name() ? before->Name(): "NULL"));

	if (!child)
		return;

	if (child->fParent != NULL)
		debugger("AddChild failed - the view already has a parent.");

	bool lockedOwner = false;
	if (fOwner && !fOwner->IsLocked()) {
		fOwner->Lock();
		lockedOwner = true;
	}

	if (!_AddChildToList(child, before))
		debugger("AddChild failed!");	

	if (fOwner) {
		check_lock();

		child->_SetOwner(fOwner);
		child->_CreateSelf();
		child->_Attach();

		if (lockedOwner)
			fOwner->Unlock();
	}
}


bool
BView::RemoveChild(BView *child)
{
	STRACE(("BView(%s)::RemoveChild(%s)\n", Name(), child->Name()));

	if (!child)
		return false;

	return child->RemoveSelf();
}


int32
BView::CountChildren() const
{
	check_lock_no_pick();

	uint32 count = 0;
	BView *child = fFirstChild;

	while (child != NULL) {
		count++;
		child = child->fNextSibling;
	}

	return count;
}


BView *
BView::ChildAt(int32 index) const
{
	check_lock_no_pick();

	BView *child = fFirstChild;
	while (child != NULL && index-- > 0) {
		child = child->fNextSibling;
	}

	return child;
}


BView *
BView::NextSibling() const
{
	return fNextSibling;
}


BView *
BView::PreviousSibling() const
{
	return fPreviousSibling;	
}


bool
BView::RemoveSelf()
{
	STRACE(("BView(%s)::RemoveSelf()...\n", Name()));

	// Remove this child from its parent

	BWindow* owner = fOwner;
	check_lock_no_pick();

	if (owner != NULL) {
		_UpdateStateForRemove();
		_Detach();
	}

	if (!fParent || !fParent->_RemoveChildFromList(this))
		return false;

	if (owner != NULL && fTopLevelView) {
		// the top level view is deleted by the app_server automatically
		owner->fLink->StartMessage(AS_LAYER_DELETE);
		owner->fLink->Attach<int32>(_get_object_token_(fParent));
	}

	STRACE(("DONE: BView(%s)::RemoveSelf()\n", Name()));

	return true;
}


BView *
BView::Parent() const
{
	if (fParent && fParent->fTopLevelView)
		return NULL;

	return fParent;
}


BView *
BView::FindView(const char *name) const
{
	if (name == NULL)
		return NULL;

	if (Name() != NULL && !strcmp(Name(), name))
		return const_cast<BView *>(this);

	BView *child = fFirstChild;
	while (child != NULL) {
		BView *view = child->FindView(name);
		if (view != NULL)
			return view;

		child = child->fNextSibling; 
	}

	return NULL;
}


void
BView::MoveBy(float deltaX, float deltaY)
{
	MoveTo(fParentOffset.x + deltaX, fParentOffset.y + deltaY);
}


void
BView::MoveTo(BPoint where)
{
	MoveTo(where.x, where.y);
}


void
BView::MoveTo(float x, float y)
{
	if (x == fParentOffset.x && y == fParentOffset.y)
		return;

	// BeBook says we should do this. And it makes sense.
	x = roundf(x);
	y = roundf(y);

	if (fOwner) {
		check_lock();
		fOwner->fLink->StartMessage(AS_LAYER_MOVE_TO);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_FRAME_BIT;
	}

	_MoveTo(x, y);
}


void
BView::ResizeBy(float deltaWidth, float deltaHeight)
{
	// NOTE: I think this check makes sense, but I didn't
	// test what R5 does.
	if (fBounds.right + deltaWidth < 0)
		deltaWidth = -fBounds.right;
	if (fBounds.bottom + deltaHeight < 0)
		deltaHeight = -fBounds.bottom;

	if (deltaWidth == 0 && deltaHeight == 0)
		return;

	// BeBook says we should do this. And it makes sense.
	deltaWidth = roundf(deltaWidth);
	deltaHeight = roundf(deltaHeight);

	if (fOwner) {
		check_lock();
		fOwner->fLink->StartMessage(AS_LAYER_RESIZE_TO);

		fOwner->fLink->Attach<float>(fBounds.right + deltaWidth);
		fOwner->fLink->Attach<float>(fBounds.bottom + deltaHeight);

		fState->valid_flags |= B_VIEW_FRAME_BIT;
	}

	_ResizeBy(deltaWidth, deltaHeight);
}


void
BView::ResizeTo(float width, float height)
{
	ResizeBy(width - fBounds.Width(), height - fBounds.Height());
}


//	#pragma mark -
//	Inherited Methods (from BHandler)


status_t
BView::GetSupportedSuites(BMessage *data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("Suites", "suite/vnd.Be-view");
	if (status == B_OK) {
		BPropertyInfo propertyInfo(sViewPropInfo);

		status = data->AddFlat("message", &propertyInfo);
		if (status == B_OK)
			status = BHandler::GetSupportedSuites(data);
	}
	return status;
}


BHandler *
BView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 what,	const char *property)
{
	if (msg->what == B_WINDOW_MOVE_BY
		|| msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(sViewPropInfo);

	switch (propertyInfo.FindMatch(msg, index, specifier, what, property)) {
		case B_ERROR:
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
			return this;

		case 4:
			if (fShelf) {
				msg->PopSpecifier();
				return fShelf;
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);

				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message", "This window doesn't have a shelf");
				msg->SendReply(&replyMsg);
				return NULL;
			}
			break;

		case 6:
		case 7:
		case 8:
		{
			if (fFirstChild) {
				BView *child;
				switch (msg->what) {
					case B_INDEX_SPECIFIER:
					{
						int32 index;
						msg->FindInt32("data", &index);
						child = ChildAt(index);
						break;
					}
					case B_REVERSE_INDEX_SPECIFIER:
					{
						int32 rindex;
						msg->FindInt32("data", &rindex);
						child = ChildAt(CountChildren() - rindex);
						break;
					}
					case B_NAME_SPECIFIER:
					{
						const char *name;
						msg->FindString("data", &name);
						child = FindView(name);
						break;
					}

					default:
						child = NULL;
						break;
				}

				if (child != NULL) {
					msg->PopSpecifier();
					return child;
				} else {
					BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
					replyMsg.AddInt32("error", B_BAD_INDEX);
					replyMsg.AddString("message", "Cannot find view at/with specified index/name.");
					msg->SendReply(&replyMsg);
					return NULL;
				}
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message", "This window doesn't have children.");
				msg->SendReply(&replyMsg);
				return NULL;
			}
			break;
		}

		default:
			break;
	}

	return BHandler::ResolveSpecifier(msg, index, specifier, what, property);
}


void
BView::MessageReceived(BMessage *msg)
{ 
	BMessage specifier;
	int32 what;
	const char *prop;
	int32 index;
	status_t err;

	if (!msg->HasSpecifiers()) {
		if (msg->what == B_MOUSE_WHEEL_CHANGED) {
			float deltaX = 0.0f, deltaY = 0.0f;

			BScrollBar *horizontal = ScrollBar(B_HORIZONTAL);
			if (horizontal != NULL)
				msg->FindFloat("be:wheel_delta_x", &deltaX);

			BScrollBar *vertical = ScrollBar(B_VERTICAL);
			if (vertical != NULL)
				msg->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaX == 0.0f && deltaY == 0.0f)
				return;

			float smallStep, largeStep;
			if (horizontal != NULL) {
				horizontal->GetSteps(&smallStep, &largeStep);

				// pressing the option key scrolls faster
				if (modifiers() & B_OPTION_KEY)
					deltaX *= largeStep;
				else
					deltaX *= smallStep * 3;

				horizontal->SetValue(horizontal->Value() + deltaX);
			}

			if (vertical != NULL) {
				vertical->GetSteps(&smallStep, &largeStep);

				// pressing the option key scrolls faster
				if (modifiers() & B_OPTION_KEY)
					deltaY *= largeStep;
				else
					deltaY *= smallStep * 3;

				vertical->SetValue(vertical->Value() + deltaY);
			}
		} else
			BHandler::MessageReceived(msg);

		return;
	}

	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK) {
		BMessage replyMsg;

		switch (msg->what) {
			case B_GET_PROPERTY:
			{
				replyMsg.what = B_NO_ERROR;
				replyMsg.AddInt32("error", B_OK);

				if (strcmp(prop, "Frame") == 0)
					replyMsg.AddRect("result", Frame());
				else if (strcmp(prop, "Hidden") == 0)
					replyMsg.AddBool( "result", IsHidden());
				break;
			}

			case B_SET_PROPERTY:
			{
				if (strcmp(prop, "Frame") == 0) {
					BRect newFrame;
					if (msg->FindRect("data", &newFrame) == B_OK) {
						MoveTo(newFrame.LeftTop());
						ResizeTo(newFrame.right, newFrame.bottom);

						replyMsg.what = B_NO_ERROR;
						replyMsg.AddInt32("error", B_OK);
					} else {
						replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						replyMsg.AddString("message", "Didn't understand the specifier(s)");
					}
				} else if (strcmp(prop, "Hidden") == 0) {
					bool newHiddenState;
					if (msg->FindBool("data", &newHiddenState) == B_OK) {
						if (!IsHidden() && newHiddenState == true) {
							Hide();

							replyMsg.what = B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK);
						}
						else if (IsHidden() && newHiddenState == false) {
							Show();

							replyMsg.what = B_NO_ERROR;
							replyMsg.AddInt32("error", B_OK);
						} else {
							replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
							replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
							replyMsg.AddString("message", "Didn't understand the specifier(s)");
						}
					} else {
						replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						replyMsg.AddString("message", "Didn't understand the specifier(s)");
					}
				}
				break;
			}

			case B_COUNT_PROPERTIES:
				if (strcmp(prop, "View") == 0) {
					replyMsg.what = B_NO_ERROR;
					replyMsg.AddInt32("error", B_OK);
					replyMsg.AddInt32("result", CountChildren());
				}
				break;
		}

		msg->SendReply(&replyMsg);
	} else {
		BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32("error" , B_BAD_SCRIPT_SYNTAX);
		replyMsg.AddString("message", "Didn't understand the specifier(s)");

		msg->SendReply(&replyMsg);
	}
} 


status_t
BView::Perform(perform_code d, void* arg)
{
	return B_BAD_VALUE;
}


//	#pragma mark -
//	Private Functions


void
BView::_InitData(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
{
	// Info: The name of the view is set by BHandler constructor
	
	STRACE(("BView::InitData: enter\n"));
	
	// initialize members		
	fFlags = (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);

	// handle rounding
	frame.left = roundf(frame.left);
	frame.top = roundf(frame.top);
	frame.right = roundf(frame.right);
	frame.bottom = roundf(frame.bottom);

	fParentOffset.Set(frame.left, frame.top);

	fOwner = NULL;
	fParent = NULL;
	fNextSibling = NULL;
	fPreviousSibling = NULL;
	fFirstChild = NULL;

	fShowLevel = 0;
	fTopLevelView = false;

	cpicture = NULL;
	comm = NULL;

	fVerScroller = NULL;
	fHorScroller = NULL;

	f_is_printing = false;
	fAttached = false;

	fState = new BPrivate::ViewState;

	fBounds = frame.OffsetToCopy(B_ORIGIN);
	fShelf = NULL;

	fEventMask = 0;
	fEventOptions = 0;
}


void
BView::removeCommArray()
{
	if (comm) {
		delete [] comm->array;
		delete comm;
		comm = NULL;
	}
}


void
BView::_SetOwner(BWindow *newOwner)
{
	if (!newOwner)
		removeCommArray();

	if (fOwner != newOwner && fOwner) {
		if (fOwner->fFocus == this)
			MakeFocus(false);

		if (fOwner->fLastMouseMovedView == this)
			fOwner->fLastMouseMovedView = NULL;

		fOwner->RemoveHandler(this);
		if (fShelf)
			fOwner->RemoveHandler(fShelf);
	}

	if (newOwner && newOwner != fOwner) {
		newOwner->AddHandler(this);
		if (fShelf)
			newOwner->AddHandler(fShelf);

		if (fTopLevelView)
			SetNextHandler(newOwner);
		else
			SetNextHandler(fParent);
	}

	fOwner = newOwner;

	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling)
		child->_SetOwner(newOwner);
}


void
BView::DoPictureClip(BPicture *picture, BPoint where,
	bool invert, bool sync)
{
	if (!picture)
		return;

	if (do_owner_check()) {
		fOwner->fLink->StartMessage(AS_LAYER_CLIP_TO_PICTURE);
		fOwner->fLink->Attach<int32>(picture->token);
		fOwner->fLink->Attach<BPoint>(where);
		fOwner->fLink->Attach<bool>(invert);

		// TODO: I think that "sync" means another thing here:
		// the bebook, at least, says so.
		if (sync)
			fOwner->fLink->Flush();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
	}

	fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
}


bool
BView::_RemoveChildFromList(BView* child)
{
	if (child->fParent != this)
		return false;

	if (fFirstChild == child) {
		// it's the first view in the list
		fFirstChild = child->fNextSibling;
	} else {
		// there must be a previous sibling
		child->fPreviousSibling->fNextSibling = child->fNextSibling;
	}

	if (child->fNextSibling)
		child->fNextSibling->fPreviousSibling = child->fPreviousSibling;

	child->fParent = NULL;
	child->fNextSibling = NULL;
	child->fPreviousSibling = NULL;

	return true;
}


bool
BView::_AddChildToList(BView* child, BView* before)
{
	if (!child)
		return false;
	if (child->fParent != NULL) {
		debugger("View already belongs to someone else");
		return false;
	}
	if (before != NULL && before->fParent != this) {
		debugger("Invalid before view");
		return false;
	}

	if (before != NULL) {
		// add view before this one
		child->fNextSibling = before;
		child->fPreviousSibling = before->fPreviousSibling;
		if (child->fPreviousSibling != NULL)
			child->fPreviousSibling->fNextSibling = child;

		before->fPreviousSibling = child;
		if (fFirstChild == before)
			fFirstChild = child;
	} else {
		// add view to the end of the list
		BView *last = fFirstChild;
		while (last != NULL && last->fNextSibling != NULL) {
			last = last->fNextSibling;
		}

		if (last != NULL) {
			last->fNextSibling = child;
			child->fPreviousSibling = last;
		} else {
			fFirstChild = child;
			child->fPreviousSibling = NULL;
		}

		child->fNextSibling = NULL;
	}

	child->fParent = this;
	return true;
}


/*!	\brief Creates the server counterpart of this view.
	This is only done for views that are part of the view hierarchy, ie. when
	they are attached to a window.
	RemoveSelf() deletes the server object again.
*/
bool
BView::_CreateSelf()
{
	// AS_LAYER_CREATE & AS_LAYER_CREATE_ROOT do not use the
	// current view mechanism via check_lock() - the token
	// of the view and its parent are both send to the server.

	if (fTopLevelView)
		fOwner->fLink->StartMessage(AS_LAYER_CREATE_ROOT);
	else
 		fOwner->fLink->StartMessage(AS_LAYER_CREATE);

	fOwner->fLink->Attach<int32>(_get_object_token_(this));
	fOwner->fLink->AttachString(Name());
	fOwner->fLink->Attach<BRect>(Frame());
	fOwner->fLink->Attach<uint32>(ResizingMode());
	fOwner->fLink->Attach<uint32>(fEventMask);
	fOwner->fLink->Attach<uint32>(fEventOptions);
	fOwner->fLink->Attach<uint32>(Flags());
	fOwner->fLink->Attach<bool>(IsHidden(this));
	fOwner->fLink->Attach<rgb_color>(fState->view_color);
	if (fTopLevelView)
		fOwner->fLink->Attach<int32>(B_NULL_TOKEN);
	else
		fOwner->fLink->Attach<int32>(_get_object_token_(fParent));
	fOwner->fLink->Flush();

	do_owner_check();
	fState->UpdateServerState(*fOwner->fLink);

	// we create all its children, too

	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling) {
		child->_CreateSelf();
	}

	fOwner->fLink->Flush();
	return true;
}


/*!
	Sets the new view position.
	It doesn't contact the server, though - the only case where this
	is called outside of MoveTo() is as reaction of moving a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the BView's FrameMoved() hook.
*/
void
BView::_MoveTo(int32 x, int32 y)
{
	fParentOffset.Set(x, y);

	if (Window() != NULL && fFlags & B_FRAME_EVENTS) {
		// TODO: CurrentMessage() is not what it used to be!
		FrameMoved(BPoint(x, y));
	}
}


/*!
	Computes the actual new frame size and recalculates the size of
	the children as well.
	It doesn't contact the server, though - the only case where this
	is called outside of ResizeBy() is as reaction of resizing a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the BView's FrameResized() hook.
*/
void
BView::_ResizeBy(int32 deltaWidth, int32 deltaHeight)
{
	fBounds.right += deltaWidth;
	fBounds.bottom += deltaHeight;

	if (Window() == NULL) {
		// we're not supposed to exercise the resizing code in case
		// we haven't been attached to a window yet
		return;
	}

	// layout the children
	for (BView* child = fFirstChild; child; child = child->fNextSibling)
		child->_ParentResizedBy(deltaWidth, deltaHeight);

	if (fFlags & B_FRAME_EVENTS) {
		// TODO: CurrentMessage() is not what it used to be!
		FrameResized(fBounds.Width(), fBounds.Height());
	}
}


/*!
	Relayouts the view according to its resizing mode.
*/
void
BView::_ParentResizedBy(int32 x, int32 y)
{
	uint32 resizingMode = fFlags & _RESIZE_MASK_;
	BRect newFrame = Frame();

	// follow with left side
	if ((resizingMode & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((resizingMode & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x / 2;

	// follow with right side
	if ((resizingMode & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((resizingMode & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x / 2;

	// follow with top side
	if ((resizingMode & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((resizingMode & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y / 2;

	// follow with bottom side
	if ((resizingMode & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((resizingMode & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y / 2;

	if (newFrame.LeftTop() != fParentOffset) {
		// move view
		_MoveTo(roundf(newFrame.left), roundf(newFrame.top));
	}

	if (newFrame != Frame()) {
		// resize view
		int32 widthDiff = (int32)(newFrame.Width() - fBounds.Width());
		int32 heightDiff = (int32)(newFrame.Height() - fBounds.Height());
		_ResizeBy(widthDiff, heightDiff);
	}
}


void
BView::_Activate(bool active)
{
	WindowActivated(active);

	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling) {
		child->_Activate(active);
	}
}


void
BView::_Attach()
{
	AttachedToWindow();
	fAttached = true;

	// after giving the view a chance to do this itself,
	// check for the B_PULSE_NEEDED flag and make sure the
	// window set's up the pulse messaging
	if (fOwner) {
		if (fFlags & B_PULSE_NEEDED) {
			check_lock_no_pick();
			if (!fOwner->fPulseEnabled)
				fOwner->SetPulseRate(500000);
		}
	}

	for (BView* child = fFirstChild; child != NULL; child = child->fNextSibling) {
		// we need to check for fAttached as new views could have been
		// added in AttachedToWindow() - and those are already attached
		if (!child->fAttached)
			child->_Attach();
	}

	AllAttached();
}


void
BView::_Detach()
{
	DetachedFromWindow();
	fAttached = false;

	for (BView* child = fFirstChild; child != NULL; child = child->fNextSibling) {
		child->_Detach();
	}

	AllDetached();

	if (fOwner) {
		check_lock();

		// make sure our owner doesn't need us anymore

		if (fOwner->CurrentFocus() == this)
			MakeFocus(false);

		if (fOwner->fDefaultButton == this)
			fOwner->SetDefaultButton(NULL);

		if (fOwner->fKeyMenuBar == this)
			fOwner->fKeyMenuBar = NULL;

		if (fOwner->fLastMouseMovedView == this)
			fOwner->fLastMouseMovedView = NULL;

		if (fOwner->fLastViewToken == _get_object_token_(this))
			fOwner->fLastViewToken = B_NULL_TOKEN;

		_SetOwner(NULL);
	}
}


void
BView::_Draw(BRect updateRectScreen)
{
	if (IsHidden(this))
		return;

	check_lock();

	ConvertFromScreen(&updateRectScreen);
	BRect updateRect = Bounds() & updateRectScreen;

	if (Flags() & B_WILL_DRAW) {
		// TODO: make states robust
		PushState();
		Draw(updateRect);
		PopState();
		Flush();
//	} else {
		// ViewColor() == B_TRANSPARENT_COLOR and no B_WILL_DRAW
		// -> View is simply not drawn at all
	}

//	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling) {
//		BRect rect = child->Frame();
//		if (!updateRect.Intersects(rect))
//			continue;
//
//		// get new update rect in child coordinates
//		rect = updateRect & rect;
//		child->ConvertFromParent(&rect);
//
//		child->_Draw(rect);
//	}
//
//	if (Flags() & B_DRAW_ON_CHILDREN) {
//		// TODO: Since we have hard clipping in the app_server,
//		// a view can never draw "on top of it's child views" as
//		// the BeBook describes.
//		// (TODO: Test if this is really possible in BeOS.)
//		PushState();
//		DrawAfterChildren(updateRect);
//		PopState();
//		Flush();
//	}
}


void
BView::_Pulse()
{
	if (Flags() & B_PULSE_NEEDED)
		Pulse();

	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling) {
		child->_Pulse();
	}
}


void
BView::_UpdateStateForRemove()
{
	if (!do_owner_check())
		return;

	fState->UpdateFrom(*fOwner->fLink);
	if (!fState->IsValid(B_VIEW_FRAME_BIT)) {
		fOwner->fLink->StartMessage(AS_LAYER_GET_COORD);

		status_t code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<BPoint>(&fParentOffset);
			fOwner->fLink->Read<BRect>(&fBounds);
			fState->valid_flags |= B_VIEW_FRAME_BIT;
		}
	}
	
	// update children as well

	for (BView *child = fFirstChild; child != NULL; child = child->fNextSibling) {
		if (child->fOwner)
			child->_UpdateStateForRemove();
	}
}


inline void
BView::_UpdatePattern(::pattern pattern)
{
	if (fState->IsValid(B_VIEW_PATTERN_BIT) && pattern == fState->pattern)
		return;

	if (fOwner) {
		check_lock();

		fOwner->fLink->StartMessage(AS_LAYER_SET_PATTERN);
		fOwner->fLink->Attach< ::pattern>(pattern);

		fState->valid_flags |= B_VIEW_PATTERN_BIT;
	}

	fState->pattern = pattern;
}


void
BView::_FlushIfNotInTransaction()
{
	if (!fOwner->fInTransaction) {
		fOwner->Flush();
	}
}


BShelf *
BView::_Shelf() const
{
	return fShelf;
}


void
BView::_SetShelf(BShelf *shelf)
{
	if (fShelf != NULL && fOwner != NULL)
		fOwner->RemoveHandler(fShelf);

	fShelf = shelf;

	if (fShelf != NULL && fOwner != NULL)
		fOwner->AddHandler(fShelf);
}


status_t
BView::_SetViewBitmap(const BBitmap* bitmap, BRect srcRect,
	BRect dstRect, uint32 followFlags, uint32 options)
{
	if (!do_owner_check())
		return B_ERROR;

	int32 serverToken = bitmap ? bitmap->get_server_token() : -1;

	fOwner->fLink->StartMessage(AS_LAYER_SET_VIEW_BITMAP);
	fOwner->fLink->Attach<int32>(serverToken);
	fOwner->fLink->Attach<BRect>(srcRect);
	fOwner->fLink->Attach<BRect>(dstRect);
	fOwner->fLink->Attach<int32>(followFlags);
	fOwner->fLink->Attach<int32>(options);

	status_t status = B_ERROR;
	fOwner->fLink->FlushWithReply(status);

	return status;
}
 

bool
BView::do_owner_check() const
{
	STRACE(("BView(%s)::do_owner_check()...", Name()));

	int32 serverToken = _get_object_token_(this);

	if (fOwner == NULL) {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	fOwner->check_lock();

	if (fOwner->fLastViewToken != serverToken) {
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		fOwner->fLink->StartMessage(AS_SET_CURRENT_LAYER);
		fOwner->fLink->Attach<int32>(serverToken);

		fOwner->fLastViewToken = serverToken;
	} else
		STRACE(("this is the lastViewToken\n"));

	return true;
}


void
BView::check_lock() const
{
	STRACE(("BView(%s)::check_lock()...", Name() ? Name(): "NULL"));

	if (!fOwner)
		return;

	fOwner->check_lock();

	int32 serverToken = _get_object_token_(this);

	if (fOwner->fLastViewToken != serverToken) {
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		fOwner->fLink->StartMessage(AS_SET_CURRENT_LAYER);
		fOwner->fLink->Attach<int32>(serverToken);

		fOwner->fLastViewToken = serverToken;
	} else {
		STRACE(("quiet2\n"));
	}
}


void
BView::check_lock_no_pick() const
{
	if (fOwner)
		fOwner->check_lock();
}


bool
BView::do_owner_check_no_pick() const
{
	if (fOwner) {
		fOwner->check_lock();
		return true;
	} else {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}


void BView::_ReservedView2(){}
void BView::_ReservedView3(){}
void BView::_ReservedView4(){}
void BView::_ReservedView5(){}
void BView::_ReservedView6(){}
void BView::_ReservedView7(){}
void BView::_ReservedView8(){}
void BView::_ReservedView9(){}
void BView::_ReservedView10(){}
void BView::_ReservedView11(){}
void BView::_ReservedView12(){}
void BView::_ReservedView13(){}
void BView::_ReservedView14(){}
void BView::_ReservedView15(){}
void BView::_ReservedView16(){}


BView::BView(const BView &other)
	: BHandler()
{
	// this is private and not functional, but exported
}


BView &
BView::operator=(const BView &other)
{
	// this is private and not functional, but exported
	return *this;
}


void
BView::PrintToStream()
{
	printf("BView::PrintToStream()\n");
	printf("\tName: %s\
\tParent: %s\
\tFirstChild: %s\
\tNextSibling: %s\
\tPrevSibling: %s\
\tOwner(Window): %s\
\tToken: %ld\
\tFlags: %ld\
\tView origin: (%f,%f)\
\tView Bounds rectangle: (%f,%f,%f,%f)\
\tShow level: %d\
\tTopView?: %s\
\tBPicture: %s\
\tVertical Scrollbar %s\
\tHorizontal Scrollbar %s\
\tIs Printing?: %s\
\tShelf?: %s\
\tEventMask: %ld\
\tEventOptions: %ld\n",
	Name(),
	fParent ? fParent->Name() : "NULL",
	fFirstChild ? fFirstChild->Name() : "NULL",
	fNextSibling ? fNextSibling->Name() : "NULL",
	fPreviousSibling ? fPreviousSibling->Name() : "NULL",
	fOwner ? fOwner->Name() : "NULL",
	_get_object_token_(this),
	fFlags,
	fParentOffset.x, fParentOffset.y,
	fBounds.left, fBounds.top, fBounds.right, fBounds.bottom,
	fShowLevel,
	fTopLevelView ? "YES" : "NO",
	cpicture? "YES" : "NULL",
	fVerScroller? "YES" : "NULL",
	fHorScroller? "YES" : "NULL",
	f_is_printing? "YES" : "NO",
	fShelf? "YES" : "NO",
	fEventMask,
	fEventOptions);
	
	printf("\tState status:\
\t\tLocalCoordianteSystem: (%f,%f)\
\t\tPenLocation: (%f,%f)\
\t\tPenSize: %f\
\t\tHighColor: [%d,%d,%d,%d]\
\t\tLowColor: [%d,%d,%d,%d]\
\t\tViewColor: [%d,%d,%d,%d]\
\t\tPattern: %llx\
\t\tDrawingMode: %d\
\t\tLineJoinMode: %d\
\t\tLineCapMode: %d\
\t\tMiterLimit: %f\
\t\tAlphaSource: %d\
\t\tAlphaFuntion: %d\
\t\tScale: %f\
\t\t(Print)FontAliasing: %s\
\t\tFont Info:\n",
	fState->origin.x, fState->origin.y,
	fState->pen_location.x, fState->pen_location.y,
	fState->pen_size,
	fState->high_color.red, fState->high_color.blue, fState->high_color.green, fState->high_color.alpha,
	fState->low_color.red, fState->low_color.blue, fState->low_color.green, fState->low_color.alpha,
	fState->view_color.red, fState->view_color.blue, fState->view_color.green, fState->view_color.alpha,
	*((uint64*)&(fState->pattern)),
	fState->drawing_mode,
	fState->line_join,
	fState->line_cap,
	fState->miter_limit,
	fState->alpha_source_mode,
	fState->alpha_function_mode,
	fState->scale,
	fState->font_aliasing? "YES" : "NO");

	fState->font.PrintToStream();
	
	// TODO: also print the line array.
}


void
BView::PrintTree()
{
	int32 spaces = 2;
	BView *c = fFirstChild; //c = short for: current
	printf( "'%s'\n", Name() );
	if (c != NULL) {
		while(true) {
			// action block
			{
				for (int i = 0; i < spaces; i++)
					printf(" ");

				printf( "'%s'\n", c->Name() );
			}

			// go deep
			if (c->fFirstChild) {
				c = c->fFirstChild;
				spaces += 2;
			} else {
				// go right
				if (c->fNextSibling) {
					c = c->fNextSibling;
				} else {
					// go up
					while (!c->fParent->fNextSibling && c->fParent != this) {
						c = c->fParent;
						spaces -= 2;
					}

					// that enough! We've reached this view.
					if (c->fParent == this)
						break;

					c = c->fParent->fNextSibling;
					spaces -= 2;
				}
			}
		}
	}
}


/* TODO:
 	-implement SetDiskMode(). What's with this method? What does it do? test!
 		does it has something to do with DrawPictureAsync( filename* .. )?
	-implement DrawAfterChildren()
*/
