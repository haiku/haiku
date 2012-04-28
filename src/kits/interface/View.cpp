/*
 * Copyright 2001-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <View.h>

#include <new>

#include <math.h>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Cursor.h>
#include <File.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <InterfaceDefs.h>
#include <Layout.h>
#include <LayoutContext.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <Message.h>
#include <MessageQueue.h>
#include <ObjectList.h>
#include <Picture.h>
#include <Point.h>
#include <Polygon.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <Shelf.h>
#include <String.h>
#include <Window.h>

#include <AppMisc.h>
#include <AppServerLink.h>
#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <ServerProtocolStructs.h>
#include <ShapePrivate.h>
#include <ToolTip.h>
#include <ToolTipManager.h>
#include <TokenSpace.h>
#include <ViewPrivate.h>

using std::nothrow;

//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#	define BVTRACE _PrintToStream()
#else
#	define STRACE(x) ;
#	define BVTRACE ;
#endif


static property_info sViewPropInfo[] = {
	{ "Frame", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER, 0 }, "The view's frame rectangle.", 0,
		{ B_RECT_TYPE }
	},
	{ "Hidden", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER, 0 }, "Whether or not the view is hidden.",
		0, { B_BOOL_TYPE }
	},
	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the "
			"shelf.", 0
	},
	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of child views.", 0,
		{ B_INT32_TYPE }
	},
	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER, 0 },
		"Directs the scripting message to the specified view.", 0
	},

	{ 0, { 0 }, { 0 }, 0, 0 }
};


//	#pragma mark -


static inline uint32
get_uint32_color(rgb_color color)
{
	return B_BENDIAN_TO_HOST_INT32(*(uint32*)&color);
		// rgb_color is always in rgba format, no matter what endian;
		// we always return the int32 value in host endian.
}


static inline rgb_color
get_rgb_color(uint32 value)
{
	value = B_HOST_TO_BENDIAN_INT32(value);
	return *(rgb_color*)&value;
}


//	#pragma mark -


namespace BPrivate {

ViewState::ViewState()
{
	pen_location.Set(0, 0);
	pen_size = 1.0;

	// NOTE: the clipping_region is empty
	// on construction but it is not used yet,
	// we avoid having to keep track of it via
	// this flag
	clipping_region_used = false;

	high_color = (rgb_color){ 0, 0, 0, 255 };
	low_color = (rgb_color){ 255, 255, 255, 255 };
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

	// We only keep the B_VIEW_CLIP_REGION_BIT flag invalidated,
	// because we should get the clipping region from app_server.
	// The other flags do not need to be included because the data they
	// represent is already in sync with app_server - app_server uses the
	// same init (default) values.
	valid_flags = ~B_VIEW_CLIP_REGION_BIT;

	archiving_flags = B_VIEW_FRAME_BIT | B_VIEW_RESIZE_BIT;
}


void
ViewState::UpdateServerFontState(BPrivate::PortLink &link)
{
	link.StartMessage(AS_VIEW_SET_FONT_STATE);
	link.Attach<uint16>(font_flags);
		// always present

	if (font_flags & B_FONT_FAMILY_AND_STYLE)
		link.Attach<uint32>(font.FamilyAndStyle());

	if (font_flags & B_FONT_SIZE)
		link.Attach<float>(font.Size());

	if (font_flags & B_FONT_SHEAR)
		link.Attach<float>(font.Shear());

	if (font_flags & B_FONT_ROTATION)
		link.Attach<float>(font.Rotation());

	if (font_flags & B_FONT_FALSE_BOLD_WIDTH)
		link.Attach<float>(font.FalseBoldWidth());

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

	link.StartMessage(AS_VIEW_SET_STATE);

	ViewSetStateInfo info;
	info.penLocation = pen_location;
	info.penSize = pen_size;
	info.highColor = high_color;
	info.lowColor = low_color;
	info.pattern = pattern;
	info.drawingMode = drawing_mode;
	info.origin = origin;
	info.scale = scale;
	info.lineJoin = line_join;
	info.lineCap = line_cap;
	info.miterLimit = miter_limit;
	info.alphaSourceMode = alpha_source_mode;
	info.alphaFunctionMode = alpha_function_mode;
	info.fontAntialiasing = font_aliasing;
	link.Attach<ViewSetStateInfo>(info);

	// we send the 'local' clipping region... if we have one...
	// TODO: Could be optimized, but is low prio, since most views won't
	// have a custom clipping region.
	if (clipping_region_used) {
		int32 count = clipping_region.CountRects();
		link.Attach<int32>(count);
		for (int32 i = 0; i < count; i++)
			link.Attach<BRect>(clipping_region.RectAt(i));
	} else {
		// no clipping region
		link.Attach<int32>(-1);
	}

	// Although we might have a 'local' clipping region, when we call
	// BView::GetClippingRegion() we ask for the 'global' one and it
	// is kept on server, so we must invalidate B_VIEW_CLIP_REGION_BIT flag

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}


void
ViewState::UpdateFrom(BPrivate::PortLink &link)
{
	link.StartMessage(AS_VIEW_GET_STATE);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	ViewGetStateInfo info;
	link.Read<ViewGetStateInfo>(&info);

	// set view's font state
	font_flags = B_FONT_ALL;
	font.SetFamilyAndStyle(info.fontID);
	font.SetSize(info.fontSize);
	font.SetShear(info.fontShear);
	font.SetRotation(info.fontRotation);
	font.SetFalseBoldWidth(info.fontFalseBoldWidth);
	font.SetSpacing(info.fontSpacing);
	font.SetEncoding(info.fontEncoding);
	font.SetFace(info.fontFace);
	font.SetFlags(info.fontFlags);

	// set view's state
	pen_location = info.viewStateInfo.penLocation;
	pen_size = info.viewStateInfo.penSize;
	high_color = info.viewStateInfo.highColor;
	low_color = info.viewStateInfo.lowColor;
	pattern = info.viewStateInfo.pattern;
	drawing_mode = info.viewStateInfo.drawingMode;
	origin = info.viewStateInfo.origin;
	scale = info.viewStateInfo.scale;
	line_join = info.viewStateInfo.lineJoin;
	line_cap = info.viewStateInfo.lineCap;
	miter_limit = info.viewStateInfo.miterLimit;
	alpha_source_mode = info.viewStateInfo.alphaSourceMode;
	alpha_function_mode = info.viewStateInfo.alphaFunctionMode;
	font_aliasing = info.viewStateInfo.fontAntialiasing;

	// read the user clipping
	// (that's NOT the current View visible clipping but the additional
	// user specified clipping!)
	int32 clippingRectCount;
	link.Read<int32>(&clippingRectCount);
	if (clippingRectCount >= 0) {
		clipping_region.MakeEmpty();
		for (int32 i = 0; i < clippingRectCount; i++) {
			BRect rect;
			link.Read<BRect>(&rect);
			clipping_region.Include(rect);
		}
	} else {
		// no user clipping used
		clipping_region_used = false;
	}

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}

}	// namespace BPrivate


//	#pragma mark -


// archiving constants
namespace {
	const char* const kSizesField = "BView:sizes";
		// kSizesField = {min, max, pref}
	const char* const kAlignmentField = "BView:alignment";
	const char* const kLayoutField = "BView:layout";
}


struct BView::LayoutData {
	LayoutData()
		:
		fMinSize(),
		fMaxSize(),
		fPreferredSize(),
		fAlignment(),
		fLayoutInvalidationDisabled(0),
		fLayout(NULL),
		fLayoutContext(NULL),
		fLayoutItems(5, false),
		fLayoutValid(true),		// TODO: Rethink these initial values!
		fMinMaxValid(true),		//
		fLayoutInProgress(false),
		fNeedsRelayout(true)
	{
	}

	status_t
	AddDataToArchive(BMessage* archive)
	{
		status_t err = archive->AddSize(kSizesField, fMinSize);

		if (err == B_OK)
			err = archive->AddSize(kSizesField, fMaxSize);

		if (err == B_OK)
			err = archive->AddSize(kSizesField, fPreferredSize);

		if (err == B_OK)
			err = archive->AddAlignment(kAlignmentField, fAlignment);

		return err;
	}

	void
	PopulateFromArchive(BMessage* archive)
	{
		archive->FindSize(kSizesField, 0, &fMinSize);
		archive->FindSize(kSizesField, 1, &fMaxSize);
		archive->FindSize(kSizesField, 2, &fPreferredSize);
		archive->FindAlignment(kAlignmentField, &fAlignment);
	}

	BSize			fMinSize;
	BSize			fMaxSize;
	BSize			fPreferredSize;
	BAlignment		fAlignment;
	int				fLayoutInvalidationDisabled;
	BLayout*		fLayout;
	BLayoutContext*	fLayoutContext;
	BObjectList<BLayoutItem> fLayoutItems;
	bool			fLayoutValid;
	bool			fMinMaxValid;
	bool			fLayoutInProgress;
	bool			fNeedsRelayout;
};


BView::BView(const char* name, uint32 flags, BLayout* layout)
	:
	BHandler(name)
{
	_InitData(BRect(0, 0, 0, 0), name, B_FOLLOW_NONE,
		flags | B_SUPPORTS_LAYOUT);
	SetLayout(layout);
}


BView::BView(BRect frame, const char* name, uint32 resizingMode, uint32 flags)
	:
	BHandler(name)
{
	_InitData(frame, name, resizingMode, flags);
}


BView::BView(BMessage* archive)
	:
	BHandler(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver unarchiver(archive);
	if (!archive)
		debugger("BView cannot be constructed from a NULL archive.");

	BRect frame;
	archive->FindRect("_frame", &frame);

	uint32 resizingMode;
	if (archive->FindInt32("_resize_mode", (int32*)&resizingMode) != B_OK)
		resizingMode = 0;

	uint32 flags;
	if (archive->FindInt32("_flags", (int32*)&flags) != B_OK)
		flags = 0;

	_InitData(frame, Name(), resizingMode, flags);

	font_family family;
	font_style style;
	if (archive->FindString("_fname", 0, (const char**)&family) == B_OK
		&& archive->FindString("_fname", 1, (const char**)&style) == B_OK) {
		BFont font;
		font.SetFamilyAndStyle(family, style);

		float size;
		if (archive->FindFloat("_fflt", 0, &size) == B_OK)
			font.SetSize(size);

		float shear;
		if (archive->FindFloat("_fflt", 1, &shear) == B_OK
			&& shear >= 45.0 && shear <= 135.0)
			font.SetShear(shear);

		float rotation;
		if (archive->FindFloat("_fflt", 2, &rotation) == B_OK
			&& rotation >=0 && rotation <= 360)
			font.SetRotation(rotation);

		SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
			| B_FONT_SHEAR | B_FONT_ROTATION);
	}

	int32 color;
	if (archive->FindInt32("_color", 0, &color) == B_OK)
		SetHighColor(get_rgb_color(color));
	if (archive->FindInt32("_color", 1, &color) == B_OK)
		SetLowColor(get_rgb_color(color));
	if (archive->FindInt32("_color", 2, &color) == B_OK)
		SetViewColor(get_rgb_color(color));

	uint32 evMask;
	uint32 options;
	if (archive->FindInt32("_evmask", 0, (int32*)&evMask) == B_OK
		&& archive->FindInt32("_evmask", 1, (int32*)&options) == B_OK)
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
	if (archive->FindInt32("_dmod", (int32*)&drawingMode) == B_OK)
		SetDrawingMode((drawing_mode)drawingMode);

	fLayoutData->PopulateFromArchive(archive);

	if (archive->FindInt16("_show", &fShowLevel) != B_OK)
		fShowLevel = 0;

	if (BUnarchiver::IsArchiveManaged(archive)) {
		int32 i = 0;
		while (unarchiver.EnsureUnarchived("_views", i++) == B_OK)
				;
		unarchiver.EnsureUnarchived(kLayoutField);

	} else {
		BMessage msg;
		for (int32 i = 0; archive->FindMessage("_views", i, &msg) == B_OK;
			i++) {
			BArchivable* object = instantiate_object(&msg);
			if (BView* child = dynamic_cast<BView*>(object))
				AddChild(child);
		}
	}
}


BArchivable*
BView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data , "BView"))
		return NULL;

	return new(std::nothrow) BView(data);
}


status_t
BView::Archive(BMessage* data, bool deep) const
{
	BArchiver archiver(data);
	status_t ret = BHandler::Archive(data, deep);

	if (ret != B_OK)
		return ret;

	if ((fState->archiving_flags & B_VIEW_FRAME_BIT) != 0)
		ret = data->AddRect("_frame", Bounds().OffsetToCopy(fParentOffset));

	if (ret == B_OK)
		ret = data->AddInt32("_resize_mode", ResizingMode());

	if (ret == B_OK)
		ret = data->AddInt32("_flags", Flags());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_EVENT_MASK_BIT) != 0) {
		ret = data->AddInt32("_evmask", fEventMask);
		if (ret == B_OK)
			ret = data->AddInt32("_evmask", fEventOptions);
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_FONT_BIT) != 0) {
		BFont font;
		GetFont(&font);

		font_family family;
		font_style style;
		font.GetFamilyAndStyle(&family, &style);
		ret = data->AddString("_fname", family);
		if (ret == B_OK)
			ret = data->AddString("_fname", style);
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Size());
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Shear());
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Rotation());
	}

	// colors
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(HighColor()));
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(LowColor()));
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(ViewColor()));

//	NOTE: we do not use this flag any more
//	if ( 1 ){
//		ret = data->AddInt32("_dbuf", 1);
//	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_ORIGIN_BIT) != 0)
		ret = data->AddPoint("_origin", Origin());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_PEN_SIZE_BIT) != 0)
		ret = data->AddFloat("_psize", PenSize());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_PEN_LOCATION_BIT) != 0)
		ret = data->AddPoint("_ploc", PenLocation());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_LINE_MODES_BIT) != 0) {
		ret = data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		if (ret == B_OK)
			ret = data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		if (ret == B_OK)
			ret = data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_BLENDING_BIT) != 0) {
		source_alpha alphaSourceMode;
		alpha_function alphaFunctionMode;
		GetBlendingMode(&alphaSourceMode, &alphaFunctionMode);

		ret = data->AddInt16("_blend", (int16)alphaSourceMode);
		if (ret == B_OK)
			ret = data->AddInt16("_blend", (int16)alphaFunctionMode);
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_DRAWING_MODE_BIT) != 0)
		ret = data->AddInt32("_dmod", DrawingMode());

	if (ret == B_OK)
		ret = fLayoutData->AddDataToArchive(data);

	if (ret == B_OK)
		ret = data->AddInt16("_show", fShowLevel);

	if (deep && ret == B_OK) {
		for (BView* child = fFirstChild; child != NULL && ret == B_OK;
			child = child->fNextSibling)
			ret = archiver.AddArchivable("_views", child, deep);

		if (ret == B_OK)
			ret = archiver.AddArchivable(kLayoutField, GetLayout(), deep);
	}

	return archiver.Finish(ret);
}


status_t
BView::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);
	status_t err = B_OK;

	int32 count;
	from->GetInfo("_views", NULL, &count);

	for (int32 i = 0; err == B_OK && i < count; i++) {
		BView* child;
		err = unarchiver.FindObject<BView>("_views", i, child);
		if (err == B_OK)
			err = _AddChild(child, NULL) ? B_OK : B_ERROR;
	}

	if (err == B_OK) {
		BLayout*& layout = fLayoutData->fLayout;
		err = unarchiver.FindObject(kLayoutField, layout);
		if (err == B_OK && layout) {
			fFlags |= B_SUPPORTS_LAYOUT;
			fLayoutData->fLayout->SetOwner(this);
		}
	}

	return err;
}


status_t
BView::AllArchived(BMessage* into) const
{
	return BHandler::AllArchived(into);
}


BView::~BView()
{
	STRACE(("BView(%s)::~BView()\n", this->Name()));

	if (fOwner) {
		debugger("Trying to delete a view that belongs to a window. "
			"Call RemoveSelf first.");
	}

	RemoveSelf();

	if (fToolTip != NULL)
		fToolTip->ReleaseReference();

	// TODO: see about BShelf! must I delete it here? is it deleted by
	// the window?

	// we also delete all our children

	BView* child = fFirstChild;
	while (child) {
		BView* nextChild = child->fNextSibling;

		delete child;
		child = nextChild;
	}

	// delete the layout and the layout data
	delete fLayoutData->fLayout;
	delete fLayoutData;

	if (fVerScroller)
		fVerScroller->SetTarget((BView*)NULL);
	if (fHorScroller)
		fHorScroller->SetTarget((BView*)NULL);

	SetName(NULL);

	_RemoveCommArray();
	delete fState;
}


BRect
BView::Bounds() const
{
	_CheckLock();

	if (fIsPrinting)
		return fState->print_rect;

	return fBounds;
}


void
BView::_ConvertToParent(BPoint* point, bool checkLock) const
{
	if (!fParent)
		return;

	if (checkLock)
		_CheckLock();

	// - our scrolling offset
	// + our bounds location within the parent
	point->x += -fBounds.left + fParentOffset.x;
	point->y += -fBounds.top + fParentOffset.y;
}


void
BView::ConvertToParent(BPoint* point) const
{
	_ConvertToParent(point, true);
}


BPoint
BView::ConvertToParent(BPoint point) const
{
	ConvertToParent(&point);

	return point;
}


void
BView::_ConvertFromParent(BPoint* point, bool checkLock) const
{
	if (!fParent)
		return;

	if (checkLock)
		_CheckLock();

	// - our bounds location within the parent
	// + our scrolling offset
	point->x += -fParentOffset.x + fBounds.left;
	point->y += -fParentOffset.y + fBounds.top;
}


void
BView::ConvertFromParent(BPoint* point) const
{
	_ConvertFromParent(point, true);
}


BPoint
BView::ConvertFromParent(BPoint point) const
{
	ConvertFromParent(&point);

	return point;
}


void
BView::ConvertToParent(BRect* rect) const
{
	if (!fParent)
		return;

	_CheckLock();

	// - our scrolling offset
	// + our bounds location within the parent
	rect->OffsetBy(-fBounds.left + fParentOffset.x,
		-fBounds.top + fParentOffset.y);
}


BRect
BView::ConvertToParent(BRect rect) const
{
	ConvertToParent(&rect);

	return rect;
}


void
BView::ConvertFromParent(BRect* rect) const
{
	if (!fParent)
		return;

	_CheckLock();

	// - our bounds location within the parent
	// + our scrolling offset
	rect->OffsetBy(-fParentOffset.x + fBounds.left,
		-fParentOffset.y + fBounds.top);
}


BRect
BView::ConvertFromParent(BRect rect) const
{
	ConvertFromParent(&rect);

	return rect;
}


void
BView::_ConvertToScreen(BPoint* pt, bool checkLock) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertToScreen(pt);

		return;
	}

	if (checkLock)
		_CheckOwnerLock();

	_ConvertToParent(pt, false);
	fParent->_ConvertToScreen(pt, false);
}


void
BView::ConvertToScreen(BPoint* pt) const
{
	_ConvertToScreen(pt, true);
}


BPoint
BView::ConvertToScreen(BPoint pt) const
{
	ConvertToScreen(&pt);

	return pt;
}


void
BView::_ConvertFromScreen(BPoint* pt, bool checkLock) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertFromScreen(pt);

		return;
	}

	if (checkLock)
		_CheckOwnerLock();

	_ConvertFromParent(pt, false);
	fParent->_ConvertFromScreen(pt, false);
}


void
BView::ConvertFromScreen(BPoint* pt) const
{
	_ConvertFromScreen(pt, true);
}


BPoint
BView::ConvertFromScreen(BPoint pt) const
{
	ConvertFromScreen(&pt);

	return pt;
}


void
BView::ConvertToScreen(BRect* rect) const
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
BView::ConvertFromScreen(BRect* rect) const
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
	_CheckLock();
	return fFlags & ~_RESIZE_MASK_;
}


void
BView::SetFlags(uint32 flags)
{
	if (Flags() == flags)
		return;

	if (fOwner) {
		if (flags & B_PULSE_NEEDED) {
			_CheckLock();
			if (fOwner->fPulseRunner == NULL)
				fOwner->SetPulseRate(fOwner->PulseRate());
		}

		uint32 changesFlags = flags ^ fFlags;
		if (changesFlags & (B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
				| B_FRAME_EVENTS | B_SUBPIXEL_PRECISE)) {
			_CheckLockAndSwitchCurrent();

			fOwner->fLink->StartMessage(AS_VIEW_SET_FLAGS);
			fOwner->fLink->Attach<uint32>(flags);
			fOwner->fLink->Flush();
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
	return Bounds().OffsetToCopy(fParentOffset.x, fParentOffset.y);
}


void
BView::Hide()
{
	if (fOwner && fShowLevel == 0) {
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_HIDE);
		fOwner->fLink->Flush();
	}
	fShowLevel++;

	if (fShowLevel == 1)
		_InvalidateParentLayout();
}


void
BView::Show()
{
	fShowLevel--;
	if (fOwner && fShowLevel == 0) {
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_SHOW);
		fOwner->fLink->Flush();
	}

	if (fShowLevel == 0)
		_InvalidateParentLayout();
}


bool
BView::IsFocus() const
{
	if (fOwner) {
		_CheckLock();
		return fOwner->CurrentFocus() == this;
	} else
		return false;
}


bool
BView::IsHidden(const BView* lookingFrom) const
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
	return fIsPrinting;
}


BPoint
BView::LeftTop() const
{
	return Bounds().LeftTop();
}


void
BView::SetResizingMode(uint32 mode)
{
	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_RESIZE_MODE);
		fOwner->fLink->Attach<uint32>(mode);
	}

	// look at SetFlags() for more info on the below line
	fFlags = (fFlags & ~_RESIZE_MASK_) | (mode & _RESIZE_MASK_);
}


uint32
BView::ResizingMode() const
{
	return fFlags & _RESIZE_MASK_;
}


void
BView::SetViewCursor(const BCursor* cursor, bool sync)
{
	if (cursor == NULL || fOwner == NULL)
		return;

	_CheckLock();

	ViewSetViewCursorInfo info;
	info.cursorToken = cursor->fServerToken;
	info.viewToken = _get_object_token_(this);
	info.sync = sync;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_VIEW_CURSOR);
	link.Attach<ViewSetViewCursorInfo>(info);

	if (sync) {
		// Make sure the server has processed the message.
		int32 code;
		link.FlushWithReply(code);
	}
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
	_CheckOwnerLock();
	if (fOwner)
		fOwner->Sync();
}


BWindow*
BView::Window() const
{
	return fOwner;
}


//	#pragma mark - Hook Functions


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
	// Hook function
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

	if (_width != NULL)
		*_width = fBounds.Width();
	if (_height != NULL)
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


//	#pragma mark - Input Functions


void
BView::BeginRectTracking(BRect startRect, uint32 style)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_BEGIN_RECT_TRACK);
		fOwner->fLink->Attach<BRect>(startRect);
		fOwner->fLink->Attach<uint32>(style);
		fOwner->fLink->Flush();
	}
}


void
BView::EndRectTracking()
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_END_RECT_TRACK);
		fOwner->fLink->Flush();
	}
}


void
BView::DragMessage(BMessage* message, BRect dragRect, BHandler* replyTo)
{
	if (!message)
		return;

	_CheckOwnerLock();

	// calculate the offset
	BPoint offset;
	uint32 buttons;
	BMessage* current = fOwner->CurrentMessage();
	if (!current || current->FindPoint("be:view_where", &offset) != B_OK)
		GetMouse(&offset, &buttons, false);
	offset -= dragRect.LeftTop();

	if (!dragRect.IsValid()) {
		DragMessage(message, NULL, B_OP_BLEND, offset, replyTo);
		return;
	}

	// TODO: that's not really what should happen - the app_server should take
	// the chance *NOT* to need to drag a whole bitmap around but just a frame.

	// create a drag bitmap for the rect
	BBitmap* bitmap = new(std::nothrow) BBitmap(dragRect, B_RGBA32);
	if (bitmap == NULL)
		return;

	uint32* bits = (uint32*)bitmap->Bits();
	uint32 bytesPerRow = bitmap->BytesPerRow();
	uint32 width = dragRect.IntegerWidth() + 1;
	uint32 height = dragRect.IntegerHeight() + 1;
	uint32 lastRow = (height - 1) * width;

	memset(bits, 0x00, height * bytesPerRow);

	// top
	for (uint32 i = 0; i < width; i += 2)
		bits[i] = 0xff000000;

	// bottom
	for (uint32 i = (height % 2 == 0 ? 1 : 0); i < width; i += 2)
		bits[lastRow + i] = 0xff000000;

	// left
	for (uint32 i = 0; i < lastRow; i += width * 2)
		bits[i] = 0xff000000;

	// right
	for (uint32 i = (width % 2 == 0 ? width : 0); i < lastRow; i += width * 2)
		bits[width - 1 + i] = 0xff000000;

	DragMessage(message, bitmap, B_OP_BLEND, offset, replyTo);
}


void
BView::DragMessage(BMessage* message, BBitmap* image, BPoint offset,
	BHandler* replyTo)
{
	DragMessage(message, image, B_OP_COPY, offset, replyTo);
}


void
BView::DragMessage(BMessage* message, BBitmap* image,
	drawing_mode dragMode, BPoint offset, BHandler* replyTo)
{
	if (message == NULL)
		return;

	if (image == NULL) {
		// TODO: workaround for drags without a bitmap - should not be necessary if
		//	we move the rectangle dragging into the app_server
		image = new(std::nothrow) BBitmap(BRect(0, 0, 0, 0), B_RGBA32);
		if (image == NULL)
			return;
	}

	if (replyTo == NULL)
		replyTo = this;

	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	_CheckOwnerLock();

	if (!message->HasInt32("buttons")) {
		BMessage* msg = fOwner->CurrentMessage();
		uint32 buttons;

		if (msg == NULL
			|| msg->FindInt32("buttons", (int32*)&buttons) != B_OK) {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);
	}

	BMessage::Private privateMessage(message);
	privateMessage.SetReply(BMessenger(replyTo, replyTo->Looper()));

	int32 bufferSize = message->FlattenedSize();
	char* buffer = new(std::nothrow) char[bufferSize];
	if (buffer != NULL) {
		message->Flatten(buffer, bufferSize);

		fOwner->fLink->StartMessage(AS_VIEW_DRAG_IMAGE);
		fOwner->fLink->Attach<int32>(image->_ServerToken());
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
		fprintf(stderr, "BView::DragMessage() - no memory to flatten drag "
			"message\n");
	}

	delete image;
}


void
BView::GetMouse(BPoint* _location, uint32* _buttons, bool checkMessageQueue)
{
	if (_location == NULL && _buttons == NULL)
		return;

	_CheckOwnerLockAndSwitchCurrent();

	uint32 eventOptions = fEventOptions | fMouseEventOptions;
	bool noHistory = eventOptions & B_NO_POINTER_HISTORY;
	bool fullHistory = eventOptions & B_FULL_POINTER_HISTORY;

	if (checkMessageQueue && !noHistory) {
		Window()->UpdateIfNeeded();
		BMessageQueue* queue = Window()->MessageQueue();
		queue->Lock();

		// Look out for mouse update messages

		BMessage* message;
		for (int32 i = 0; (message = queue->FindMessage(i)) != NULL; i++) {
			switch (message->what) {
				case B_MOUSE_MOVED:
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
					bool deleteMessage;
					if (!Window()->_StealMouseMessage(message, deleteMessage))
						continue;

					if (!fullHistory && message->what == B_MOUSE_MOVED) {
						// Check if the message is too old. Some applications
						// check the message queue in such a way that mouse
						// messages *must* pile up. This check makes them work
						// as intended, although these applications could simply
						// use the version of BView::GetMouse() that does not
						// check the history. Also note that it isn't a problem
						// to delete the message in case there is not a newer
						// one. If we don't find a message in the queue, we will
						// just fall back to asking the app_sever directly. So
						// the imposed delay will not be a problem on slower
						// computers. This check also prevents another problem,
						// when the message that we use is *not* removed from
						// the queue. Subsequent calls to GetMouse() would find
						// this message over and over!
						bigtime_t eventTime;
						if (message->FindInt64("when", &eventTime) == B_OK
							&& system_time() - eventTime > 10000) {
							// just discard the message
							if (deleteMessage)
								delete message;
							continue;
						}
					}
					message->FindPoint("screen_where", _location);
					message->FindInt32("buttons", (int32*)_buttons);
					queue->Unlock();
						// we need to hold the queue lock until here, because
						// the message might still be used for something else

					if (_location != NULL)
						ConvertFromScreen(_location);

					if (deleteMessage)
						delete message;

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
		BPoint location;
		uint32 buttons;
		fOwner->fLink->Read<BPoint>(&location);
		fOwner->fLink->Read<uint32>(&buttons);
			// TODO: ServerWindow replies with an int32 here

		ConvertFromScreen(&location);
			// TODO: in beos R5, location is already converted to the view
			// local coordinate system, so if an app checks the window message
			// queue by itself, it might not find what it expects.
			// NOTE: the fact that we have mouse coords in screen space in our
			// queue avoids the problem that messages already in the queue will
			// be outdated as soon as a window or even the view moves. The
			// second situation being quite common actually, also with regards
			// to scrolling. An app reading these messages would have to know
			// the locations of the window and view for each message...
			// otherwise it is potentially broken anyways.
		if (_location != NULL)
			*_location = location;
		if (_buttons != NULL)
			*_buttons = buttons;
	} else {
		if (_location != NULL)
			_location->Set(0, 0);
		if (_buttons != NULL)
			*_buttons = 0;
	}
}


void
BView::MakeFocus(bool focusState)
{
	if (fOwner) {
		// TODO: If this view has focus and focusState==false,
		// will there really be no other view with focus? No
		// cycling to the next one?
		BView* focus = fOwner->CurrentFocus();
		if (focusState) {
			// Unfocus a previous focus view
			if (focus && focus != this)
				focus->MakeFocus(false);
			// if we want to make this view the current focus view
			fOwner->_SetFocus(this, true);
		} else {
			// we want to unfocus this view, but only if it actually has focus
			if (focus == this) {
				fOwner->_SetFocus(NULL, true);
			}
		}
	}
}


BScrollBar*
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
BView::ScrollBy(float deltaX, float deltaY)
{
	ScrollTo(BPoint(fBounds.left + deltaX, fBounds.top + deltaY));
}


void
BView::ScrollTo(BPoint where)
{
	// scrolling by fractional values is not supported
	where.x = roundf(where.x);
	where.y = roundf(where.y);

	// no reason to process this further if no scroll is intended.
	if (where.x == fBounds.left && where.y == fBounds.top)
		return;

	// make sure scrolling is within valid bounds
	if (fHorScroller) {
		float min, max;
		fHorScroller->GetRange(&min, &max);

		if (where.x < min)
			where.x = min;
		else if (where.x > max)
			where.x = max;
	}
	if (fVerScroller) {
		float min, max;
		fVerScroller->GetRange(&min, &max);

		if (where.y < min)
			where.y = min;
		else if (where.y > max)
			where.y = max;
	}

	_CheckLockAndSwitchCurrent();

	float xDiff = where.x - fBounds.left;
	float yDiff = where.y - fBounds.top;

	// if we're attached to a window tell app_server about this change
	if (fOwner) {
		fOwner->fLink->StartMessage(AS_VIEW_SCROLL);
		fOwner->fLink->Attach<float>(xDiff);
		fOwner->fLink->Attach<float>(yDiff);

		fOwner->fLink->Flush();

//		fState->valid_flags &= ~B_VIEW_FRAME_BIT;
	}

	// we modify our bounds rectangle by deltaX/deltaY coord units hor/ver.
	fBounds.OffsetTo(where.x, where.y);

	// then set the new values of the scrollbars
	if (fHorScroller && xDiff != 0.0)
		fHorScroller->SetValue(fBounds.left);
	if (fVerScroller && yDiff != 0.0)
		fVerScroller->SetValue(fBounds.top);

}


status_t
BView::SetEventMask(uint32 mask, uint32 options)
{
	if (fEventMask == mask && fEventOptions == options)
		return B_OK;

	// don't change the mask if it's zero and we've got options
	if (mask != 0 || options == 0)
		fEventMask = mask | (fEventMask & 0xffff0000);
	fEventOptions = options;

	fState->archiving_flags |= B_VIEW_EVENT_MASK_BIT;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);
		fOwner->fLink->Flush();
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
		_CheckLockAndSwitchCurrent();
		fMouseEventOptions = options;

		fOwner->fLink->StartMessage(AS_VIEW_SET_MOUSE_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);
		fOwner->fLink->Flush();
		return B_OK;
	}

	return B_ERROR;
}


//	#pragma mark - Graphic State Functions


void
BView::PushState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_PUSH_STATE);

	// initialize origin and scale
	fState->valid_flags |= B_VIEW_SCALE_BIT | B_VIEW_ORIGIN_BIT;
	fState->scale = 1.0f;
	fState->origin.Set(0, 0);
}


void
BView::PopState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_POP_STATE);
	_FlushIfNotInTransaction();

	// invalidate all flags (except those that are not part of pop/push)
	fState->valid_flags = B_VIEW_VIEW_COLOR_BIT;
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

	fState->origin.x = x;
	fState->origin.y = y;

	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_SET_ORIGIN);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_ORIGIN_BIT;
	}

	// our local coord system origin has changed, so when archiving we'll add
	// this too
	fState->archiving_flags |= B_VIEW_ORIGIN_BIT;
}


BPoint
BView::Origin() const
{
	if (!fState->IsValid(B_VIEW_ORIGIN_BIT)) {
		// we don't keep graphics state information, therefor
		// we need to ask the server for the origin after PopState()
		_CheckOwnerLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_ORIGIN);

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
BView::SetScale(float scale) const
{
	if (fState->IsValid(B_VIEW_SCALE_BIT) && scale == fState->scale)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_SCALE);
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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_SCALE);

 		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK)
			fOwner->fLink->Read<float>(&fState->scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}


void
BView::SetLineMode(cap_mode lineCap, join_mode lineJoin, float miterLimit)
{
	if (fState->IsValid(B_VIEW_LINE_MODES_BIT)
		&& lineCap == fState->line_cap && lineJoin == fState->line_join
		&& miterLimit == fState->miter_limit)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		ViewSetLineModeInfo info;
		info.lineJoin = lineJoin;
		info.lineCap = lineCap;
		info.miterLimit = miterLimit;

		fOwner->fLink->StartMessage(AS_VIEW_SET_LINE_MODE);
		fOwner->fLink->Attach<ViewSetLineModeInfo>(info);

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_LINE_MODE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {

			ViewSetLineModeInfo info;
			fOwner->fLink->Read<ViewSetLineModeInfo>(&info);

			fState->line_cap = info.lineCap;
			fState->line_join = info.lineJoin;
			fState->miter_limit = info.miterLimit;
		}

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	return fState->miter_limit;
}


void
BView::SetDrawingMode(drawing_mode mode)
{
	if (fState->IsValid(B_VIEW_DRAWING_MODE_BIT)
		&& mode == fState->drawing_mode)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_DRAWING_MODE);
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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_DRAWING_MODE);

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
		_CheckLockAndSwitchCurrent();

		ViewBlendingModeInfo info;
		info.sourceAlpha = sourceAlpha;
		info.alphaFunction = alphaFunction;

		fOwner->fLink->StartMessage(AS_VIEW_SET_BLENDING_MODE);
		fOwner->fLink->Attach<ViewBlendingModeInfo>(info);

		fState->valid_flags |= B_VIEW_BLENDING_BIT;
	}

	fState->alpha_source_mode = sourceAlpha;
	fState->alpha_function_mode = alphaFunction;

	fState->archiving_flags |= B_VIEW_BLENDING_BIT;
}


void
BView::GetBlendingMode(source_alpha* _sourceAlpha,
	alpha_function* _alphaFunction) const
{
	if (!fState->IsValid(B_VIEW_BLENDING_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_BLENDING_MODE);

		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK) {
 			ViewBlendingModeInfo info;
			fOwner->fLink->Read<ViewBlendingModeInfo>(&info);

			fState->alpha_source_mode = info.sourceAlpha;
			fState->alpha_function_mode = info.alphaFunction;

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PEN_LOC);
		fOwner->fLink->Attach<BPoint>(BPoint(x, y));

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_PEN_LOC);

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PEN_SIZE);
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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_PEN_SIZE);

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_HIGH_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
	}

	fState->high_color = color;

	fState->archiving_flags |= B_VIEW_HIGH_COLOR_BIT;
}


rgb_color
BView::HighColor() const
{
	if (!fState->IsValid(B_VIEW_HIGH_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_HIGH_COLOR);

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_LOW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
	}

	fState->low_color = color;

	fState->archiving_flags |= B_VIEW_LOW_COLOR_BIT;
}


rgb_color
BView::LowColor() const
{
	if (!fState->IsValid(B_VIEW_LOW_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_LOW_COLOR);

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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);
		fOwner->fLink->Flush();

		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
	}

	fState->view_color = color;

	fState->archiving_flags |= B_VIEW_VIEW_COLOR_BIT;
}


rgb_color
BView::ViewColor() const
{
	if (!fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_VIEW_COLOR);

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
	if (fState->IsValid(B_VIEW_FONT_ALIASING_BIT)
		&& enable == fState->font_aliasing)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_PRINT_ALIASING);
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
		// TODO: move this into a BFont method
		if (mask & B_FONT_FAMILY_AND_STYLE)
			fState->font.SetFamilyAndStyle(font->FamilyAndStyle());

		if (mask & B_FONT_SIZE)
			fState->font.SetSize(font->Size());

		if (mask & B_FONT_SHEAR)
			fState->font.SetShear(font->Shear());

		if (mask & B_FONT_ROTATION)
			fState->font.SetRotation(font->Rotation());

		if (mask & B_FONT_FALSE_BOLD_WIDTH)
			fState->font.SetFalseBoldWidth(font->FalseBoldWidth());

		if (mask & B_FONT_SPACING)
			fState->font.SetSpacing(font->Spacing());

		if (mask & B_FONT_ENCODING)
			fState->font.SetEncoding(font->Encoding());

		if (mask & B_FONT_FACE)
			fState->font.SetFace(font->Face());

		if (mask & B_FONT_FLAGS)
			fState->font.SetFlags(font->Flags());
	}

	fState->font_flags |= mask;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fState->UpdateServerFontState(*fOwner->fLink);
		fState->valid_flags |= B_VIEW_FONT_BIT;
	}

	fState->archiving_flags |= B_VIEW_FONT_BIT;
	// TODO: InvalidateLayout() here for convenience?
}


void
BView::GetFont(BFont* font) const
{
	if (!fState->IsValid(B_VIEW_FONT_BIT)) {
		// we don't keep graphics state information, therefor
		// we need to ask the server for the origin after PopState()
		_CheckOwnerLockAndSwitchCurrent();

		// TODO: add a font getter!
		fState->UpdateFrom(*fOwner->fLink);
	}

	*font = fState->font;
}


void
BView::GetFontHeight(font_height* height) const
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
BView::StringWidth(const char* string) const
{
	return fState->font.StringWidth(string);
}


float
BView::StringWidth(const char* string, int32 length) const
{
	return fState->font.StringWidth(string, length);
}


void
BView::GetStringWidths(char* stringArray[], int32 lengthArray[],
	int32 numStrings, float widthArray[]) const
{
	fState->font.GetStringWidths(const_cast<const char**>(stringArray),
		const_cast<const int32*>(lengthArray), numStrings, widthArray);
}


void
BView::TruncateString(BString* string, uint32 mode, float width) const
{
	fState->font.TruncateString(string, mode, width);
}


void
BView::ClipToPicture(BPicture* picture, BPoint where, bool sync)
{
	_ClipToPicture(picture, where, false, sync);
}


void
BView::ClipToInversePicture(BPicture* picture, BPoint where, bool sync)
{
	_ClipToPicture(picture, where, true, sync);
}


void
BView::GetClippingRegion(BRegion* region) const
{
	if (!region)
		return;

	// NOTE: the client has no idea when the clipping in the server
	// changed, so it is always read from the server
	region->MakeEmpty();


	if (fOwner) {
		if (fIsPrinting && _CheckOwnerLock()) {
			region->Set(fState->print_rect);
			return;
		}

		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_GET_CLIP_REGION);

 		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK
 			&& code == B_OK) {
			fOwner->fLink->ReadRegion(region);
			fState->valid_flags |= B_VIEW_CLIP_REGION_BIT;
		}
	}
}


void
BView::ConstrainClippingRegion(BRegion* region)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_SET_CLIP_REGION);

		if (region) {
			int32 count = region->CountRects();
			fOwner->fLink->Attach<int32>(count);
			if (count > 0)
				fOwner->fLink->AttachRegion(*region);
		} else {
			fOwner->fLink->Attach<int32>(-1);
			// '-1' means that in the app_server, there won't be any 'local'
			// clipping region (it will be NULL)
		}

		_FlushIfNotInTransaction();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
		fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
	}
}


//	#pragma mark - Drawing Functions


void
BView::DrawBitmapAsync(const BBitmap* bitmap, BRect bitmapRect, BRect viewRect,
	uint32 options)
{
	if (bitmap == NULL || fOwner == NULL
		|| !bitmapRect.IsValid() || !viewRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawBitmapInfo info;
	info.bitmapToken = bitmap->_ServerToken();
	info.options = options;
	info.viewRect = viewRect;
	info.bitmapRect = bitmapRect;

	fOwner->fLink->StartMessage(AS_VIEW_DRAW_BITMAP);
	fOwner->fLink->Attach<ViewDrawBitmapInfo>(info);

	_FlushIfNotInTransaction();
}


void
BView::DrawBitmapAsync(const BBitmap* bitmap, BRect bitmapRect, BRect viewRect)
{
	DrawBitmapAsync(bitmap, bitmapRect, viewRect, 0);
}


void
BView::DrawBitmapAsync(const BBitmap* bitmap, BRect viewRect)
{
	if (bitmap && fOwner) {
		DrawBitmapAsync(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN),
			viewRect, 0);
	}
}


void
BView::DrawBitmapAsync(const BBitmap* bitmap, BPoint where)
{
	if (bitmap == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawBitmapInfo info;
	info.bitmapToken = bitmap->_ServerToken();
	info.options = 0;
	info.bitmapRect = bitmap->Bounds().OffsetToCopy(B_ORIGIN);
	info.viewRect = info.bitmapRect.OffsetToCopy(where);

	fOwner->fLink->StartMessage(AS_VIEW_DRAW_BITMAP);
	fOwner->fLink->Attach<ViewDrawBitmapInfo>(info);

	_FlushIfNotInTransaction();
}


void
BView::DrawBitmapAsync(const BBitmap* bitmap)
{
	DrawBitmapAsync(bitmap, PenLocation());
}


void
BView::DrawBitmap(const BBitmap* bitmap, BRect bitmapRect, BRect viewRect,
	uint32 options)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, bitmapRect, viewRect, options);
		Sync();
	}
}


void
BView::DrawBitmap(const BBitmap* bitmap, BRect bitmapRect, BRect viewRect)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, bitmapRect, viewRect, 0);
		Sync();
	}
}


void
BView::DrawBitmap(const BBitmap* bitmap, BRect viewRect)
{
	if (bitmap && fOwner) {
		DrawBitmap(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN), viewRect,
			0);
	}
}


void
BView::DrawBitmap(const BBitmap* bitmap, BPoint where)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, where);
		Sync();
	}
}


void
BView::DrawBitmap(const BBitmap* bitmap)
{
	DrawBitmap(bitmap, PenLocation());
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
BView::DrawString(const char* string, escapement_delta* delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), PenLocation(), delta);
}


void
BView::DrawString(const char* string, BPoint location, escapement_delta* delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), location, delta);
}


void
BView::DrawString(const char* string, int32 length, escapement_delta* delta)
{
	DrawString(string, length, PenLocation(), delta);
}


void
BView::DrawString(const char* string, int32 length, BPoint location,
	escapement_delta* delta)
{
	if (fOwner == NULL || string == NULL || length < 1)
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawStringInfo info;
	info.stringLength = length;
	info.location = location;
	if (delta != NULL)
		info.delta = *delta;

	// quite often delta will be NULL
	if (delta)
		fOwner->fLink->StartMessage(AS_DRAW_STRING_WITH_DELTA);
	else
		fOwner->fLink->StartMessage(AS_DRAW_STRING);

	fOwner->fLink->Attach<ViewDrawStringInfo>(info);
	fOwner->fLink->Attach(string, length);

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
BView::DrawString(const char* string, const BPoint* locations,
	int32 locationCount)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), locations, locationCount);
}


void
BView::DrawString(const char* string, int32 length, const BPoint* locations,
	int32 locationCount)
{
	if (fOwner == NULL || string == NULL || length < 1 || locations == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_DRAW_STRING_WITH_OFFSETS);

	fOwner->fLink->Attach<int32>(length);
	fOwner->fLink->Attach<int32>(locationCount);
	fOwner->fLink->Attach(string, length);
	fOwner->fLink->Attach(locations, locationCount * sizeof(BPoint));

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
BView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
	::pattern pattern)
{
	StrokeEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), pattern);
}


void
BView::StrokeEllipse(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
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
BView::FillEllipse(BPoint center, float xRadius, float yRadius,
	const BGradient& gradient)
{
	FillEllipse(BRect(center.x - xRadius, center.y - yRadius,
					  center.x + xRadius, center.y + yRadius), gradient);
}


void
BView::FillEllipse(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ELLIPSE);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::FillEllipse(BRect rect, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ELLIPSE_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::StrokeArc(BPoint center, float xRadius, float yRadius, float startAngle,
	float arcAngle, ::pattern pattern)
{
	StrokeArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
BView::StrokeArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
BView::FillArc(BPoint center,float xRadius, float yRadius, float startAngle,
	float arcAngle, ::pattern pattern)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
BView::FillArc(BPoint center,float xRadius, float yRadius, float startAngle,
	float arcAngle, const BGradient& gradient)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, gradient);
}


void
BView::FillArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
BView::FillArc(BRect rect, float startAngle, float arcAngle,
	const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ARC_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::StrokeBezier(BPoint* controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
BView::FillBezier(BPoint* controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
BView::FillBezier(BPoint* controlPoints, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_BEZIER_GRADIENT);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::StrokePolygon(const BPolygon* polygon, bool closed, ::pattern pattern)
{
	if (!polygon)
		return;

	StrokePolygon(polygon->fPoints, polygon->fCount, polygon->Frame(), closed,
		pattern);
}


void
BView::StrokePolygon(const BPoint* pointArray, int32 numPoints, bool closed,
	::pattern pattern)
{
	BPolygon polygon(pointArray, numPoints);

	StrokePolygon(polygon.fPoints, polygon.fCount, polygon.Frame(), closed,
		pattern);
}


void
BView::StrokePolygon(const BPoint* ptArray, int32 numPoints, BRect bounds,
	bool closed, ::pattern pattern)
{
	if (!ptArray
		|| numPoints <= 1
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	BPolygon polygon(ptArray, numPoints);
	polygon.MapTo(polygon.Frame(), bounds);

	if (fOwner->fLink->StartMessage(AS_STROKE_POLYGON,
			polygon.fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(bool)
				+ sizeof(int32)) == B_OK) {
		fOwner->fLink->Attach<BRect>(polygon.Frame());
		fOwner->fLink->Attach<bool>(closed);
		fOwner->fLink->Attach<int32>(polygon.fCount);
		fOwner->fLink->Attach(polygon.fPoints, polygon.fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
BView::FillPolygon(const BPolygon* polygon, ::pattern pattern)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	if (fOwner->fLink->StartMessage(AS_FILL_POLYGON,
			polygon->fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(int32))
				== B_OK) {
		fOwner->fLink->Attach<BRect>(polygon->Frame());
		fOwner->fLink->Attach<int32>(polygon->fCount);
		fOwner->fLink->Attach(polygon->fPoints,
			polygon->fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
BView::FillPolygon(const BPolygon* polygon, const BGradient& gradient)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	if (fOwner->fLink->StartMessage(AS_FILL_POLYGON_GRADIENT,
			polygon->fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(int32))
				== B_OK) {
		fOwner->fLink->Attach<BRect>(polygon->Frame());
		fOwner->fLink->Attach<int32>(polygon->fCount);
		fOwner->fLink->Attach(polygon->fPoints,
			polygon->fCount * sizeof(BPoint));
		fOwner->fLink->AttachGradient(gradient);

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
BView::FillPolygon(const BPoint* ptArray, int32 numPts, ::pattern pattern)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);
	FillPolygon(&polygon, pattern);
}


void
BView::FillPolygon(const BPoint* ptArray, int32 numPts,
	const BGradient& gradient)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);
	FillPolygon(&polygon, gradient);
}


void
BView::FillPolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
	pattern p)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, p);
}


void
BView::FillPolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
	const BGradient& gradient)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, gradient);
}


void
BView::StrokeRect(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
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

	// NOTE: ensuring compatibility with R5,
	// invalid rects are not filled, they are stroked though!
	if (!rect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_RECT);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
BView::FillRect(BRect rect, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	// NOTE: ensuring compatibility with R5,
	// invalid rects are not filled, they are stroked though!
	if (!rect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_RECT_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::StrokeRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
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

	_CheckLockAndSwitchCurrent();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ROUNDRECT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);

	_FlushIfNotInTransaction();
}


void
BView::FillRoundRect(BRect rect, float xRadius, float yRadius,
	const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ROUNDRECT_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::FillRegion(BRegion* region, ::pattern pattern)
{
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_REGION);
	fOwner->fLink->AttachRegion(*region);

	_FlushIfNotInTransaction();
}


void
BView::FillRegion(BRegion* region, const BGradient& gradient)
{
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_REGION_GRADIENT);
	fOwner->fLink->AttachRegion(*region);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, BRect bounds,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

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
	const BGradient& gradient)
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

		FillTriangle(pt1, pt2, pt3, bounds, gradient);
	}
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_TRIANGLE);
	fOwner->fLink->Attach<BPoint>(pt1);
	fOwner->fLink->Attach<BPoint>(pt2);
	fOwner->fLink->Attach<BPoint>(pt3);
	fOwner->fLink->Attach<BRect>(bounds);

	_FlushIfNotInTransaction();
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	fOwner->fLink->StartMessage(AS_FILL_TRIANGLE_GRADIENT);
	fOwner->fLink->Attach<BPoint>(pt1);
	fOwner->fLink->Attach<BPoint>(pt2);
	fOwner->fLink->Attach<BPoint>(pt3);
	fOwner->fLink->Attach<BRect>(bounds);
	fOwner->fLink->AttachGradient(gradient);

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

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	ViewStrokeLineInfo info;
	info.startPoint = pt0;
	info.endPoint = pt1;

	fOwner->fLink->StartMessage(AS_STROKE_LINE);
	fOwner->fLink->Attach<ViewStrokeLineInfo>(info);

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
BView::StrokeShape(BShape* shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)shape->fPrivateData;
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_SHAPE);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(uint32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

	_FlushIfNotInTransaction();
}


void
BView::FillShape(BShape* shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_SHAPE);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(int32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

	_FlushIfNotInTransaction();
}


void
BView::FillShape(BShape* shape, const BGradient& gradient)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_SHAPE_GRADIENT);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(int32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
BView::BeginLineArray(int32 count)
{
	if (fOwner == NULL)
		return;

	if (count <= 0)
		debugger("Calling BeginLineArray with a count <= 0");

	_CheckLock();

	if (fCommArray) {
		debugger("Can't nest BeginLineArray calls");
			// not fatal, but it helps during
			// development of your app and is in
			// line with R5...
		delete[] fCommArray->array;
		delete fCommArray;
	}

	// TODO: since this method cannot return failure, and further AddLine()
	//	calls with a NULL fCommArray would drop into the debugger anyway,
	//	we allow the possible std::bad_alloc exceptions here...
	fCommArray = new _array_data_;
	fCommArray->maxCount = count;
	fCommArray->count = 0;
	fCommArray->array = new ViewLineArrayInfo[count];
}


void
BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (fOwner == NULL)
		return;

	if (!fCommArray)
		debugger("BeginLineArray must be called before using AddLine");

	_CheckLock();

	const uint32 &arrayCount = fCommArray->count;
	if (arrayCount < fCommArray->maxCount) {
		fCommArray->array[arrayCount].startPoint = pt0;
		fCommArray->array[arrayCount].endPoint = pt1;
		fCommArray->array[arrayCount].color = col;

		fCommArray->count++;
	}
}


void
BView::EndLineArray()
{
	if (fOwner == NULL)
		return;

	if (fCommArray == NULL)
		debugger("Can't call EndLineArray before BeginLineArray");

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_STROKE_LINEARRAY);
	fOwner->fLink->Attach<int32>(fCommArray->count);
	fOwner->fLink->Attach(fCommArray->array,
		fCommArray->count * sizeof(ViewLineArrayInfo));

	_FlushIfNotInTransaction();

	_RemoveCommArray();
}


void
BView::SetDiskMode(char* filename, long offset)
{
	// TODO: implement
	// One BeBook version has this to say about SetDiskMode():
	//
	// "Begins recording a picture to the file with the given filename
	// at the given offset. Subsequent drawing commands sent to the view
	// will be written to the file until EndPicture() is called. The
	// stored commands may be played from the file with DrawPicture()."
}


void
BView::BeginPicture(BPicture* picture)
{
	if (_CheckOwnerLockAndSwitchCurrent()
		&& picture && picture->fUsurped == NULL) {
		picture->Usurp(fCurrentPicture);
		fCurrentPicture = picture;

		fOwner->fLink->StartMessage(AS_VIEW_BEGIN_PICTURE);
	}
}


void
BView::AppendToPicture(BPicture* picture)
{
	_CheckLockAndSwitchCurrent();

	if (picture && picture->fUsurped == NULL) {
		int32 token = picture->Token();

		if (token == -1) {
			BeginPicture(picture);
		} else {
			picture->SetToken(-1);
			picture->Usurp(fCurrentPicture);
			fCurrentPicture = picture;
			fOwner->fLink->StartMessage(AS_VIEW_APPEND_TO_PICTURE);
			fOwner->fLink->Attach<int32>(token);
		}
	}
}


BPicture*
BView::EndPicture()
{
	if (_CheckOwnerLockAndSwitchCurrent() && fCurrentPicture) {
		int32 token;

		fOwner->fLink->StartMessage(AS_VIEW_END_PICTURE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK
			&& fOwner->fLink->Read<int32>(&token) == B_OK) {
			BPicture* picture = fCurrentPicture;
			fCurrentPicture = picture->StepDown();
			picture->SetToken(token);

			// TODO do this more efficient e.g. use a shared area and let the
			// client write into it
			picture->_Download();
			return picture;
		}
	}

	return NULL;
}


void
BView::SetViewBitmap(const BBitmap* bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	_SetViewBitmap(bitmap, srcRect, dstRect, followFlags, options);
}


void
BView::SetViewBitmap(const BBitmap* bitmap, uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();

 	rect.OffsetTo(B_ORIGIN);

	_SetViewBitmap(bitmap, rect, rect, followFlags, options);
}


void
BView::ClearViewBitmap()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


status_t
BView::SetViewOverlay(const BBitmap* overlay, BRect srcRect, BRect dstRect,
	rgb_color* colorKey, uint32 followFlags, uint32 options)
{
	if (overlay == NULL || (overlay->fFlags & B_BITMAP_WILL_OVERLAY) == 0)
		return B_BAD_VALUE;

	status_t status = _SetViewBitmap(overlay, srcRect, dstRect, followFlags,
		options | AS_REQUEST_COLOR_KEY);
	if (status == B_OK) {
		// read the color that will be treated as transparent
		fOwner->fLink->Read<rgb_color>(colorKey);
	}

	return status;
}


status_t
BView::SetViewOverlay(const BBitmap* overlay, rgb_color* colorKey,
	uint32 followFlags, uint32 options)
{
	if (overlay == NULL)
		return B_BAD_VALUE;

	BRect rect = overlay->Bounds();
 	rect.OffsetTo(B_ORIGIN);

	return SetViewOverlay(overlay, rect, rect, colorKey, followFlags, options);
}


void
BView::ClearViewOverlay()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


void
BView::CopyBits(BRect src, BRect dst)
{
	if (fOwner == NULL)
		return;

	if (!src.IsValid() || !dst.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_COPY_BITS);
	fOwner->fLink->Attach<BRect>(src);
	fOwner->fLink->Attach<BRect>(dst);

	_FlushIfNotInTransaction();
}


void
BView::DrawPicture(const BPicture* picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
	Sync();
}


void
BView::DrawPicture(const BPicture* picture, BPoint where)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, where);
	Sync();
}


void
BView::DrawPicture(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	DrawPictureAsync(filename, offset, where);
	Sync();
}


void
BView::DrawPictureAsync(const BPicture* picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
}


void
BView::DrawPictureAsync(const BPicture* picture, BPoint where)
{
	if (picture == NULL)
		return;

	if (_CheckOwnerLockAndSwitchCurrent() && picture->Token() > 0) {
		fOwner->fLink->StartMessage(AS_VIEW_DRAW_PICTURE);
		fOwner->fLink->Attach<int32>(picture->Token());
		fOwner->fLink->Attach<BPoint>(where);

		_FlushIfNotInTransaction();
	}
}


void
BView::DrawPictureAsync(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	// TODO: Test
	BFile file(filename, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	file.Seek(offset, SEEK_SET);

	BPicture picture;
	if (picture.Unflatten(&file) < B_OK)
		return;

	DrawPictureAsync(&picture, where);
}


void
BView::Invalidate(BRect invalRect)
{
	if (fOwner == NULL)
		return;

	// NOTE: This rounding of the invalid rect is to stay compatible with BeOS.
	// On the server side, the invalid rect will be converted to a BRegion,
	// which rounds in a different manner, so that it really includes the
	// fractional coordinates of a BRect (ie ceilf(rect.right) &
	// ceilf(rect.bottom)), which is also what BeOS does. So we have to do the
	// different rounding here to stay compatible in both ways.
	invalRect.left = (int)invalRect.left;
	invalRect.top = (int)invalRect.top;
	invalRect.right = (int)invalRect.right;
	invalRect.bottom = (int)invalRect.bottom;
	if (!invalRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_INVALIDATE_RECT);
	fOwner->fLink->Attach<BRect>(invalRect);

// TODO: determine why this check isn't working correctly.
#if 0
	if (!fOwner->fUpdateRequested) {
		fOwner->fLink->Flush();
		fOwner->fUpdateRequested = true;
	}
#else
	fOwner->fLink->Flush();
#endif
}


void
BView::Invalidate(const BRegion* region)
{
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_INVALIDATE_REGION);
	fOwner->fLink->AttachRegion(*region);

// TODO: See above.
#if 0
	if (!fOwner->fUpdateRequested) {
		fOwner->fLink->Flush();
		fOwner->fUpdateRequested = true;
	}
#else
	fOwner->fLink->Flush();
#endif
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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_INVERT_RECT);
		fOwner->fLink->Attach<BRect>(rect);

		_FlushIfNotInTransaction();
	}
}


//	#pragma mark - View Hierarchy Functions


void
BView::AddChild(BView* child, BView* before)
{
	STRACE(("BView(%s)::AddChild(child '%s', before '%s')\n",
 		this->Name(),
 		child != NULL && child->Name() ? child->Name() : "NULL",
 		before != NULL && before->Name() ? before->Name() : "NULL"));

	if (!_AddChild(child, before))
		return;

	if (fLayoutData->fLayout)
		fLayoutData->fLayout->AddView(child);
}


bool
BView::AddChild(BLayoutItem* child)
{
	if (!fLayoutData->fLayout)
		return false;
	return fLayoutData->fLayout->AddItem(child);
}


bool
BView::_AddChild(BView* child, BView* before)
{
	if (!child)
		return false;

	if (child->fParent != NULL) {
		debugger("AddChild failed - the view already has a parent.");
		return false;
	}

	bool lockedOwner = false;
	if (fOwner && !fOwner->IsLocked()) {
		fOwner->Lock();
		lockedOwner = true;
	}

	if (!_AddChildToList(child, before)) {
		debugger("AddChild failed!");
		if (lockedOwner)
			fOwner->Unlock();
		return false;
	}

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		child->_SetOwner(fOwner);
		child->_CreateSelf();
		child->_Attach();

		if (lockedOwner)
			fOwner->Unlock();
	}

	InvalidateLayout();

	return true;
}


bool
BView::RemoveChild(BView* child)
{
	STRACE(("BView(%s)::RemoveChild(%s)\n", Name(), child->Name()));

	if (!child)
		return false;

	if (child->fParent != this)
		return false;

	return child->RemoveSelf();
}

int32
BView::CountChildren() const
{
	_CheckLock();

	uint32 count = 0;
	BView* child = fFirstChild;

	while (child != NULL) {
		count++;
		child = child->fNextSibling;
	}

	return count;
}


BView*
BView::ChildAt(int32 index) const
{
	_CheckLock();

	BView* child = fFirstChild;
	while (child != NULL && index-- > 0) {
		child = child->fNextSibling;
	}

	return child;
}


BView*
BView::NextSibling() const
{
	return fNextSibling;
}


BView*
BView::PreviousSibling() const
{
	return fPreviousSibling;
}


bool
BView::RemoveSelf()
{
	if (fParent && fParent->fLayoutData->fLayout) {
		int32 itemsRemaining = fLayoutData->fLayoutItems.CountItems();
		while (itemsRemaining-- > 0) {
			BLayoutItem* item = fLayoutData->fLayoutItems.ItemAt(0);
				// always remove item at index 0, since items are shuffled
				// downwards by BObjectList
			item->Layout()->RemoveItem(item);
				// removes item from fLayoutItems list
			delete item;
		}
	}

	return _RemoveSelf();
}


bool
BView::_RemoveSelf()
{
	STRACE(("BView(%s)::_RemoveSelf()\n", Name()));

	// Remove this child from its parent

	BWindow* owner = fOwner;
	_CheckLock();

	if (owner != NULL) {
		_UpdateStateForRemove();
		_Detach();
	}

	BView* parent = fParent;
	if (!parent || !parent->_RemoveChildFromList(this))
		return false;

	if (owner != NULL && !fTopLevelView) {
		// the top level view is deleted by the app_server automatically
		owner->fLink->StartMessage(AS_VIEW_DELETE);
		owner->fLink->Attach<int32>(_get_object_token_(this));
	}

	parent->InvalidateLayout();

	STRACE(("DONE: BView(%s)::_RemoveSelf()\n", Name()));

	return true;
}


BView*
BView::Parent() const
{
	if (fParent && fParent->fTopLevelView)
		return NULL;

	return fParent;
}


BView*
BView::FindView(const char* name) const
{
	if (name == NULL)
		return NULL;

	if (Name() != NULL && !strcmp(Name(), name))
		return const_cast<BView*>(this);

	BView* child = fFirstChild;
	while (child != NULL) {
		BView* view = child->FindView(name);
		if (view != NULL)
			return view;

		child = child->fNextSibling;
	}

	return NULL;
}


void
BView::MoveBy(float deltaX, float deltaY)
{
	MoveTo(fParentOffset.x + roundf(deltaX), fParentOffset.y + roundf(deltaY));
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
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_MOVE_TO);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

//		fState->valid_flags |= B_VIEW_FRAME_BIT;

		_FlushIfNotInTransaction();
	}

	_MoveTo((int32)x, (int32)y);
}


void
BView::ResizeBy(float deltaWidth, float deltaHeight)
{
	// BeBook says we should do this. And it makes sense.
	deltaWidth = roundf(deltaWidth);
	deltaHeight = roundf(deltaHeight);

	if (deltaWidth == 0 && deltaHeight == 0)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_RESIZE_TO);

		fOwner->fLink->Attach<float>(fBounds.Width() + deltaWidth);
		fOwner->fLink->Attach<float>(fBounds.Height() + deltaHeight);

//		fState->valid_flags |= B_VIEW_FRAME_BIT;

		_FlushIfNotInTransaction();
	}

	_ResizeBy((int32)deltaWidth, (int32)deltaHeight);
}


void
BView::ResizeTo(float width, float height)
{
	ResizeBy(width - fBounds.Width(), height - fBounds.Height());
}


void
BView::ResizeTo(BSize size)
{
	ResizeBy(size.width - fBounds.Width(), size.height - fBounds.Height());
}


//	#pragma mark - Inherited Methods (from BHandler)


status_t
BView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-view");
	BPropertyInfo propertyInfo(sViewPropInfo);
	if (status == B_OK)
		status = data->AddFlat("messages", &propertyInfo);
	if (status == B_OK)
		return BHandler::GetSupportedSuites(data);
	return status;
}


BHandler*
BView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 what,	const char* property)
{
	if (msg->what == B_WINDOW_MOVE_BY
		|| msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(sViewPropInfo);
	status_t err = B_BAD_SCRIPT_SYNTAX;
	BMessage replyMsg(B_REPLY);

	switch (propertyInfo.FindMatch(msg, index, specifier, what, property)) {
		case 0:
		case 1:
		case 3:
			return this;

		case 2:
			if (fShelf) {
				msg->PopSpecifier();
				return fShelf;
			}

			err = B_NAME_NOT_FOUND;
			replyMsg.AddString("message", "This window doesn't have a shelf");
			break;

		case 4:
		{
			if (!fFirstChild) {
				err = B_NAME_NOT_FOUND;
				replyMsg.AddString("message", "This window doesn't have "
					"children.");
				break;
			}
			BView* child = NULL;
			switch (what) {
				case B_INDEX_SPECIFIER:
				{
					int32 index;
					err = specifier->FindInt32("index", &index);
					if (err == B_OK)
						child = ChildAt(index);
					break;
				}
				case B_REVERSE_INDEX_SPECIFIER:
				{
					int32 rindex;
					err = specifier->FindInt32("index", &rindex);
					if (err == B_OK)
						child = ChildAt(CountChildren() - rindex);
					break;
				}
				case B_NAME_SPECIFIER:
				{
					const char* name;
					err = specifier->FindString("name", &name);
					if (err == B_OK)
						child = FindView(name);
					break;
				}
			}

			if (child != NULL) {
				msg->PopSpecifier();
				return child;
			}

			if (err == B_OK)
				err = B_BAD_INDEX;

			replyMsg.AddString("message",
				"Cannot find view at/with specified index/name.");
			break;
		}

		default:
			return BHandler::ResolveSpecifier(msg, index, specifier, what,
				property);
	}

	if (err < B_OK) {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;

		if (err == B_BAD_SCRIPT_SYNTAX)
			replyMsg.AddString("message", "Didn't understand the specifier(s)");
		else
			replyMsg.AddString("message", strerror(err));
	}

	replyMsg.AddInt32("error", err);
	msg->SendReply(&replyMsg);
	return NULL;
}


void
BView::MessageReceived(BMessage* msg)
{
	if (!msg->HasSpecifiers()) {
		switch (msg->what) {
			case B_VIEW_RESIZED:
				// By the time the message arrives, the bounds may have
				// changed already, that's why we don't use the values
				// in the message itself.
				FrameResized(fBounds.Width(), fBounds.Height());
				break;

			case B_VIEW_MOVED:
				FrameMoved(fParentOffset);
				break;

			case B_MOUSE_IDLE:
			{
				BPoint where;
				if (msg->FindPoint("be:view_where", &where) != B_OK)
					break;

				BToolTip* tip;
				if (GetToolTipAt(where, &tip))
					ShowToolTip(tip);
				else
					BHandler::MessageReceived(msg);
				break;
			}

			case B_MOUSE_WHEEL_CHANGED:
			{
				BScrollBar* horizontal = ScrollBar(B_HORIZONTAL);
				BScrollBar* vertical = ScrollBar(B_VERTICAL);
				if (horizontal == NULL && vertical == NULL) {
					// Pass the message to the next handler
					BHandler::MessageReceived(msg);
					break;
				}

				float deltaX = 0.0f, deltaY = 0.0f;
				if (horizontal != NULL)
					msg->FindFloat("be:wheel_delta_x", &deltaX);
				if (vertical != NULL)
					msg->FindFloat("be:wheel_delta_y", &deltaY);

				if (deltaX == 0.0f && deltaY == 0.0f)
					break;

				float smallStep, largeStep;
				if (horizontal != NULL) {
					horizontal->GetSteps(&smallStep, &largeStep);

					// pressing the option/command/control key scrolls faster
					if (modifiers()
						& (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) {
						deltaX *= largeStep;
					} else
						deltaX *= smallStep * 3;

					horizontal->SetValue(horizontal->Value() + deltaX);
				}

				if (vertical != NULL) {
					vertical->GetSteps(&smallStep, &largeStep);

					// pressing the option/command/control key scrolls faster
					if (modifiers()
						& (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) {
						deltaY *= largeStep;
					} else
						deltaY *= smallStep * 3;

					vertical->SetValue(vertical->Value() + deltaY);
				}
				break;
			}

			default:
				BHandler::MessageReceived(msg);
				break;
		}

		return;
	}

	// Scripting message

	BMessage replyMsg(B_REPLY);
	status_t err = B_BAD_SCRIPT_SYNTAX;
	int32 index;
	BMessage specifier;
	int32 what;
	const char* property;

	if (msg->GetCurrentSpecifier(&index, &specifier, &what, &property) != B_OK)
		return BHandler::MessageReceived(msg);

	BPropertyInfo propertyInfo(sViewPropInfo);
	switch (propertyInfo.FindMatch(msg, index, &specifier, what, property)) {
		case 0:
			if (msg->what == B_GET_PROPERTY) {
				err = replyMsg.AddRect("result", Frame());
			} else if (msg->what == B_SET_PROPERTY) {
				BRect newFrame;
				err = msg->FindRect("data", &newFrame);
				if (err == B_OK) {
					MoveTo(newFrame.LeftTop());
					ResizeTo(newFrame.Width(), newFrame.Height());
				}
			}
			break;
		case 1:
			if (msg->what == B_GET_PROPERTY) {
				err = replyMsg.AddBool("result", IsHidden());
			} else if (msg->what == B_SET_PROPERTY) {
				bool newHiddenState;
				err = msg->FindBool("data", &newHiddenState);
				if (err == B_OK) {
					if (newHiddenState == true)
						Hide();
					else
						Show();
				}
			}
			break;
		case 3:
			err = replyMsg.AddInt32("result", CountChildren());
			break;
		default:
			return BHandler::MessageReceived(msg);
	}

	if (err != B_OK) {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;

		if (err == B_BAD_SCRIPT_SYNTAX)
			replyMsg.AddString("message", "Didn't understand the specifier(s)");
		else
			replyMsg.AddString("message", strerror(err));

		replyMsg.AddInt32("error", err);
	}

	msg->SendReply(&replyMsg);
}


status_t
BView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BView::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_CHANGED:
		{
			BView::LayoutChanged();
			return B_OK;
		}
		case PERFORM_CODE_GET_TOOL_TIP_AT:
		{
			perform_data_get_tool_tip_at* data
				= (perform_data_get_tool_tip_at*)_data;
			data->return_value
				= BView::GetToolTipAt(data->point, data->tool_tip);
			return B_OK;
		}
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data =
				(perform_data_all_unarchived*)_data;

			data->return_value = BView::AllUnarchived(data->archive);
			return B_OK;
		}
		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data =
				(perform_data_all_archived*)_data;

			data->return_value = BView::AllArchived(data->archive);
			return B_OK;
		}
	}

	return BHandler::Perform(code, _data);
}


// #pragma mark - Layout Functions


BSize
BView::MinSize()
{
	// TODO: make sure this works correctly when some methods are overridden
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(fLayoutData->fMinSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->MinSize()
			: BSize(width, height)));
}


BSize
BView::MaxSize()
{
	return BLayoutUtils::ComposeSize(fLayoutData->fMaxSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->MaxSize()
			: BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED)));
}


BSize
BView::PreferredSize()
{
	// TODO: make sure this works correctly when some methods are overridden
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(fLayoutData->fPreferredSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->PreferredSize()
			: BSize(width, height)));
}


BAlignment
BView::LayoutAlignment()
{
	return BLayoutUtils::ComposeAlignment(fLayoutData->fAlignment,
		(fLayoutData->fLayout ? fLayoutData->fLayout->Alignment()
			: BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER)));
}


void
BView::SetExplicitMinSize(BSize size)
{
	fLayoutData->fMinSize = size;
	InvalidateLayout();
}


void
BView::SetExplicitMaxSize(BSize size)
{
	fLayoutData->fMaxSize = size;
	InvalidateLayout();
}


void
BView::SetExplicitPreferredSize(BSize size)
{
	fLayoutData->fPreferredSize = size;
	InvalidateLayout();
}


void
BView::SetExplicitAlignment(BAlignment alignment)
{
	fLayoutData->fAlignment = alignment;
	InvalidateLayout();
}


BSize
BView::ExplicitMinSize() const
{
	return fLayoutData->fMinSize;
}


BSize
BView::ExplicitMaxSize() const
{
	return fLayoutData->fMaxSize;
}


BSize
BView::ExplicitPreferredSize() const
{
	return fLayoutData->fPreferredSize;
}


BAlignment
BView::ExplicitAlignment() const
{
	return fLayoutData->fAlignment;
}


bool
BView::HasHeightForWidth()
{
	return (fLayoutData->fLayout
		? fLayoutData->fLayout->HasHeightForWidth() : false);
}


void
BView::GetHeightForWidth(float width, float* min, float* max, float* preferred)
{
	if (fLayoutData->fLayout)
		fLayoutData->fLayout->GetHeightForWidth(width, min, max, preferred);
}


void
BView::SetLayout(BLayout* layout)
{
	if (layout == fLayoutData->fLayout)
		return;

	if (layout && layout->Layout())
		debugger("BView::SetLayout() failed, layout is already in use.");

	fFlags |= B_SUPPORTS_LAYOUT;

	// unset and delete the old layout
	if (fLayoutData->fLayout) {
		fLayoutData->fLayout->SetOwner(NULL);
		delete fLayoutData->fLayout;
	}

	fLayoutData->fLayout = layout;

	if (fLayoutData->fLayout) {
		fLayoutData->fLayout->SetOwner(this);

		// add all children
		int count = CountChildren();
		for (int i = 0; i < count; i++)
			fLayoutData->fLayout->AddView(ChildAt(i));
	}

	InvalidateLayout();
}


BLayout*
BView::GetLayout() const
{
	return fLayoutData->fLayout;
}


void
BView::InvalidateLayout(bool descendants)
{
	// printf("BView(%p)::InvalidateLayout(%i), valid: %i, inProgress: %i\n",
	//	this, descendants, fLayoutData->fLayoutValid,
	//	fLayoutData->fLayoutInProgress);

	if (!fLayoutData->fMinMaxValid || fLayoutData->fLayoutInProgress
 			|| fLayoutData->fLayoutInvalidationDisabled > 0) {
		return;
	}

	fLayoutData->fLayoutValid = false;
	fLayoutData->fMinMaxValid = false;
	LayoutInvalidated(descendants);

	if (descendants) {
		for (BView* child = fFirstChild;
			child; child = child->fNextSibling) {
			child->InvalidateLayout(descendants);
		}
	}

	if (fLayoutData->fLayout)
		fLayoutData->fLayout->InvalidateLayout(descendants);
	else
		_InvalidateParentLayout();

	if (fTopLevelView && fOwner)
		fOwner->PostMessage(B_LAYOUT_WINDOW);
}


void
BView::EnableLayoutInvalidation()
{
	if (fLayoutData->fLayoutInvalidationDisabled > 0)
		fLayoutData->fLayoutInvalidationDisabled--;
}


void
BView::DisableLayoutInvalidation()
{
	fLayoutData->fLayoutInvalidationDisabled++;
}


bool
BView::IsLayoutValid() const
{
	return fLayoutData->fLayoutValid;
}


/*!	\brief Service call for BView derived classes reenabling
	InvalidateLayout() notifications.

	BLayout & BView will avoid calling InvalidateLayout on views that have
	already been invalidated, but if the view caches internal layout information
	which it updates in methods other than DoLayout(), it has to invoke this
	method, when it has done so, since otherwise the information might become
	obsolete without the layout noticing.
*/
void
BView::ResetLayoutInvalidation()
{
	fLayoutData->fMinMaxValid = true;
}


BLayoutContext*
BView::LayoutContext() const
{
	return fLayoutData->fLayoutContext;
}


void
BView::Layout(bool force)
{
	BLayoutContext context;
	_Layout(force, &context);
}


void
BView::Relayout()
{
	if (fLayoutData->fLayoutValid && !fLayoutData->fLayoutInProgress) {
		fLayoutData->fNeedsRelayout = true;
		if (fLayoutData->fLayout)
			fLayoutData->fLayout->RequireLayout();

		// Layout() is recursive, that is if the parent view is currently laid
		// out, we don't call layout() on this view, but wait for the parent's
		// Layout() to do that for us.
		if (!fParent || !fParent->fLayoutData->fLayoutInProgress)
			Layout(false);
	}
}


void
BView::LayoutInvalidated(bool descendants)
{
	// hook method
}


void
BView::DoLayout()
{
	if (fLayoutData->fLayout)
		fLayoutData->fLayout->_LayoutWithinContext(false, LayoutContext());
}


void
BView::SetToolTip(const char* text)
{
	if (text == NULL || text[0] == '\0')
		return;

	if (BTextToolTip* tip = dynamic_cast<BTextToolTip*>(fToolTip))
		tip->SetText(text);
	else
		SetToolTip(new BTextToolTip(text));
}


void
BView::SetToolTip(BToolTip* tip)
{
	if (fToolTip == tip)
		return;

	if (fToolTip != NULL)
		fToolTip->ReleaseReference();
	fToolTip = tip;
	if (fToolTip != NULL)
		fToolTip->AcquireReference();
}


BToolTip*
BView::ToolTip() const
{
	return fToolTip;
}


void
BView::ShowToolTip(BToolTip* tip)
{
	if (tip == NULL)
		return;

	BPoint where;
	GetMouse(&where, NULL, false);

	BToolTipManager::Manager()->ShowTip(tip, ConvertToScreen(where), this);
}


void
BView::HideToolTip()
{
	BToolTipManager::Manager()->HideTip();
}


bool
BView::GetToolTipAt(BPoint point, BToolTip** _tip)
{
	if (fToolTip != NULL) {
		*_tip = fToolTip;
		return true;
	}

	*_tip = NULL;
	return false;
}


void
BView::LayoutChanged()
{
	// hook method
}


void
BView::_Layout(bool force, BLayoutContext* context)
{
//printf("%p->BView::_Layout(%d, %p)\n", this, force, context);
//printf("  fNeedsRelayout: %d, fLayoutValid: %d, fLayoutInProgress: %d\n",
//fLayoutData->fNeedsRelayout, fLayoutData->fLayoutValid,
//fLayoutData->fLayoutInProgress);
	if (fLayoutData->fNeedsRelayout || !fLayoutData->fLayoutValid || force) {
		fLayoutData->fLayoutValid = false;

		if (fLayoutData->fLayoutInProgress)
			return;

		BLayoutContext* oldContext = fLayoutData->fLayoutContext;
		fLayoutData->fLayoutContext = context;

		fLayoutData->fLayoutInProgress = true;
		DoLayout();
		fLayoutData->fLayoutInProgress = false;

		fLayoutData->fLayoutValid = true;
		fLayoutData->fMinMaxValid = true;
		fLayoutData->fNeedsRelayout = false;

		// layout children
		for(BView* child = fFirstChild; child; child = child->fNextSibling) {
			if (!child->IsHidden(child))
				child->_Layout(force, context);
		}

		LayoutChanged();

		fLayoutData->fLayoutContext = oldContext;

		// invalidate the drawn content, if requested
		if (fFlags & B_INVALIDATE_AFTER_LAYOUT)
			Invalidate();
	}
}


void
BView::_LayoutLeft(BLayout* deleted)
{
	// If our layout is added to another layout (via BLayout::AddItem())
	// then we share ownership of our layout. In the event that our layout gets
	// deleted by the layout it has been added to, this method is called so
	// that we don't double-delete our layout.
	if (fLayoutData->fLayout == deleted)
		fLayoutData->fLayout = NULL;
	InvalidateLayout();
}


void
BView::_InvalidateParentLayout()
{
	if (!fParent)
		return;

	BLayout* layout = fLayoutData->fLayout;
	BLayout* layoutParent = layout ? layout->Layout() : NULL;
	if (layoutParent) {
		layoutParent->InvalidateLayout();
	} else if (fLayoutData->fLayoutItems.CountItems() > 0) {
		int32 count = fLayoutData->fLayoutItems.CountItems();
		for (int32 i = 0; i < count; i++) {
			fLayoutData->fLayoutItems.ItemAt(i)->Layout()->InvalidateLayout();
		}
	} else {
		fParent->InvalidateLayout();
	}
}


//	#pragma mark - Private Functions


void
BView::_InitData(BRect frame, const char* name, uint32 resizingMode,
	uint32 flags)
{
	// Info: The name of the view is set by BHandler constructor

	STRACE(("BView::_InitData: enter\n"));

	// initialize members
	if ((resizingMode & ~_RESIZE_MASK_) || (flags & _RESIZE_MASK_))
		printf("%s BView::_InitData(): resizing mode or flags swapped\n", name);

	// There are applications that swap the resize mask and the flags in the
	// BView constructor. This does not cause problems under BeOS as it just
	// ors the two fields to one 32bit flag.
	// For now we do the same but print the above warning message.
	// TODO: this should be removed at some point and the original
	// version restored:
	// fFlags = (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);
	fFlags = resizingMode | flags;

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

	fCurrentPicture = NULL;
	fCommArray = NULL;

	fVerScroller = NULL;
	fHorScroller = NULL;

	fIsPrinting = false;
	fAttached = false;

	// TODO: Since we cannot communicate failure, we don't use std::nothrow here
	// TODO: Maybe we could auto-delete those views on AddChild() instead?
	fState = new BPrivate::ViewState;

	fBounds = frame.OffsetToCopy(B_ORIGIN);
	fShelf = NULL;

	fEventMask = 0;
	fEventOptions = 0;
	fMouseEventOptions = 0;

	fLayoutData = new LayoutData;

	fToolTip = NULL;
}


void
BView::_RemoveCommArray()
{
	if (fCommArray) {
		delete [] fCommArray->array;
		delete fCommArray;
		fCommArray = NULL;
	}
}


void
BView::_SetOwner(BWindow* newOwner)
{
	if (!newOwner)
		_RemoveCommArray();

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

	for (BView* child = fFirstChild; child != NULL; child = child->fNextSibling)
		child->_SetOwner(newOwner);
}


void
BView::_ClipToPicture(BPicture* picture, BPoint where, bool invert, bool sync)
{
	if (!picture)
		return;

#if 1
	// TODO: Move the implementation to the server!!!
	// This implementation is pretty slow, since just creating an offscreen
	// bitmap takes a lot of time. That's the main reason why it should be moved
	// to the server.

	// Here the idea is to get rid of the padding bytes in the bitmap,
	// as padding complicates and slows down the iteration.
	// TODO: Maybe it's not so nice as it assumes BBitmaps to be aligned
	// to a 4 byte boundary.
	BRect bounds(Bounds());
	if ((bounds.IntegerWidth() + 1) % 32) {
		bounds.right = bounds.left + ((bounds.IntegerWidth() + 1) / 32 + 1)
			* 32 - 1;
	}

	// TODO: I used a RGBA32 bitmap because drawing on a GRAY8 doesn't work.
	BBitmap* bitmap = new(std::nothrow) BBitmap(bounds, B_RGBA32, true);
	if (bitmap != NULL && bitmap->InitCheck() == B_OK && bitmap->Lock()) {
		BView* view = new(std::nothrow) BView(bounds, "drawing view",
			B_FOLLOW_NONE, 0);
		if (view != NULL) {
			bitmap->AddChild(view);
			view->DrawPicture(picture, where);
			view->Sync();
		}
		bitmap->Unlock();
	}

	BRegion region;
	int32 width = bounds.IntegerWidth() + 1;
	int32 height = bounds.IntegerHeight() + 1;
	if (bitmap != NULL && bitmap->LockBits() == B_OK) {
		uint32 bit = 0;
		uint32* bits = (uint32*)bitmap->Bits();
		clipping_rect rect;

		// TODO: A possible optimization would be adding "spans" instead
		// of 1x1 rects. That would probably help with very complex
		// BPictures
		for (int32 y = 0; y < height; y++) {
			for (int32 x = 0; x < width; x++) {
				bit = *bits++;
				if (bit != 0xFFFFFFFF) {
					rect.left = x;
					rect.right = rect.left;
					rect.top = rect.bottom = y;
					region.Include(rect);
				}
			}
		}
		bitmap->UnlockBits();
	}
	delete bitmap;

	if (invert) {
		BRegion inverseRegion;
		inverseRegion.Include(Bounds());
		inverseRegion.Exclude(&region);
		ConstrainClippingRegion(&inverseRegion);
	} else
		ConstrainClippingRegion(&region);
#else
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_CLIP_TO_PICTURE);
		fOwner->fLink->Attach<int32>(picture->Token());
		fOwner->fLink->Attach<BPoint>(where);
		fOwner->fLink->Attach<bool>(invert);

		// TODO: I think that "sync" means another thing here:
		// the bebook, at least, says so.
		if (sync)
			fOwner->fLink->Flush();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
	}

	fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
#endif
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
		BView* last = fFirstChild;
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
	// AS_VIEW_CREATE & AS_VIEW_CREATE_ROOT do not use the
	// current view mechanism via _CheckLockAndSwitchCurrent() - the token
	// of the view and its parent are both send to the server.

	if (fTopLevelView)
		fOwner->fLink->StartMessage(AS_VIEW_CREATE_ROOT);
	else
 		fOwner->fLink->StartMessage(AS_VIEW_CREATE);

	fOwner->fLink->Attach<int32>(_get_object_token_(this));
	fOwner->fLink->AttachString(Name());
	fOwner->fLink->Attach<BRect>(Frame());
	fOwner->fLink->Attach<BPoint>(LeftTop());
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

	_CheckOwnerLockAndSwitchCurrent();
	fState->UpdateServerState(*fOwner->fLink);

	// we create all its children, too

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_CreateSelf();
	}

	fOwner->fLink->Flush();
	return true;
}


/*!	Sets the new view position.
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
		BMessage moved(B_VIEW_MOVED);
		moved.AddInt64("when", system_time());
		moved.AddPoint("where", BPoint(x, y));

		BMessenger target(this);
		target.SendMessage(&moved);
	}
}


/*!	Computes the actual new frame size and recalculates the size of
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
	if (fFlags & B_SUPPORTS_LAYOUT) {
		Relayout();
	} else {
		for (BView* child = fFirstChild; child; child = child->fNextSibling)
			child->_ParentResizedBy(deltaWidth, deltaHeight);
	}

	if (fFlags & B_FRAME_EVENTS) {
		BMessage resized(B_VIEW_RESIZED);
		resized.AddInt64("when", system_time());
		resized.AddInt32("width", fBounds.IntegerWidth());
		resized.AddInt32("height", fBounds.IntegerHeight());

		BMessenger target(this);
		target.SendMessage(&resized);
	}
}


/*!	Relayouts the view according to its resizing mode. */
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
		_MoveTo((int32)roundf(newFrame.left), (int32)roundf(newFrame.top));
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

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
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
			_CheckLock();
			if (fOwner->fPulseRunner == NULL)
				fOwner->SetPulseRate(fOwner->PulseRate());
		}

		if (!fOwner->IsHidden())
			Invalidate();
	}

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
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

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Detach();
	}

	AllDetached();

	if (fOwner) {
		_CheckLock();

		if (!fOwner->IsHidden())
			Invalidate();

		// make sure our owner doesn't need us anymore

		if (fOwner->CurrentFocus() == this) {
			MakeFocus(false);
			// MakeFocus() is virtual and might not be
			// passing through to the BView version,
			// but we need to make sure at this point
			// that we are not the focus view anymore.
			if (fOwner->CurrentFocus() == this)
				fOwner->_SetFocus(NULL, true);
		}

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
BView::_Draw(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW))
		return;

	// NOTE: if ViewColor() == B_TRANSPARENT_COLOR and no B_WILL_DRAW
	// -> View is simply not drawn at all

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (the hook implementation could
	// mess things up if it uses non-matching Push- and PopState(),
	// we would not be guaranteed to still have the same state on
	// the stack after having called Draw())
	PushState();
	Draw(updateRect);
	PopState();
	Flush();
}


void
BView::_DrawAfterChildren(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW)
		|| !(Flags() & B_DRAW_ON_CHILDREN))
		return;

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (see above)
	PushState();
	DrawAfterChildren(updateRect);
	PopState();
	Flush();
}


void
BView::_Pulse()
{
	if ((Flags() & B_PULSE_NEEDED) != 0)
		Pulse();

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Pulse();
	}
}


void
BView::_UpdateStateForRemove()
{
	// TODO: _CheckLockAndSwitchCurrent() would be good enough, no?
	if (!_CheckOwnerLockAndSwitchCurrent())
		return;

	fState->UpdateFrom(*fOwner->fLink);
//	if (!fState->IsValid(B_VIEW_FRAME_BIT)) {
//		fOwner->fLink->StartMessage(AS_VIEW_GET_COORD);
//
//		status_t code;
//		if (fOwner->fLink->FlushWithReply(code) == B_OK
//			&& code == B_OK) {
//			fOwner->fLink->Read<BPoint>(&fParentOffset);
//			fOwner->fLink->Read<BRect>(&fBounds);
//			fState->valid_flags |= B_VIEW_FRAME_BIT;
//		}
//	}

	// update children as well

	for (BView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
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
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PATTERN);
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


BShelf*
BView::_Shelf() const
{
	return fShelf;
}


void
BView::_SetShelf(BShelf* shelf)
{
	if (fShelf != NULL && fOwner != NULL)
		fOwner->RemoveHandler(fShelf);

	fShelf = shelf;

	if (fShelf != NULL && fOwner != NULL)
		fOwner->AddHandler(fShelf);
}


status_t
BView::_SetViewBitmap(const BBitmap* bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	if (!_CheckOwnerLockAndSwitchCurrent())
		return B_ERROR;

	int32 serverToken = bitmap ? bitmap->_ServerToken() : -1;

	fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_BITMAP);
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
BView::_CheckOwnerLockAndSwitchCurrent() const
{
	STRACE(("BView(%s)::_CheckOwnerLockAndSwitchCurrent()\n", Name()));

	if (fOwner == NULL) {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	_CheckLockAndSwitchCurrent();

	return true;
}


bool
BView::_CheckOwnerLock() const
{
	if (fOwner) {
		fOwner->check_lock();
		return true;
	} else {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}


void
BView::_CheckLockAndSwitchCurrent() const
{
	STRACE(("BView(%s)::_CheckLockAndSwitchCurrent()\n", Name()));

	if (!fOwner)
		return;

	fOwner->check_lock();

	_SwitchServerCurrentView();
}


void
BView::_CheckLock() const
{
	if (fOwner)
		fOwner->check_lock();
}


void
BView::_SwitchServerCurrentView() const
{
	int32 serverToken = _get_object_token_(this);

	if (fOwner->fLastViewToken != serverToken) {
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		fOwner->fLink->StartMessage(AS_SET_CURRENT_VIEW);
		fOwner->fLink->Attach<int32>(serverToken);

		fOwner->fLastViewToken = serverToken;
	}
}


#if __GNUC__ == 2


extern "C" void
_ReservedView1__5BView(BView* view, BRect rect)
{
	view->BView::DrawAfterChildren(rect);
}


extern "C" void
_ReservedView2__5BView(BView* view)
{
	// MinSize()
	perform_data_min_size data;
	view->Perform(PERFORM_CODE_MIN_SIZE, &data);
}


extern "C" void
_ReservedView3__5BView(BView* view)
{
	// MaxSize()
	perform_data_max_size data;
	view->Perform(PERFORM_CODE_MAX_SIZE, &data);
}


extern "C" BSize
_ReservedView4__5BView(BView* view)
{
	// PreferredSize()
	perform_data_preferred_size data;
	view->Perform(PERFORM_CODE_PREFERRED_SIZE, &data);
	return data.return_value;
}


extern "C" BAlignment
_ReservedView5__5BView(BView* view)
{
	// LayoutAlignment()
	perform_data_layout_alignment data;
	view->Perform(PERFORM_CODE_LAYOUT_ALIGNMENT, &data);
	return data.return_value;
}


extern "C" bool
_ReservedView6__5BView(BView* view)
{
	// HasHeightForWidth()
	perform_data_has_height_for_width data;
	view->Perform(PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH, &data);
	return data.return_value;
}


extern "C" void
_ReservedView7__5BView(BView* view, float width, float* min, float* max,
	float* preferred)
{
	// GetHeightForWidth()
	perform_data_get_height_for_width data;
	data.width = width;
	view->Perform(PERFORM_CODE_GET_HEIGHT_FOR_WIDTH, &data);
	if (min != NULL)
		*min = data.min;
	if (max != NULL)
		*max = data.max;
	if (preferred != NULL)
		*preferred = data.preferred;
}


extern "C" void
_ReservedView8__5BView(BView* view, BLayout* layout)
{
	// SetLayout()
	perform_data_set_layout data;
	data.layout = layout;
	view->Perform(PERFORM_CODE_SET_LAYOUT, &data);
}


extern "C" void
_ReservedView9__5BView(BView* view, bool descendants)
{
	// LayoutInvalidated()
	perform_data_layout_invalidated data;
	data.descendants = descendants;
	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}


extern "C" void
_ReservedView10__5BView(BView* view)
{
	// DoLayout()
	view->Perform(PERFORM_CODE_DO_LAYOUT, NULL);
}


#endif	// __GNUC__ == 2


extern "C" bool
B_IF_GCC_2(_ReservedView11__5BView, _ZN5BView15_ReservedView11Ev)(
	BView* view, BPoint point, BToolTip** _toolTip)
{
	// GetToolTipAt()
	perform_data_get_tool_tip_at data;
	data.point = point;
	data.tool_tip = _toolTip;
	view->Perform(PERFORM_CODE_GET_TOOL_TIP_AT, &data);
	return data.return_value;
}


extern "C" void
B_IF_GCC_2(_ReservedView12__5BView, _ZN5BView15_ReservedView12Ev)(
	BView* view)
{
	// LayoutChanged();
	view->Perform(PERFORM_CODE_LAYOUT_CHANGED, NULL);
}


void BView::_ReservedView13() {}
void BView::_ReservedView14() {}
void BView::_ReservedView15() {}
void BView::_ReservedView16() {}


BView::BView(const BView& other)
	:
	BHandler()
{
	// this is private and not functional, but exported
}


BView&
BView::operator=(const BView& other)
{
	// this is private and not functional, but exported
	return *this;
}


void
BView::_PrintToStream()
{
	printf("BView::_PrintToStream()\n");
	printf("\tName: %s\n"
		"\tParent: %s\n"
		"\tFirstChild: %s\n"
		"\tNextSibling: %s\n"
		"\tPrevSibling: %s\n"
		"\tOwner(Window): %s\n"
		"\tToken: %ld\n"
		"\tFlags: %ld\n"
		"\tView origin: (%f,%f)\n"
		"\tView Bounds rectangle: (%f,%f,%f,%f)\n"
		"\tShow level: %d\n"
		"\tTopView?: %s\n"
		"\tBPicture: %s\n"
		"\tVertical Scrollbar %s\n"
		"\tHorizontal Scrollbar %s\n"
		"\tIs Printing?: %s\n"
		"\tShelf?: %s\n"
		"\tEventMask: %ld\n"
		"\tEventOptions: %ld\n",
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
	fCurrentPicture? "YES" : "NULL",
	fVerScroller? "YES" : "NULL",
	fHorScroller? "YES" : "NULL",
	fIsPrinting? "YES" : "NO",
	fShelf? "YES" : "NO",
	fEventMask,
	fEventOptions);

	printf("\tState status:\n"
		"\t\tLocalCoordianteSystem: (%f,%f)\n"
		"\t\tPenLocation: (%f,%f)\n"
		"\t\tPenSize: %f\n"
		"\t\tHighColor: [%d,%d,%d,%d]\n"
		"\t\tLowColor: [%d,%d,%d,%d]\n"
		"\t\tViewColor: [%d,%d,%d,%d]\n"
		"\t\tPattern: %llx\n"
		"\t\tDrawingMode: %d\n"
		"\t\tLineJoinMode: %d\n"
		"\t\tLineCapMode: %d\n"
		"\t\tMiterLimit: %f\n"
		"\t\tAlphaSource: %d\n"
		"\t\tAlphaFuntion: %d\n"
		"\t\tScale: %f\n"
		"\t\t(Print)FontAliasing: %s\n"
		"\t\tFont Info:\n",
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
BView::_PrintTree()
{
	int32 spaces = 2;
	BView* c = fFirstChild; //c = short for: current
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


// #pragma mark -


BLayoutItem*
BView::Private::LayoutItemAt(int32 index)
{
	return fView->fLayoutData->fLayoutItems.ItemAt(index);
}


int32
BView::Private::CountLayoutItems()
{
	return fView->fLayoutData->fLayoutItems.CountItems();
}


void
BView::Private::RegisterLayoutItem(BLayoutItem* item)
{
	fView->fLayoutData->fLayoutItems.AddItem(item);
}


void
BView::Private::DeregisterLayoutItem(BLayoutItem* item)
{
	fView->fLayoutData->fLayoutItems.RemoveItem(item);
}


bool
BView::Private::MinMaxValid()
{
	return fView->fLayoutData->fMinMaxValid;
}


bool
BView::Private::WillLayout()
{
	BView::LayoutData* data = fView->fLayoutData;
	if (data->fLayoutInProgress)
		return false;
	if (data->fNeedsRelayout || !data->fLayoutValid || !data->fMinMaxValid)
		return true;
	return false;
}
