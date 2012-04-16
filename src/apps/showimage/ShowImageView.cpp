/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 *		yellowTAB GmbH
 *		Bernd Korz
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ShowImageView.h"

#include <math.h>
#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>

#include <tracker_private.h>

#include "ImageCache.h"
#include "ShowImageApp.h"
#include "ShowImageWindow.h"


using std::nothrow;


class PopUpMenu : public BPopUpMenu {
	public:
		PopUpMenu(const char* name, BMessenger target);
		virtual ~PopUpMenu();

	private:
		BMessenger fTarget;
};


// the delay time for hiding the cursor in 1/10 seconds (the pulse rate)
#define HIDE_CURSOR_DELAY_TIME 20
#define STICKY_ZOOM_DELAY_TIME 5
#define SHOW_IMAGE_ORIENTATION_ATTRIBUTE "ShowImage:orientation"


const rgb_color kBorderColor = { 0, 0, 0, 255 };

enum ShowImageView::image_orientation
ShowImageView::fTransformation[ImageProcessor::kNumberOfAffineTransformations]
		[kNumberOfOrientations] = {
	// rotate 90°
	{k90, k180, k270, k0, k270V, k0V, k90V, k0H},
	// rotate -90°
	{k270, k0, k90, k180, k90V, k0H, k270V, k0V},
	// mirror vertical
	{k0H, k270V, k0V, k90V, k180, k270, k0, k90},
	// mirror horizontal
	{k0V, k90V, k0H, k270V, k0, k90, k180, k270}
};

const rgb_color kAlphaLow = (rgb_color){ 0xbb, 0xbb, 0xbb, 0xff };
const rgb_color kAlphaHigh = (rgb_color){ 0xe0, 0xe0, 0xe0, 0xff };

const uint32 kMsgPopUpMenuClosed = 'pmcl';


inline void
blend_colors(uint8* d, uint8 r, uint8 g, uint8 b, uint8 a)
{
	d[0] = ((b - d[0]) * a + (d[0] << 8)) >> 8;
	d[1] = ((g - d[1]) * a + (d[1] << 8)) >> 8;
	d[2] = ((r - d[2]) * a + (d[2] << 8)) >> 8;
}


BBitmap*
compose_checker_background(const BBitmap* bitmap)
{
	BBitmap* result = new (nothrow) BBitmap(bitmap);
	if (result && !result->IsValid()) {
		delete result;
		result = NULL;
	}
	if (!result)
		return NULL;

	uint8* bits = (uint8*)result->Bits();
	uint32 bpr = result->BytesPerRow();
	uint32 width = result->Bounds().IntegerWidth() + 1;
	uint32 height = result->Bounds().IntegerHeight() + 1;

	for (uint32 i = 0; i < height; i++) {
		uint8* p = bits;
		for (uint32 x = 0; x < width; x++) {
			uint8 alpha = p[3];
			if (alpha < 255) {
				p[3] = 255;
				alpha = 255 - alpha;
				if (x % 10 >= 5) {
					if (i % 10 >= 5) {
						blend_colors(p, kAlphaLow.red, kAlphaLow.green, kAlphaLow.blue, alpha);
					} else {
						blend_colors(p, kAlphaHigh.red, kAlphaHigh.green, kAlphaHigh.blue, alpha);
					}
				} else {
					if (i % 10 >= 5) {
						blend_colors(p, kAlphaHigh.red, kAlphaHigh.green, kAlphaHigh.blue, alpha);
					} else {
						blend_colors(p, kAlphaLow.red, kAlphaLow.green, kAlphaLow.blue, alpha);
					}
				}
			}
			p += 4;
		}
		bits += bpr;
	}
	return result;
}


//	#pragma mark -


PopUpMenu::PopUpMenu(const char* name, BMessenger target)
	:
	BPopUpMenu(name, false, false),
	fTarget(target)
{
	SetAsyncAutoDestruct(true);
}


PopUpMenu::~PopUpMenu()
{
	fTarget.SendMessage(kMsgPopUpMenuClosed);
}


//	#pragma mark -


ShowImageView::ShowImageView(BRect rect, const char *name, uint32 resizingMode,
		uint32 flags)
	:
	BView(rect, name, resizingMode, flags),
	fBitmapOwner(NULL),
	fBitmap(NULL),
	fDisplayBitmap(NULL),
	fSelectionBitmap(NULL),

	fZoom(1.0),

	fScaleBilinear(true),

	fBitmapLocationInView(0.0, 0.0),

	fStretchToBounds(false),
	fHideCursor(false),
	fScrollingBitmap(false),
	fCreatingSelection(false),
	fFirstPoint(0.0, 0.0),
	fSelectionMode(false),
	fAnimateSelection(true),
	fHasSelection(false),
	fShowCaption(false),
	fShowingPopUpMenu(false),
	fHideCursorCountDown(HIDE_CURSOR_DELAY_TIME),
	fStickyZoomCountDown(0),
	fIsActiveWin(true),
	fDefaultCursor(NULL),
	fGrabCursor(NULL)
{
	ShowImageSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		fStretchToBounds = settings->GetBool("StretchToBounds",
			fStretchToBounds);
		fScaleBilinear = settings->GetBool("ScaleBilinear", fScaleBilinear);
		settings->Unlock();
	}

	fDefaultCursor = new BCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
	fGrabCursor = new BCursor(B_CURSOR_ID_GRABBING);

	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(kBorderColor);
	SetLowColor(0, 0, 0);
}


ShowImageView::~ShowImageView()
{
	_DeleteBitmap();

	delete fDefaultCursor;
	delete fGrabCursor;
}


void
ShowImageView::_AnimateSelection(bool enabled)
{
	fAnimateSelection = enabled;
}


void
ShowImageView::Pulse()
{
	// animate marching ants
	if (fHasSelection && fAnimateSelection && fIsActiveWin) {
		fSelectionBox.Animate();
		fSelectionBox.Draw(this, Bounds());
	}

	if (fHideCursor && !fHasSelection && !fShowingPopUpMenu && fIsActiveWin) {
		if (fHideCursorCountDown <= 0) {
			BPoint mousePos;
			uint32 buttons;
			GetMouse(&mousePos, &buttons, false);
			if (Bounds().Contains(mousePos)) {
				be_app->ObscureCursor();
			}
		} else
			fHideCursorCountDown--;
	}

	if (fStickyZoomCountDown > 0)
		fStickyZoomCountDown--;

}


void
ShowImageView::_SendMessageToWindow(BMessage *message)
{
	BMessenger target(Window());
	target.SendMessage(message);
}


void
ShowImageView::_SendMessageToWindow(uint32 code)
{
	BMessage message(code);
	_SendMessageToWindow(&message);
}


//! send message to parent about new image
void
ShowImageView::_Notify()
{
	BMessage msg(MSG_UPDATE_STATUS);

	msg.AddInt32("width", fBitmap->Bounds().IntegerWidth() + 1);
	msg.AddInt32("height", fBitmap->Bounds().IntegerHeight() + 1);

	msg.AddInt32("colors", fBitmap->ColorSpace());
	_SendMessageToWindow(&msg);

	FixupScrollBars();
	Invalidate();
}


void
ShowImageView::_UpdateStatusText()
{
	BMessage msg(MSG_UPDATE_STATUS_TEXT);

	if (fHasSelection) {
		char size[50];
		snprintf(size, sizeof(size), "(%.0fx%.0f)",
			fSelectionBox.Bounds().Width() + 1.0,
			fSelectionBox.Bounds().Height() + 1.0);

		msg.AddString("status", size);
	}

	_SendMessageToWindow(&msg);
}


void
ShowImageView::_DeleteBitmap()
{
	_DeleteSelectionBitmap();

	if (fDisplayBitmap != fBitmap)
		delete fDisplayBitmap;
	fDisplayBitmap = NULL;

	if (fBitmapOwner != NULL)
		fBitmapOwner->ReleaseReference();
	else
		delete fBitmap;

	fBitmapOwner = NULL;
	fBitmap = NULL;
}


void
ShowImageView::_DeleteSelectionBitmap()
{
	delete fSelectionBitmap;
	fSelectionBitmap = NULL;
}


status_t
ShowImageView::SetImage(const BMessage* message)
{
	BBitmap* bitmap;
	entry_ref ref;
	if (message->FindPointer("bitmap", (void**)&bitmap) != B_OK
		|| message->FindRef("ref", &ref) != B_OK || bitmap == NULL)
		return B_ERROR;

	status_t status = SetImage(&ref, bitmap);
	if (status == B_OK) {
		fFormatDescription = message->FindString("type");
		fMimeType = message->FindString("mime");

		message->FindPointer("bitmapOwner", (void**)&fBitmapOwner);
	}

	return status;
}


status_t
ShowImageView::SetImage(const entry_ref* ref, BBitmap* bitmap)
{
	// Delete the old one, and clear everything
	fUndo.Clear();
	_SetHasSelection(false);
	fCreatingSelection = false;
	_DeleteBitmap();

	fBitmap = bitmap;
	if (ref == NULL)
		fCurrentRef.device = -1;
	else
		fCurrentRef = *ref;

	if (fBitmap != NULL) {
		// prepare the display bitmap
		if (fBitmap->ColorSpace() == B_RGBA32)
			fDisplayBitmap = compose_checker_background(fBitmap);
		if (fDisplayBitmap == NULL)
			fDisplayBitmap = fBitmap;

		BNode node(ref);

		// restore orientation
		int32 orientation;
		fImageOrientation = k0;
		if (node.ReadAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE, B_INT32_TYPE, 0,
				&orientation, sizeof(orientation)) == sizeof(orientation)) {
			orientation &= 255;
			switch (orientation) {
				case k0:
					break;
				case k90:
					_DoImageOperation(ImageProcessor::kRotateClockwise, true);
					break;
				case k180:
					_DoImageOperation(ImageProcessor::kRotateClockwise, true);
					_DoImageOperation(ImageProcessor::kRotateClockwise, true);
					break;
				case k270:
					_DoImageOperation(ImageProcessor::kRotateCounterClockwise, true);
					break;
				case k0V:
					_DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
					break;
				case k90V:
					_DoImageOperation(ImageProcessor::kRotateClockwise, true);
					_DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
					break;
				case k0H:
					_DoImageOperation(ImageProcessor::ImageProcessor::kFlipLeftToRight, true);
					break;
				case k270V:
					_DoImageOperation(ImageProcessor::kRotateCounterClockwise, true);
					_DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
					break;
			}
		}
	}

	BPath path(ref);
	fCaption = path.Path();
	fFormatDescription = "Bitmap";
	fMimeType = "image/x-be-bitmap";

	be_roster->AddToRecentDocuments(ref, kApplicationSignature);

	FitToBounds();
	_Notify();
	return B_OK;
}


BPoint
ShowImageView::ImageToView(BPoint p) const
{
	p.x = floorf(fZoom * p.x + fBitmapLocationInView.x);
	p.y = floorf(fZoom * p.y + fBitmapLocationInView.y);
	return p;
}


BPoint
ShowImageView::ViewToImage(BPoint p) const
{
	p.x = floorf((p.x - fBitmapLocationInView.x) / fZoom);
	p.y = floorf((p.y - fBitmapLocationInView.y) / fZoom);
	return p;
}


BRect
ShowImageView::ImageToView(BRect r) const
{
	BPoint leftTop(ImageToView(BPoint(r.left, r.top)));
	BPoint rightBottom(r.right, r.bottom);
	rightBottom += BPoint(1, 1);
	rightBottom = ImageToView(rightBottom);
	rightBottom -= BPoint(1, 1);
	return BRect(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
}


void
ShowImageView::ConstrainToImage(BPoint& point) const
{
	point.ConstrainTo(fBitmap->Bounds());
}


void
ShowImageView::ConstrainToImage(BRect& rect) const
{
	rect = rect & fBitmap->Bounds();
}


void
ShowImageView::SetShowCaption(bool show)
{
	if (fShowCaption != show) {
		fShowCaption = show;
		_UpdateCaption();
	}
}


void
ShowImageView::SetStretchToBounds(bool enable)
{
	if (fStretchToBounds != enable) {
		_SettingsSetBool("StretchToBounds", enable);
		fStretchToBounds = enable;
		if (enable || fZoom > 1.0)
			FitToBounds();
	}
}


void
ShowImageView::SetHideIdlingCursor(bool hide)
{
	fHideCursor = hide;
}


BBitmap*
ShowImageView::Bitmap()
{
	return fBitmap;
}


void
ShowImageView::SetScaleBilinear(bool enabled)
{
	if (fScaleBilinear != enabled) {
		_SettingsSetBool("ScaleBilinear", enabled);
		fScaleBilinear = enabled;
		Invalidate();
	}
}


void
ShowImageView::AttachedToWindow()
{
	FitToBounds();
	fUndo.SetWindow(Window());
	FixupScrollBars();
}


void
ShowImageView::FrameResized(float width, float height)
{
	FixupScrollBars();
}


float
ShowImageView::_FitToBoundsZoom() const
{
	if (fBitmap == NULL)
		return 1.0f;

	// the width/height of the bitmap (in pixels)
	float bitmapWidth = fBitmap->Bounds().Width() + 1;
	float bitmapHeight = fBitmap->Bounds().Height() + 1;

	// the available width/height for layouting the bitmap (in pixels)
	float width = Bounds().Width() + 1;
	float height = Bounds().Height() + 1;

	float zoom = width / bitmapWidth;

	if (zoom * bitmapHeight <= height)
		return zoom;

	return height / bitmapHeight;
}


BRect
ShowImageView::_AlignBitmap()
{
	BRect rect(fBitmap->Bounds());

	// the width/height of the bitmap (in pixels)
	float bitmapWidth = rect.Width() + 1;
	float bitmapHeight = rect.Height() + 1;

	// the available width/height for layouting the bitmap (in pixels)
	float width = Bounds().Width() + 1;
	float height = Bounds().Height() + 1;

	if (width == 0 || height == 0)
		return rect;

	// zoom image
	rect.right = floorf(bitmapWidth * fZoom) - 1;
	rect.bottom = floorf(bitmapHeight * fZoom) - 1;

	// update the bitmap size after the zoom
	bitmapWidth = rect.Width() + 1.0;
	bitmapHeight = rect.Height() + 1.0;

	// always align in the center if the bitmap is smaller than the window
	if (width > bitmapWidth)
		rect.OffsetBy(floorf((width - bitmapWidth) / 2.0), 0);

	if (height > bitmapHeight)
		rect.OffsetBy(0, floorf((height - bitmapHeight) / 2.0));

	return rect;
}


void
ShowImageView::_DrawBackground(BRect border)
{
	BRect bounds(Bounds());
	// top
	FillRect(BRect(0, 0, bounds.right, border.top-1), B_SOLID_LOW);
	// left
	FillRect(BRect(0, border.top, border.left-1, border.bottom), B_SOLID_LOW);
	// right
	FillRect(BRect(border.right+1, border.top, bounds.right, border.bottom), B_SOLID_LOW);
	// bottom
	FillRect(BRect(0, border.bottom+1, bounds.right, bounds.bottom), B_SOLID_LOW);
}


void
ShowImageView::_LayoutCaption(BFont &font, BPoint &pos, BRect &rect)
{
	font_height fontHeight;
	float width, height;
	BRect bounds(Bounds());
	font = be_plain_font;
	width = font.StringWidth(fCaption.String());
	font.GetHeight(&fontHeight);
	height = fontHeight.ascent + fontHeight.descent;
	// center text horizontally
	pos.x = (bounds.left + bounds.right - width) / 2;
	// flush bottom
	pos.y = bounds.bottom - fontHeight.descent - 7;

	// background rectangle
	rect.Set(0, 0, width + 4, height + 4);
	rect.OffsetTo(pos);
	rect.OffsetBy(-2, -2 - fontHeight.ascent); // -2 for border
}


void
ShowImageView::_DrawCaption()
{
	BFont font;
	BPoint position;
	BRect rect;
	_LayoutCaption(font, position, rect);

	PushState();

	// draw background
	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(255, 255, 255, 160);
	FillRect(rect);

	// draw text
	SetDrawingMode(B_OP_OVER);
	SetFont(&font);
	SetLowColor(B_TRANSPARENT_COLOR);
	SetHighColor(0, 0, 0);
	DrawString(fCaption.String(), position);

	PopState();
}


void
ShowImageView::_UpdateCaption()
{
	BFont font;
	BPoint pos;
	BRect rect;
	_LayoutCaption(font, pos, rect);

	// draw over portion of image where caption is located
	BRegion clip(rect);
	PushState();
	ConstrainClippingRegion(&clip);
	Draw(rect);
	PopState();
}


void
ShowImageView::_DrawImage(BRect rect)
{
	// TODO: fix composing of fBitmap with other bitmaps
	// with regard to alpha channel
	if (!fDisplayBitmap)
		fDisplayBitmap = fBitmap;

	uint32 options = fScaleBilinear ? B_FILTER_BITMAP_BILINEAR : 0;
	DrawBitmap(fDisplayBitmap, fDisplayBitmap->Bounds(), rect, options);
}


void
ShowImageView::Draw(BRect updateRect)
{
	if (fBitmap == NULL)
		return;

	if (IsPrinting()) {
		DrawBitmap(fBitmap);
		return;
	}

	BRect rect = _AlignBitmap();
	fBitmapLocationInView.x = floorf(rect.left);
	fBitmapLocationInView.y = floorf(rect.top);

	_DrawBackground(rect);
	_DrawImage(rect);

	if (fShowCaption)
		_DrawCaption();

	if (fHasSelection) {
		if (fSelectionBitmap != NULL) {
			BRect srcRect;
			BRect dstRect;
			_GetSelectionMergeRects(srcRect, dstRect);
			dstRect = ImageToView(dstRect);
			DrawBitmap(fSelectionBitmap, srcRect, dstRect);
		}
		fSelectionBox.Draw(this, updateRect);
	}
}


BBitmap*
ShowImageView::_CopySelection(uchar alpha, bool imageSize)
{
	bool hasAlpha = alpha != 255;

	if (!fHasSelection)
		return NULL;

	BRect rect = fSelectionBox.Bounds().OffsetToCopy(B_ORIGIN);
	if (!imageSize) {
		// scale image to view size
		rect.right = floorf((rect.right + 1.0) * fZoom - 1.0);
		rect.bottom = floorf((rect.bottom + 1.0) * fZoom - 1.0);
	}
	BView view(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap* bitmap = new(nothrow) BBitmap(rect, hasAlpha ? B_RGBA32
		: fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
#ifdef __HAIKU__
		// On Haiku, B_OP_SUBSTRACT does not affect alpha like it did on BeOS.
		// Don't know if it's better to fix it or not (stippi).
		if (hasAlpha) {
			view.SetHighColor(0, 0, 0, 0);
			view.FillRect(view.Bounds());
			view.SetDrawingMode(B_OP_ALPHA);
			view.SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			view.SetHighColor(0, 0, 0, alpha);
		}
		if (fSelectionBitmap) {
			view.DrawBitmap(fSelectionBitmap,
				fSelectionBitmap->Bounds().OffsetToCopy(B_ORIGIN), rect);
		} else
			view.DrawBitmap(fBitmap, fCopyFromRect, rect);
#else
		if (fSelectionBitmap) {
			view.DrawBitmap(fSelectionBitmap,
				fSelectionBitmap->Bounds().OffsetToCopy(B_ORIGIN), rect);
		} else
			view.DrawBitmap(fBitmap, fCopyFromRect, rect);
		if (hasAlpha) {
			view.SetDrawingMode(B_OP_SUBTRACT);
			view.SetHighColor(0, 0, 0, 255 - alpha);
			view.FillRect(rect, B_SOLID_HIGH);
		}
#endif
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
	}

	return bitmap;
}


bool
ShowImageView::_AddSupportedTypes(BMessage* msg, BBitmap* bitmap)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	// add the current image mime first, will make it the preferred format on
	// left mouse drag
	msg->AddString("be:types", fMimeType);
	msg->AddString("be:filetypes", fMimeType);
	msg->AddString("be:type_descriptions", fFormatDescription);

	bool foundOther = false;
	bool foundCurrent = false;

	int32 infoCount;
	translator_info* info;
	BBitmapStream stream(bitmap);
	if (roster->GetTranslators(&stream, NULL, &info, &infoCount) == B_OK) {
		for (int32 i = 0; i < infoCount; i++) {
			const translation_format* formats;
			int32 count;
			roster->GetOutputFormats(info[i].translator, &formats, &count);
			for (int32 j = 0; j < count; j++) {
				if (fMimeType == formats[j].MIME) {
					foundCurrent = true;
				} else if (strcmp(formats[j].MIME, "image/x-be-bitmap") != 0) {
					foundOther = true;
					// needed to send data in message
					msg->AddString("be:types", formats[j].MIME);
					// needed to pass data via file
					msg->AddString("be:filetypes", formats[j].MIME);
					msg->AddString("be:type_descriptions", formats[j].name);
				}
			}
		}
	}
	stream.DetachBitmap(&bitmap);

	if (!foundCurrent) {
		msg->RemoveData("be:types", 0);
		msg->RemoveData("be:filetypes", 0);
		msg->RemoveData("be:type_descriptions", 0);
	}

	return foundOther || foundCurrent;
}


void
ShowImageView::_BeginDrag(BPoint sourcePoint)
{
	BBitmap* bitmap = _CopySelection(128, false);
	if (bitmap == NULL)
		return;

	SetMouseEventMask(B_POINTER_EVENTS);

	// fill the drag message
	BMessage drag(B_SIMPLE_DATA);
	drag.AddInt32("be:actions", B_COPY_TARGET);
	drag.AddString("be:clip_name", "Bitmap Clip");
	// ShowImage specific fields
	drag.AddPoint("be:_source_point", sourcePoint);
	drag.AddRect("be:_frame", fSelectionBox.Bounds());
	if (_AddSupportedTypes(&drag, bitmap)) {
		// we also support "Passing Data via File" protocol
		drag.AddString("be:types", B_FILE_MIME_TYPE);
		// avoid flickering of dragged bitmap caused by drawing into the window
		_AnimateSelection(false);
		// only use a transparent bitmap on selections less than 400x400
		// (taking into account zooming)
		BRect selectionRect = fSelectionBox.Bounds();
		if (selectionRect.Width() * fZoom < 400.0
			&& selectionRect.Height() * fZoom < 400.0) {
			sourcePoint -= selectionRect.LeftTop();
			sourcePoint.x *= fZoom;
			sourcePoint.y *= fZoom;
			// DragMessage takes ownership of bitmap
			DragMessage(&drag, bitmap, B_OP_ALPHA, sourcePoint);
			bitmap = NULL;
		} else {
			delete bitmap;
			// Offset and scale the rect
			BRect rect(selectionRect);
			rect = ImageToView(rect);
			rect.InsetBy(-1, -1);
			DragMessage(&drag, rect);
		}
	}
}


bool
ShowImageView::_OutputFormatForType(BBitmap* bitmap, const char* type,
	translation_format* format)
{
	bool found = false;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	BBitmapStream stream(bitmap);

	translator_info* outInfo;
	int32 outNumInfo;
	if (roster->GetTranslators(&stream, NULL, &outInfo, &outNumInfo) == B_OK) {
		for (int32 i = 0; i < outNumInfo; i++) {
			const translation_format* formats;
			int32 formatCount;
			roster->GetOutputFormats(outInfo[i].translator, &formats,
				&formatCount);
			for (int32 j = 0; j < formatCount; j++) {
				if (strcmp(formats[j].MIME, type) == 0) {
					*format = formats[j];
					found = true;
					break;
				}
			}
		}
	}
	stream.DetachBitmap(&bitmap);
	return found;
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SaveToFile"


void
ShowImageView::SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap,
	const translation_format* format)
{
	if (bitmap == NULL) {
		// If no bitmap is supplied, write out the whole image
		bitmap = fBitmap;
	}

	BBitmapStream stream(bitmap);

	bool loop = true;
	while (loop) {
		BTranslatorRoster *roster = BTranslatorRoster::Default();
		if (!roster)
			break;
		// write data
		BFile file(dir, name, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() != B_OK)
			break;
		if (roster->Translate(&stream, NULL, NULL, &file, format->type) < B_OK)
			break;
		// set mime type
		BNodeInfo info(&file);
		if (info.InitCheck() == B_OK)
			info.SetType(format->MIME);

		loop = false;
			// break out of loop gracefully (indicates no errors)
	}
	if (loop) {
		// If loop terminated because of a break, there was an error
		char buffer[512];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("The file '%s' could not "
			"be written."), name);
		BAlert *palert = new BAlert("", buffer, B_TRANSLATE("OK"));
		palert->Go();
	}

	stream.DetachBitmap(&bitmap);
		// Don't allow the bitmap to be deleted, this is
		// especially important when using fBitmap as the bitmap
}


void
ShowImageView::_SendInMessage(BMessage* msg, BBitmap* bitmap, translation_format* format)
{
	BMessage reply(B_MIME_DATA);
	BBitmapStream stream(bitmap); // destructor deletes bitmap
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BMallocIO memStream;
	if (roster->Translate(&stream, NULL, NULL, &memStream, format->type) == B_OK) {
		reply.AddData(format->MIME, B_MIME_TYPE, memStream.Buffer(), memStream.BufferLength());
		msg->SendReply(&reply);
	}
}


void
ShowImageView::_HandleDrop(BMessage* msg)
{
	entry_ref dirRef;
	BString name, type;
	bool saveToFile = msg->FindString("be:filetypes", &type) == B_OK
		&& msg->FindRef("directory", &dirRef) == B_OK
		&& msg->FindString("name", &name) == B_OK;

	bool sendInMessage = !saveToFile
		&& msg->FindString("be:types", &type) == B_OK;

	BBitmap* bitmap = _CopySelection();
	if (bitmap == NULL)
		return;

	translation_format format;
	if (!_OutputFormatForType(bitmap, type.String(), &format)) {
		delete bitmap;
		return;
	}

	if (saveToFile) {
		BDirectory dir(&dirRef);
		SaveToFile(&dir, name.String(), bitmap, &format);
		delete bitmap;
	} else if (sendInMessage) {
		_SendInMessage(msg, bitmap, &format);
	} else {
		delete bitmap;
	}
}


void
ShowImageView::_ScrollBitmap(BPoint point)
{
	point = ConvertToScreen(point);
	BPoint delta = fFirstPoint - point;
	fFirstPoint = point;
	_ScrollRestrictedBy(delta.x, delta.y);
}


void
ShowImageView::_GetMergeRects(BBitmap* merge, BRect selection, BRect& srcRect,
	BRect& dstRect)
{
	// Constrain dstRect to target image size and apply the same edge offsets
	// to the srcRect.

	dstRect = selection;

	BRect clippedDstRect(dstRect);
	ConstrainToImage(clippedDstRect);

	srcRect = merge->Bounds().OffsetToCopy(B_ORIGIN);

	srcRect.left += clippedDstRect.left - dstRect.left;
	srcRect.top += clippedDstRect.top - dstRect.top;
	srcRect.right += clippedDstRect.right - dstRect.right;
	srcRect.bottom += clippedDstRect.bottom - dstRect.bottom;

	dstRect = clippedDstRect;
}


void
ShowImageView::_GetSelectionMergeRects(BRect& srcRect, BRect& dstRect)
{
	_GetMergeRects(fSelectionBitmap, fSelectionBox.Bounds(), srcRect, dstRect);
}


void
ShowImageView::_MergeWithBitmap(BBitmap* merge, BRect selection)
{
	BView view(fBitmap->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap* bitmap = new(nothrow) BBitmap(fBitmap->Bounds(),
		fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		view.DrawBitmap(fBitmap, fBitmap->Bounds());
		BRect srcRect;
		BRect dstRect;
		_GetMergeRects(merge, selection, srcRect, dstRect);
		view.DrawBitmap(merge, srcRect, dstRect);

		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();

		_DeleteBitmap();
		fBitmap = bitmap;

		_SendMessageToWindow(MSG_MODIFIED);
	} else
		delete bitmap;
}


void
ShowImageView::MouseDown(BPoint position)
{
	MakeFocus(true);

	BPoint point = ViewToImage(position);
	int32 clickCount = 0;
	uint32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL) {
		clickCount = Window()->CurrentMessage()->FindInt32("clicks");
		buttons = Window()->CurrentMessage()->FindInt32("buttons");
	}

	// Using clickCount >= 2 and the modulo 2 accounts for quickly repeated
	// double-clicks
	if (buttons == B_PRIMARY_MOUSE_BUTTON && clickCount >= 2 && 
			clickCount % 2 == 0) {
		Window()->PostMessage(MSG_FULL_SCREEN);
		return;
	}

	if (fHasSelection && fSelectionBox.Bounds().Contains(point)
		&& (buttons
				& (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON)) != 0) {
		if (!fSelectionBitmap)
			fSelectionBitmap = _CopySelection();

		_BeginDrag(point);
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON
			&& (fSelectionMode
				|| (modifiers() & (B_COMMAND_KEY | B_CONTROL_KEY)) != 0)) {
		// begin new selection
		_SetHasSelection(true);
		fCreatingSelection = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		ConstrainToImage(point);
		fFirstPoint = point;
		fCopyFromRect.Set(point.x, point.y, point.x, point.y);
		fSelectionBox.SetBounds(this, fCopyFromRect);
		Invalidate();
	} else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		_ShowPopUpMenu(ConvertToScreen(position));
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON
		|| buttons == B_TERTIARY_MOUSE_BUTTON) {
		// move image in window
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		fScrollingBitmap = true;
		fFirstPoint = ConvertToScreen(position);
		be_app->SetCursor(fGrabCursor);
	}
}


void
ShowImageView::_UpdateSelectionRect(BPoint point, bool final)
{
	BRect oldSelection = fCopyFromRect;
	point = ViewToImage(point);
	ConstrainToImage(point);
	fCopyFromRect.left = min_c(fFirstPoint.x, point.x);
	fCopyFromRect.right = max_c(fFirstPoint.x, point.x);
	fCopyFromRect.top = min_c(fFirstPoint.y, point.y);
	fCopyFromRect.bottom = max_c(fFirstPoint.y, point.y);
	fSelectionBox.SetBounds(this, fCopyFromRect);

	if (final) {
		// selection must be at least 2 pixels wide or 2 pixels tall
		if (fCopyFromRect.Width() < 1.0 && fCopyFromRect.Height() < 1.0)
			_SetHasSelection(false);
	} else
		_UpdateStatusText();

	if (oldSelection != fCopyFromRect || !fHasSelection) {
		BRect updateRect;
		updateRect = oldSelection | fCopyFromRect;
		updateRect = ImageToView(updateRect);
		updateRect.InsetBy(-1, -1);
		Invalidate(updateRect);
	}
}


void
ShowImageView::MouseMoved(BPoint point, uint32 state, const BMessage* message)
{
	fHideCursorCountDown = HIDE_CURSOR_DELAY_TIME;
	if (fHideCursor) {
		// Show toolbar when mouse hits top 15 pixels, hide otherwise
		_ShowToolBarIfEnabled(point.y <= 15);
	}
	if (fCreatingSelection)
		_UpdateSelectionRect(point, false);
	else if (fScrollingBitmap)
		_ScrollBitmap(point);
}


void
ShowImageView::MouseUp(BPoint point)
{
	if (fCreatingSelection) {
		_UpdateSelectionRect(point, true);
		fCreatingSelection = false;
	} else if (fScrollingBitmap) {
		_ScrollBitmap(point);
		fScrollingBitmap = false;
		be_app->SetCursor(fDefaultCursor);
	}
	_AnimateSelection(true);
}


float
ShowImageView::_LimitToRange(float v, orientation o, bool absolute)
{
	BScrollBar* psb = ScrollBar(o);
	if (psb) {
		float min, max, pos;
		pos = v;
		if (!absolute)
			pos += psb->Value();

		psb->GetRange(&min, &max);
		if (pos < min)
			pos = min;
		else if (pos > max)
			pos = max;

		v = pos;
		if (!absolute)
			v -= psb->Value();
	}
	return v;
}


void
ShowImageView::_ScrollRestricted(float x, float y, bool absolute)
{
	if (x != 0)
		x = _LimitToRange(x, B_HORIZONTAL, absolute);

	if (y != 0)
		y = _LimitToRange(y, B_VERTICAL, absolute);

	// We invalidate before we scroll to avoid the caption messing up the
	// image, and to prevent it from flickering
	if (fShowCaption)
		Invalidate();

	ScrollBy(x, y);
}


// XXX method is not unused
void
ShowImageView::_ScrollRestrictedTo(float x, float y)
{
	_ScrollRestricted(x, y, true);
}


void
ShowImageView::_ScrollRestrictedBy(float x, float y)
{
	_ScrollRestricted(x, y, false);
}


void
ShowImageView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes != 1) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	bool shiftKeyDown = (modifiers() & B_SHIFT_KEY) != 0;

	switch (*bytes) {
		case B_DOWN_ARROW:
			if (shiftKeyDown)
				_ScrollRestrictedBy(0, 10);
			else
				_SendMessageToWindow(MSG_FILE_NEXT);
			break;
		case B_RIGHT_ARROW:
			if (shiftKeyDown)
				_ScrollRestrictedBy(10, 0);
			else
				_SendMessageToWindow(MSG_FILE_NEXT);
			break;
		case B_UP_ARROW:
			if (shiftKeyDown)
				_ScrollRestrictedBy(0, -10);
			else
				_SendMessageToWindow(MSG_FILE_PREV);
			break;
		case B_LEFT_ARROW:
			if (shiftKeyDown)
				_ScrollRestrictedBy(-10, 0);
			else
				_SendMessageToWindow(MSG_FILE_PREV);
			break;
		case B_BACKSPACE:
			_SendMessageToWindow(MSG_FILE_PREV);
			break;
		case B_HOME:
			break;
		case B_END:
			break;
		case B_SPACE:
			_ToggleSlideShow();
			break;
		case B_ESCAPE:
			// stop slide show
			_StopSlideShow();
			_ExitFullScreen();

			ClearSelection();
			break;
		case B_DELETE:
			if (fHasSelection)
				ClearSelection();
			else
				_SendMessageToWindow(kMsgDeleteCurrentFile);
			break;
		case '0':
			FitToBounds();
			break;
		case '1':
			SetZoom(1.0f);
			break;
		case '+':
		case '=':
			ZoomIn();
			break;
		case '-':
			ZoomOut();
			break;
		case '[':
			Rotate(270);
			break;
		case ']':
			Rotate(90);
			break;
	}
}


void
ShowImageView::_MouseWheelChanged(BMessage *msg)
{
	// The BeOS driver does not currently support
	// X wheel scrolling, therefore, dx is zero.
	// |dy| is the number of notches scrolled up or down.
	// When the wheel is scrolled down (towards the user) dy > 0
	// When the wheel is scrolled up (away from the user) dy < 0
	const float kscrollBy = 40;
	float dy, dx;
	float x, y;
	x = 0; y = 0;
	if (msg->FindFloat("be:wheel_delta_x", &dx) == B_OK)
		x = dx * kscrollBy;
	if (msg->FindFloat("be:wheel_delta_y", &dy) == B_OK)
		y = dy * kscrollBy;

	if ((modifiers() & B_SHIFT_KEY) != 0)
		_ScrollRestrictedBy(x, y);
	else if ((modifiers() & B_COMMAND_KEY) != 0)
		_ScrollRestrictedBy(y, x);
	else {
		// Zoom in spot
		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons);

		if (fStickyZoomCountDown <= 0) {
			if (dy < 0)
				ZoomIn(where);
			else if (dy > 0)
				ZoomOut(where);

			if (fZoom == 1.0)
				fStickyZoomCountDown = STICKY_ZOOM_DELAY_TIME;
		}

	}
}


void
ShowImageView::_ShowPopUpMenu(BPoint screen)
{
	if (!fShowingPopUpMenu) {
	  PopUpMenu* menu = new PopUpMenu("PopUpMenu", this);

	  ShowImageWindow* window = dynamic_cast<ShowImageWindow*>(Window());
	  if (window != NULL)
		  window->BuildContextMenu(menu);

	  menu->Go(screen, true, true, true);
	  fShowingPopUpMenu = true;
	}
}


void
ShowImageView::_SettingsSetBool(const char* name, bool value)
{
	ShowImageSettings* settings;
	settings = my_app->Settings();
	if (settings->Lock()) {
		settings->SetBool(name, value);
		settings->Unlock();
	}
}


void
ShowImageView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_COPY_TARGET:
			_HandleDrop(message);
			break;
		case B_MOUSE_WHEEL_CHANGED:
			_MouseWheelChanged(message);
			break;

		case kMsgPopUpMenuClosed:
			fShowingPopUpMenu = false;
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ShowImageView::FixupScrollBar(orientation o, float bitmapLength,
	float viewLength)
{
	float prop, range;
	BScrollBar *psb;

	psb = ScrollBar(o);
	if (psb) {
		range = bitmapLength - viewLength;
		if (range < 0.0) {
			range = 0.0;
		}
		prop = viewLength / bitmapLength;
		if (prop > 1.0) {
			prop = 1.0;
		}
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
}


void
ShowImageView::FixupScrollBars()
{
	BRect viewRect = Bounds();
	BRect bitmapRect;
	if (fBitmap != NULL) {
		bitmapRect = _AlignBitmap();
		bitmapRect.OffsetTo(0, 0);
	}

	FixupScrollBar(B_HORIZONTAL, bitmapRect.Width(), viewRect.Width());
	FixupScrollBar(B_VERTICAL, bitmapRect.Height(), viewRect.Height());
}


void
ShowImageView::SetSelectionMode(bool selectionMode)
{
	// The mode only has an effect in MouseDown()
	fSelectionMode = selectionMode;
}


void
ShowImageView::Undo()
{
	int32 undoType = fUndo.GetType();
	if (undoType != UNDO_UNDO && undoType != UNDO_REDO)
		return;

	// backup current selection
	BRect undoneSelRect;
	BBitmap *undoneSelection;
	undoneSelRect = fSelectionBox.Bounds();
	undoneSelection = _CopySelection();

	if (undoType == UNDO_UNDO) {
		BBitmap *undoRestore;
		undoRestore = fUndo.GetRestoreBitmap();
		if (undoRestore)
			_MergeWithBitmap(undoRestore, fUndo.GetRect());
	}

	// restore previous image/selection
	BBitmap *undoSelection;
	undoSelection = fUndo.GetSelectionBitmap();
		// NOTE: ShowImageView is responsible for deleting this bitmap
		// (Which it will, as it would with a fSelectionBitmap that it allocated itself)
	if (!undoSelection)
		_SetHasSelection(false);
	else {
		fCopyFromRect = BRect();
		fSelectionBox.SetBounds(this, fUndo.GetRect());
		_SetHasSelection(true);
		fSelectionBitmap = undoSelection;
	}

	fUndo.Undo(undoneSelRect, NULL, undoneSelection);

	Invalidate();
}


void
ShowImageView::SelectAll()
{
	fCopyFromRect.Set(0, 0, fBitmap->Bounds().Width(),
		fBitmap->Bounds().Height());
	fSelectionBox.SetBounds(this, fCopyFromRect);
	_SetHasSelection(true);
	Invalidate();
}


void
ShowImageView::ClearSelection()
{
	if (!fHasSelection)
		return;

	_SetHasSelection(false);
	Invalidate();
}


void
ShowImageView::_SetHasSelection(bool hasSelection)
{
	_DeleteSelectionBitmap();
	fHasSelection = hasSelection;

	_UpdateStatusText();

	BMessage msg(MSG_SELECTION);
	msg.AddBool("has_selection", fHasSelection);
	_SendMessageToWindow(&msg);
}


void
ShowImageView::CopySelectionToClipboard()
{
	if (!fHasSelection || !be_clipboard->Lock())
		return;

	be_clipboard->Clear();

	BMessage* data = be_clipboard->Data();
	if (data != NULL) {
		BBitmap* bitmap = _CopySelection();
		if (bitmap != NULL) {
			BMessage bitmapArchive;
			bitmap->Archive(&bitmapArchive);
			// NOTE: Possibly "image/x-be-bitmap" is more correct.
			// This works with WonderBrush, though, which in turn had been
			// tested with other apps.
			data->AddMessage("image/bitmap", &bitmapArchive);
			data->AddPoint("be:location", fSelectionBox.Bounds().LeftTop());

			delete bitmap;

			be_clipboard->Commit();
		}
	}
	be_clipboard->Unlock();
}


void
ShowImageView::SetZoom(float zoom, BPoint where)
{
	float fitToBoundsZoom = _FitToBoundsZoom();
	if (zoom > 32)
		zoom = 32;
	if (zoom < fitToBoundsZoom / 2 && zoom < 0.25)
		zoom = min_c(fitToBoundsZoom / 2, 0.25);

	if (zoom == fZoom) {
		// window size might have changed
		FixupScrollBars();
		return;
	}

	// Invalidate before scrolling, as that prevents the app_server
	// to do the scrolling server side
	Invalidate();

	// zoom to center if not otherwise specified
	BPoint offset;
	if (where.x == -1) {
		where.Set(Bounds().Width() / 2, Bounds().Height() / 2);
		offset = where;
		where += Bounds().LeftTop();
	} else
		offset = where - Bounds().LeftTop();

	float oldZoom = fZoom;
	fZoom = zoom;

	FixupScrollBars();

	if (fBitmap != NULL) {
		offset.x = (int)(where.x * fZoom / oldZoom + 0.5) - offset.x;
		offset.y = (int)(where.y * fZoom / oldZoom + 0.5) - offset.y;
		ScrollTo(offset);
	}
}


void
ShowImageView::ZoomIn(BPoint where)
{
	// snap zoom to "fit to bounds", and "original size"
	float zoom = fZoom * 1.2;
	float zoomSnap = fZoom * 1.25;
	float fitToBoundsZoom = _FitToBoundsZoom();
	if (fZoom < fitToBoundsZoom - 0.001 && zoomSnap > fitToBoundsZoom)
		zoom = fitToBoundsZoom;
	if (fZoom < 1.0 && zoomSnap > 1.0)
		zoom = 1.0;

	SetZoom(zoom, where);
}


void
ShowImageView::ZoomOut(BPoint where)
{
	// snap zoom to "fit to bounds", and "original size"
	float zoom = fZoom / 1.2;
	float zoomSnap = fZoom / 1.25;
	float fitToBoundsZoom = _FitToBoundsZoom();
	if (fZoom > fitToBoundsZoom + 0.001 && zoomSnap < fitToBoundsZoom)
		zoom = fitToBoundsZoom;
	if (fZoom > 1.0 && zoomSnap < 1.0)
		zoom = 1.0;

	SetZoom(zoom, where);
}


/*!	Fits the image to the view bounds.
*/
void
ShowImageView::FitToBounds()
{
	if (fBitmap == NULL)
		return;

	float fitToBoundsZoom = _FitToBoundsZoom();
	if (!fStretchToBounds && fitToBoundsZoom > 1.0f)
		SetZoom(1.0f);
	else
		SetZoom(fitToBoundsZoom);

	FixupScrollBars();
}


void
ShowImageView::_DoImageOperation(ImageProcessor::operation op, bool quiet)
{
	BMessenger msgr;
	ImageProcessor imageProcessor(op, fBitmap, msgr, 0);
	imageProcessor.Start(false);
	BBitmap* bm = imageProcessor.DetachBitmap();
	if (bm == NULL) {
		// operation failed
		return;
	}

	// update orientation state
	if (op != ImageProcessor::kInvert) {
		// Note: If one of these fails, check its definition in class ImageProcessor.
//		ASSERT(ImageProcessor::kRotateClockwise < ImageProcessor::kNumberOfAffineTransformations);
//		ASSERT(ImageProcessor::kRotateCounterClockwise < ImageProcessor::kNumberOfAffineTransformations);
//		ASSERT(ImageProcessor::kFlipLeftToRight < ImageProcessor::kNumberOfAffineTransformations);
//		ASSERT(ImageProcessor::kFlipTopToBottom < ImageProcessor::kNumberOfAffineTransformations);
		fImageOrientation = fTransformation[op][fImageOrientation];
	}

	if (!quiet) {
		// write orientation state
		BNode node(&fCurrentRef);
		int32 orientation = fImageOrientation;
		if (orientation != k0) {
			node.WriteAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE, B_INT32_TYPE, 0,
				&orientation, sizeof(orientation));
		} else {
			node.RemoveAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE);
		}
	}

	// set new bitmap
	_DeleteBitmap();
	fBitmap = bm;

	if (!quiet) {
		// remove selection
		_SetHasSelection(false);
		_Notify();
	}
}


//! Image operation initiated by user
void
ShowImageView::_UserDoImageOperation(ImageProcessor::operation op, bool quiet)
{
	fUndo.Clear();
	_DoImageOperation(op, quiet);
}


void
ShowImageView::Rotate(int degree)
{
	_UserDoImageOperation(degree == 90 ? ImageProcessor::kRotateClockwise
		: ImageProcessor::kRotateCounterClockwise);

	FitToBounds();
}


void
ShowImageView::Flip(bool vertical)
{
	if (vertical)
		_UserDoImageOperation(ImageProcessor::kFlipLeftToRight);
	else
		_UserDoImageOperation(ImageProcessor::kFlipTopToBottom);
}


void
ShowImageView::ResizeImage(int w, int h)
{
	if (fBitmap == NULL || w < 1 || h < 1)
		return;

	Scaler scaler(fBitmap, BRect(0, 0, w-1, h-1), BMessenger(), 0, false);
	scaler.Start(false);
	BBitmap* scaled = scaler.DetachBitmap();
	if (scaled == NULL) {
		// operation failed
		return;
	}

	// remove selection
	_SetHasSelection(false);
	fUndo.Clear();
	_DeleteBitmap();
	fBitmap = scaled;

	_SendMessageToWindow(MSG_MODIFIED);

	_Notify();
}


void
ShowImageView::_SetIcon(bool clear, icon_size which)
{
	int32 size;
	switch (which) {
		case B_MINI_ICON: size = 16;
			break;
		case B_LARGE_ICON: size = 32;
			break;
		default:
			return;
	}

	BRect rect(fBitmap->Bounds());
	float s;
	s = size / (rect.Width()+1.0);

	if (s * (rect.Height()+1.0) <= size) {
		rect.right = size-1;
		rect.bottom = static_cast<int>(s * (rect.Height()+1.0))-1;
		// center vertically
		rect.OffsetBy(0, (size - rect.IntegerHeight()) / 2);
	} else {
		s = size / (rect.Height()+1.0);
		rect.right = static_cast<int>(s * (rect.Width()+1.0))-1;
		rect.bottom = size-1;
		// center horizontally
		rect.OffsetBy((size - rect.IntegerWidth()) / 2, 0);
	}

	// scale bitmap to thumbnail size
	BMessenger msgr;
	Scaler scaler(fBitmap, rect, msgr, 0, true);
	BBitmap* thumbnail = scaler.GetBitmap();
	scaler.Start(false);
	ASSERT(thumbnail->ColorSpace() == B_CMAP8);
	// create icon from thumbnail
	BBitmap icon(BRect(0, 0, size-1, size-1), B_CMAP8);
	memset(icon.Bits(), B_TRANSPARENT_MAGIC_CMAP8, icon.BitsLength());
	BScreen screen;
	const uchar* src = (uchar*)thumbnail->Bits();
	uchar* dest = (uchar*)icon.Bits();
	const int32 srcBPR = thumbnail->BytesPerRow();
	const int32 destBPR = icon.BytesPerRow();
	const int32 dx = (int32)rect.left;
	const int32 dy = (int32)rect.top;

	for (int32 y = 0; y <= rect.IntegerHeight(); y ++) {
		for (int32 x = 0; x <= rect.IntegerWidth(); x ++) {
			const uchar* s = src + y * srcBPR + x;
			uchar* d = dest + (y+dy) * destBPR + (x+dx);
			*d = *s;
		}
	}

	// set icon
	BNode node(&fCurrentRef);
	BNodeInfo info(&node);
	info.SetIcon(clear ? NULL : &icon, which);
}


void
ShowImageView::SetIcon(bool clear)
{
	_SetIcon(clear, B_MINI_ICON);
	_SetIcon(clear, B_LARGE_ICON);
}


void
ShowImageView::_ToggleSlideShow()
{
	_SendMessageToWindow(MSG_SLIDE_SHOW);
}


void
ShowImageView::_StopSlideShow()
{
	_SendMessageToWindow(kMsgStopSlideShow);
}


void
ShowImageView::_ExitFullScreen()
{
	be_app->ShowCursor();
	_SendMessageToWindow(MSG_EXIT_FULL_SCREEN);
}


void
ShowImageView::_ShowToolBarIfEnabled(bool show)
{
	BMessage message(kShowToolBarIfEnabled);
	message.AddBool("show", show);
	Window()->PostMessage(&message);
}


void
ShowImageView::WindowActivated(bool active)
{
	fIsActiveWin = active;
	fHideCursorCountDown = HIDE_CURSOR_DELAY_TIME;
}

