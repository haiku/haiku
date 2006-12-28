/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
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
 */


#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageView.h"
#include "ShowImageWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
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

#include <math.h>
#include <new>
#include <stdio.h>

// TODO: Remove this and use Tracker's Command.h once it is moved into the private headers
namespace BPrivate {
	const uint32 kMoveToTrash = 'Ttrs';
}

using std::nothrow;


class PopUpMenu : public BPopUpMenu {
	public:
		PopUpMenu(const char* name, BMessenger target);
		virtual ~PopUpMenu();

	private:
		BMessenger fTarget;
};


#define SHOW_IMAGE_ORIENTATION_ATTRIBUTE "ShowImage:orientation"
const rgb_color kBorderColor = { 0, 0, 0, 255 };

enum ShowImageView::image_orientation 
ShowImageView::fTransformation[ImageProcessor::kNumberOfAffineTransformations][kNumberOfOrientations] = {
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


static bool
entry_ref_is_file(const entry_ref *ref)
{
	BEntry entry(ref);
	if (entry.InitCheck() != B_OK)
		return false;

	return entry.IsFile();
}


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
	: BPopUpMenu(name, false, false),
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
	: BView(rect, name, resizingMode, flags)
{
	ShowImageSettings* settings;
	settings = my_app->Settings();
	
	InitPatterns();
	fDither = false;
	fBitmap = NULL;
	fDisplayBitmap = NULL;
	fSelBitmap = NULL;
	fDocumentIndex = 1;
	fDocumentCount = 1;
	fAnimateSelection = true;
	fHasSelection = false;
	fShrinkToBounds = false;
	fZoomToBounds = false;
	fFullScreen = false;
	fSlideShow = false;
	fSlideShowDelay = 3 * 10; // 3 seconds
	fShowCaption = false;
	fZoom = 1.0;
	fMovesImage = false;
	fScaleBilinear = false;
	fScaler = NULL;
#if DELAYED_SCALING
	fScalingCountDown = SCALING_DELAY_TIME;
#endif
	fShowingPopUpMenu = false;
	fHideCursorCountDown = HIDE_CURSOR_DELAY_TIME;
	fIsActiveWin = true;

	if (settings->Lock()) {
		fDither = settings->GetBool("Dither", fDither);
		fShrinkToBounds = settings->GetBool("ShrinkToBounds", fShrinkToBounds);
		fZoomToBounds = settings->GetBool("ZoomToBounds", fZoomToBounds);
		fSlideShowDelay = settings->GetInt32("SlideShowDelay", fSlideShowDelay);
		fScaleBilinear = settings->GetBool("ScaleBilinear", fScaleBilinear);
		settings->Unlock();
	}
	
	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(kBorderColor);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetPenSize(PEN_SIZE);
}


ShowImageView::~ShowImageView()
{
	DeleteBitmap();
}


//! Use patterns to simulate marching ants for selection
void 
ShowImageView::InitPatterns()
{
	uchar p;
	uchar p1 = 0x33;
	uchar p2 = 0xCC;
	for (int i = 0; i <= 7; i ++) {
		fPatternLeft.data[i] = p1;
		fPatternRight.data[i] = p2;
		if ((i / 2) % 2 == 0) {
			p = 255;
		} else {
			p = 0;
		}
		fPatternUp.data[i] = p;
		fPatternDown.data[i] = ~p;
	}
}


void
ShowImageView::RotatePatterns()
{
	int i;
	uchar p;
	bool set;
	
	// rotate up
	p = fPatternUp.data[0];
	for (i = 0; i <= 6; i ++) {
		fPatternUp.data[i] = fPatternUp.data[i+1];	
	}
	fPatternUp.data[7] = p;
	
	// rotate down
	p = fPatternDown.data[7];
	for (i = 7; i >= 1; i --) {
		fPatternDown.data[i] = fPatternDown.data[i-1];
	}
	fPatternDown.data[0] = p;
	
	// rotate to left
	p = fPatternLeft.data[0];
	set = (p & 0x80) != 0;
	p <<= 1;
	p &= 0xfe;
	if (set) p |= 1;
	memset(fPatternLeft.data, p, 8);
	
	// rotate to right
	p = fPatternRight.data[0];
	set = (p & 1) != 0;
	p >>= 1;
	if (set) p |= 0x80;
	memset(fPatternRight.data, p, 8);
}


void
ShowImageView::AnimateSelection(bool enabled)
{
	fAnimateSelection = enabled;
}


void
ShowImageView::Pulse()
{
	// animate marching ants
	if (HasSelection() && fAnimateSelection && fIsActiveWin) {	
		RotatePatterns();
		DrawSelectionBox();
	}
	if (fSlideShow) {
		fSlideShowCountDown --;
		if (fSlideShowCountDown <= 0) {
			fSlideShowCountDown = fSlideShowDelay;
			if (!NextFile()) {
				FirstFile();
			}
		}
	}

	// Hide cursor in full screen mode
	if (fFullScreen && !HasSelection() && !fShowingPopUpMenu && fIsActiveWin) {
		if (fHideCursorCountDown <= 0)
			be_app->ObscureCursor();
		else
			fHideCursorCountDown--;
	}

#if DELAYED_SCALING
	if (fBitmap && (fScaleBilinear || fDither) && fScalingCountDown > 0) {
		if (fScalingCountDown == 1) {
			fScalingCountDown = 0;
			GetScaler(AlignBitmap());
		} else {
			fScalingCountDown --;
		}
	}
#endif	
}


bool
ShowImageView::IsImage(const entry_ref *ref)
{
	if (ref == NULL || !entry_ref_is_file(ref))
		return false;

	BFile file(ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (!roster)
		return false;
		
	BMessage ioExtension;
	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return false;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	if (roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return false;
		
	return true;
}


void
ShowImageView::SetTrackerMessenger(const BMessenger& trackerMessenger)
{
	fTrackerMessenger = trackerMessenger;
}


//! send message to parent about new image
void 
ShowImageView::Notify(const char* status)
{
	BMessage msg(MSG_UPDATE_STATUS);
	if (status != NULL) {
		msg.AddString("status", status);
	}
	msg.AddInt32("width", fBitmap->Bounds().IntegerWidth() + 1);
	msg.AddInt32("height", fBitmap->Bounds().IntegerHeight() + 1);

	msg.AddInt32("colors", fBitmap->ColorSpace());
	BMessenger msgr(Window());
	msgr.SendMessage(&msg);

	FixupScrollBars();
	Invalidate();
}


void
ShowImageView::AddToRecentDocuments()
{
	be_roster->AddToRecentDocuments(&fCurrentRef, kApplicationSignature);
}


void
ShowImageView::DeleteScaler()
{
	if (fScaler) {
		fScaler->Stop();
		delete fScaler;
		fScaler = NULL;
	}
#if DELAYED_SCALING
	fScalingCountDown = SCALING_DELAY_TIME;
#endif
}


void
ShowImageView::DeleteBitmap()
{
	DeleteScaler();
	DeleteSelBitmap();

	if (fDisplayBitmap != fBitmap)
		delete fDisplayBitmap;
	fDisplayBitmap = NULL;

	delete fBitmap;
	fBitmap = NULL;
}


void
ShowImageView::DeleteSelBitmap()
{
	delete fSelBitmap;
	fSelBitmap = NULL;
}


status_t
ShowImageView::SetImage(const entry_ref *ref)
{
	// If no file was specified, load the specified page of
	// the current file.
	if (ref == NULL)
		ref = &fCurrentRef;

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (!roster)
		return B_ERROR;

	if (!entry_ref_is_file(ref))
		return B_ERROR;

	BFile file(ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;
	if (ref != &fCurrentRef) {
		// if new image, reset to first document
		fDocumentIndex = 1;
	}
	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return B_ERROR;
	if (roster->Identify(&file, &ioExtension, &info, 0, NULL,
			B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;

	// Translate image data and create a new ShowImage window
	BBitmapStream outstream;
	if (roster->Translate(&file, &info, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;

	BBitmap *newBitmap = NULL;
	if (outstream.DetachBitmap(&newBitmap) != B_OK)
		return B_ERROR;

	// Now that I've successfully loaded the new bitmap,
	// I can be sure it is safe to delete the old one, 
	// and clear everything
	fUndo.Clear();
	SetHasSelection(false);
	fMakesSelection = false;
	DeleteBitmap();
	fBitmap = newBitmap;
	fDisplayBitmap = NULL;
	newBitmap = NULL;
	fCurrentRef = *ref;

	// prepare the display bitmap
	if (fBitmap->ColorSpace() == B_RGBA32)
		fDisplayBitmap = compose_checker_background(fBitmap);

	if (!fDisplayBitmap)
		fDisplayBitmap = fBitmap;

	// restore orientation
	int32 orientation;
	fImageOrientation = k0;
	fInverted = false;
	if (file.ReadAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE, B_INT32_TYPE, 0,
			&orientation, sizeof(orientation)) == sizeof(orientation)) {
		if (orientation & 256)
			DoImageOperation(ImageProcessor::ImageProcessor::kInvert, true);

		orientation &= 255;
		switch (orientation) {
			case k0:
				break;
			case k90:
				DoImageOperation(ImageProcessor::kRotateClockwise, true);
				break;
			case k180:
				DoImageOperation(ImageProcessor::kRotateClockwise, true);
				DoImageOperation(ImageProcessor::kRotateClockwise, true);
				break;
			case k270:
				DoImageOperation(ImageProcessor::kRotateAntiClockwise, true);
				break;
			case k0V:
				DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
				break;
			case k90V:
				DoImageOperation(ImageProcessor::kRotateClockwise, true);
				DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
				break;
			case k0H:
				DoImageOperation(ImageProcessor::ImageProcessor::kFlipLeftToRight, true);
				break;
			case k270V:
				DoImageOperation(ImageProcessor::kRotateAntiClockwise, true);
				DoImageOperation(ImageProcessor::ImageProcessor::kFlipTopToBottom, true);
				break;
		}
	}

	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK &&
			documentCount > 0)
		fDocumentCount = documentCount;
	else
		fDocumentCount = 1;

	GetPath(&fCaption);
	if (fDocumentCount > 1)
		fCaption << ", " << fDocumentIndex << "/" << fDocumentCount;

	fCaption << ", " << info.name;

	AddToRecentDocuments();

	Notify(info.name);
	return B_OK;
}


status_t
ShowImageView::SetSelection(const entry_ref *ref, BPoint point)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (!roster)
		return B_ERROR;

	BFile file(ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	if (roster->Identify(&file, NULL, &info, 0, NULL,
			B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;

	// Translate image data and create a new ShowImage window
	BBitmapStream outstream;
	if (roster->Translate(&file, &info, NULL, &outstream,
			B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;

	BBitmap *newBitmap = NULL;
	if (outstream.DetachBitmap(&newBitmap) != B_OK)
		return B_ERROR;
		
	return PasteBitmap(newBitmap, point);
}


void
ShowImageView::SetDither(bool dither)
{
	if (fDither != dither) {
		SettingsSetBool("Dither", dither);
		fDither = dither;
		Invalidate();
	}
}


void 
ShowImageView::SetShowCaption(bool show)
{
	if (fShowCaption != show) {
		fShowCaption = show;
		UpdateCaption();
	}
}


void
ShowImageView::SetShrinkToBounds(bool enable)
{
	if (fShrinkToBounds != enable) {
		SettingsSetBool("ShrinkToBounds", enable);
		fShrinkToBounds = enable;
		FixupScrollBars();
		Invalidate();
	}
}


void
ShowImageView::SetZoomToBounds(bool enable)
{
	if (fZoomToBounds != enable) {
		SettingsSetBool("ZoomToBounds", enable);
		fZoomToBounds = enable;
		FixupScrollBars();
		Invalidate();
	}
}


void
ShowImageView::SetFullScreen(bool fullScreen)
{
	if (fFullScreen != fullScreen) {
		fFullScreen = fullScreen;
		if (fFullScreen) {
			SetLowColor(0, 0, 0, 255);
		} else
			SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
}


BBitmap *
ShowImageView::GetBitmap()
{
	return fBitmap;
}


void
ShowImageView::GetName(BString* outName)
{
	BEntry entry(&fCurrentRef);
	char name[B_FILE_NAME_LENGTH];
	if (entry.InitCheck() < B_OK || entry.GetName(name) < B_OK)
		outName->SetTo("");
	else
		outName->SetTo(name);
}


void
ShowImageView::GetPath(BString *outPath)
{
	BEntry entry(&fCurrentRef);
	BPath path;
	if (entry.InitCheck() < B_OK || entry.GetPath(&path) < B_OK)
		outPath->SetTo("");
	else
		outPath->SetTo(path.Path());
}


void
ShowImageView::SetScaleBilinear(bool enabled)
{
	if (fScaleBilinear != enabled) {
		SettingsSetBool("ScaleBilinear", enabled);
		fScaleBilinear = enabled;
		Invalidate();
	}
}


void
ShowImageView::AttachedToWindow()
{
	fUndo.SetWindow(Window());
	FixupScrollBars();
}


BRect
ShowImageView::AlignBitmap()
{
	BRect rect(fBitmap->Bounds());

	// the width/height of the bitmap (in pixels)
	float bitmapWidth = rect.Width() + 1.0;
	float bitmapHeight = rect.Height() + 1.0;

	// the available width/height for layouting the bitmap (in pixels)
	float width = Bounds().Width() - 2 * PEN_SIZE + 1.0;
	float height = Bounds().Height() - 2 * PEN_SIZE + 1.0;

	if (width == 0 || height == 0)
		return rect;

	fShrinkOrZoomToBounds = (fShrinkToBounds && 
		(bitmapWidth >= width || bitmapHeight >= height)) ||
		(fZoomToBounds && (bitmapWidth < width && bitmapHeight < height));
	if (fShrinkOrZoomToBounds) {
		float s = width / bitmapWidth;

		if (s * bitmapHeight <= height) {
			rect.right = width - 1;
			rect.bottom = static_cast<int>(s * bitmapHeight) - 1;
			// center vertically
			rect.OffsetBy(0, static_cast<int>((height - rect.Height()) / 2));
		} else {
			s = height / bitmapHeight;
			rect.right = static_cast<int>(s * bitmapWidth) - 1;
			rect.bottom = height - 1;
			// center horizontally
			rect.OffsetBy(static_cast<int>((width - rect.Width()) / 2), 0);
		}
	} else {
		// zoom image
		rect.right = floorf(bitmapWidth * fZoom) - 1; 
		rect.bottom = floorf(bitmapHeight * fZoom) - 1;

		// update the bitmap size after the zoom
		bitmapWidth = rect.Width() + 1.0;
		bitmapHeight = rect.Height() + 1.0;

		// always align in the center
		if (width > bitmapWidth)
			rect.OffsetBy((width - bitmapWidth) / 2.0, 0);

		if (height > bitmapHeight)
			rect.OffsetBy(0, (height - bitmapHeight) / 2.0);
	}
	rect.OffsetBy(PEN_SIZE, PEN_SIZE);
	return rect;
}


void
ShowImageView::Setup(BRect rect)
{
	fLeft = floorf(rect.left);
	fTop = floorf(rect.top);
	fZoom = (rect.Width()+1.0) / (fBitmap->Bounds().Width()+1.0);
}


BPoint
ShowImageView::ImageToView(BPoint p) const
{
	p.x = floorf(fZoom * p.x + fLeft);
	p.y = floorf(fZoom * p.y + fTop);
	return p;
}


BPoint
ShowImageView::ViewToImage(BPoint p) const
{
	p.x = floorf((p.x - fLeft) / fZoom);
	p.y = floorf((p.y - fTop) / fZoom);
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
ShowImageView::DrawBorder(BRect border)
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
ShowImageView::LayoutCaption(BFont &font, BPoint &pos, BRect &rect)
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
ShowImageView::DrawCaption()
{
	BFont font;
	BPoint position;
	BRect rect;
	LayoutCaption(font, position, rect);		

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
ShowImageView::UpdateCaption()
{
	BFont font;
	BPoint pos;
	BRect rect;
	LayoutCaption(font, pos, rect);		

	// draw over portion of image where caption is located
	BRegion clip(rect);
	PushState();
	ConstrainClippingRegion(&clip);
	Draw(rect);
	PopState();
}


Scaler*
ShowImageView::GetScaler(BRect rect)
{
	if (fScaler == NULL || !fScaler->Matches(rect, fDither)) {
		DeleteScaler();
		BMessenger msgr(this, Window());
		fScaler = new Scaler(fDisplayBitmap, rect, msgr, MSG_INVALIDATE, fDither);
		fScaler->Start();
	}
	return fScaler;
}


void
ShowImageView::DrawImage(BRect rect)
{
	if (fScaleBilinear || fDither) {
#if DELAYED_SCALING
		Scaler* scaler = fScaler;
		if (scaler != NULL && !scaler->Matches(rect, fDither)) {
			DeleteScaler(); scaler = NULL;
		}
#else
		Scaler* scaler = GetScaler(rect);
#endif
		if (scaler != NULL && !scaler->IsRunning()) {
			BBitmap* bitmap = scaler->GetBitmap();

			if (bitmap) {
				DrawBitmap(bitmap, BPoint(rect.left, rect.top));
				return;
			}
		}
	}
	// TODO: fix composing of fBitmap with other bitmaps
	// with regard to alpha channel
	if (!fDisplayBitmap)
		fDisplayBitmap = fBitmap;

	DrawBitmap(fDisplayBitmap, fDisplayBitmap->Bounds(), rect);
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

	BRect rect = AlignBitmap();
	Setup(rect);

	BRect border(rect);
	border.InsetBy(-PEN_SIZE, -PEN_SIZE);

	DrawBorder(border);

	// Draw black rectangle around image
	StrokeRect(border);

	// Draw image
	DrawImage(rect);

	if (fShowCaption)
		DrawCaption();

	if (HasSelection()) {
		if (fSelBitmap) {
			BRect srcBits, destRect;
			GetSelMergeRects(srcBits, destRect);
			destRect = ImageToView(destRect);
			DrawBitmap(fSelBitmap, srcBits, destRect);
		}
		DrawSelectionBox();
	}
}


void
ShowImageView::DrawSelectionBox()
{
	BRect r(fSelectionRect);
	ConstrainToImage(r);
	r = ImageToView(r);
	// draw selection box *around* selection
	r.InsetBy(-1, -1);
	PushState();
	rgb_color white = {255, 255, 255};
	SetLowColor(white);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top), fPatternLeft);
	StrokeLine(BPoint(r.right, r.top+1), BPoint(r.right, r.bottom-1), fPatternUp);
	StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom), fPatternRight);
	StrokeLine(BPoint(r.left, r.top+1), BPoint(r.left, r.bottom-1), fPatternDown);
	PopState();
}


void
ShowImageView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
}


void
ShowImageView::ConstrainToImage(BPoint &point)
{
	point.ConstrainTo(fBitmap->Bounds());
}


void
ShowImageView::ConstrainToImage(BRect &rect)
{
	rect = rect & fBitmap->Bounds();
}


BBitmap*
ShowImageView::CopyFromRect(BRect srcRect)
{	
	BRect rect(0, 0, srcRect.Width(), srcRect.Height());
	BView view(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap *bitmap = new(nothrow) BBitmap(rect, fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		view.DrawBitmap(fBitmap, srcRect, rect);
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
	}

	return bitmap;
}


BBitmap*
ShowImageView::CopySelection(uchar alpha, bool imageSize)
{
	bool hasAlpha = alpha != 255;

	if (!HasSelection())
		return NULL;

	BRect rect(0, 0, fSelectionRect.Width(), fSelectionRect.Height());
	if (!imageSize) {
		// scale image to view size
		rect.right = floorf((rect.right + 1.0) * fZoom - 1.0); 
		rect.bottom = floorf((rect.bottom + 1.0) * fZoom - 1.0);
	}
	BView view(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap *bitmap = new(nothrow) BBitmap(rect, hasAlpha ? B_RGBA32 : fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		if (fSelBitmap)
			view.DrawBitmap(fSelBitmap, fSelBitmap->Bounds(), rect);
		else
			view.DrawBitmap(fBitmap, fCopyFromRect, rect);
		if (hasAlpha) {
			view.SetDrawingMode(B_OP_SUBTRACT);
			view.SetHighColor(0, 0, 0, 255-alpha);
			view.FillRect(rect, B_SOLID_HIGH);
		}
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
	}

	return bitmap;
}


bool
ShowImageView::AddSupportedTypes(BMessage* msg, BBitmap* bitmap)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	BBitmapStream stream(bitmap);

	translator_info *outInfo;
	bool found = false;
	int32 outNumInfo;		
	if (roster->GetTranslators(&stream, NULL, &outInfo, &outNumInfo) == B_OK) {
		for (int32 i = 0; i < outNumInfo; i++) {
			const translation_format *fmts;
			int32 num_fmts;
			roster->GetOutputFormats(outInfo[i].translator, &fmts, &num_fmts);
			for (int32 j = 0; j < num_fmts; j++) {
				if (strcmp(fmts[j].MIME, "image/x-be-bitmap") != 0) {
					// needed to send data in message
 					msg->AddString("be:types", fmts[j].MIME);
 					// needed to pass data via file
					msg->AddString("be:filetypes", fmts[j].MIME);
					msg->AddString("be:type_descriptions", fmts[j].name);
				}
				found = true;
			}
		}
	}
	stream.DetachBitmap(&bitmap);

	return found;
}


void
ShowImageView::BeginDrag(BPoint sourcePoint)
{
	BBitmap* bitmap = CopySelection(128, false);
	if (bitmap == NULL)
		return;

	SetMouseEventMask(B_POINTER_EVENTS);

	// fill the drag message
	BMessage drag(B_SIMPLE_DATA);
	drag.AddInt32("be:actions", B_COPY_TARGET);
	drag.AddString("be:clip_name", "Bitmap Clip"); 
	// ShowImage specific fields
	drag.AddPoint("be:_source_point", sourcePoint);
	drag.AddRect("be:_frame", fSelectionRect);
	if (AddSupportedTypes(&drag, bitmap)) {
		// we also support "Passing Data via File" protocol
		drag.AddString("be:types", B_FILE_MIME_TYPE);
		// avoid flickering of dragged bitmap caused by drawing into the window
		AnimateSelection(false);
		// only use a transparent bitmap on selections less than 400x400 (taking into account scaling)
		if ((fSelectionRect.Width() * fZoom) < 400.0 && (fSelectionRect.Height() * fZoom) < 400.0)
		{
			sourcePoint -= fSelectionRect.LeftTop();
			sourcePoint.x *= fZoom;
			sourcePoint.y *= fZoom;
			// DragMessage takes ownership of bitmap
			DragMessage(&drag, bitmap, B_OP_ALPHA, sourcePoint);
			bitmap = NULL;
		}
		else
		{
			delete bitmap;
			// Offset and scale the rect
			BRect rect(fSelectionRect);
			rect = ImageToView(rect);
			rect.InsetBy(-1, -1);
			DragMessage(&drag, rect);
		}
	}
}


bool
ShowImageView::OutputFormatForType(BBitmap* bitmap, const char* type,
	translation_format* format)
{
	bool found = false;

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	BBitmapStream stream(bitmap);
	
	translator_info *outInfo;
	int32 outNumInfo;
	if (roster->GetTranslators(&stream, NULL, &outInfo, &outNumInfo) == B_OK) {		
		for (int32 i = 0; i < outNumInfo; i++) {
			const translation_format *fmts;
			int32 num_fmts;
			roster->GetOutputFormats(outInfo[i].translator, &fmts, &num_fmts);
			for (int32 j = 0; j < num_fmts; j++) {
				if (strcmp(fmts[j].MIME, type) == 0) {
					*format = fmts[j];
					found = true; 
					break;
				}
			}
		}
	}
	stream.DetachBitmap(&bitmap);
	return found;
}


void
ShowImageView::SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap,
	const translation_format* format)
{
	if (!bitmap) {
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
		BString errText;
		errText << "Sorry, the file '" << name << "' could not be written.";
		BAlert *palert = new BAlert(NULL, errText.String(), "Ok");
		palert->Go();
	}

	stream.DetachBitmap(&bitmap);
		// Don't allow the bitmap to be deleted, this is
		// especially important when using fBitmap as the bitmap
}


void
ShowImageView::SendInMessage(BMessage* msg, BBitmap* bitmap, translation_format* format)
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
ShowImageView::HandleDrop(BMessage* msg)
{
	BMessage data(B_MIME_DATA);
	entry_ref dirRef;
	BString name, type;
	bool saveToFile;
	bool sendInMessage;
	BBitmap *bitmap;

	saveToFile = msg->FindString("be:filetypes", &type) == B_OK
		&& msg->FindRef("directory", &dirRef) == B_OK
		&& msg->FindString("name", &name) == B_OK;

	sendInMessage = (!saveToFile) && msg->FindString("be:types", &type) == B_OK;

	bitmap = CopySelection();
	if (bitmap == NULL)
		return;

	translation_format format;
	if (!OutputFormatForType(bitmap, type.String(), &format)) {
		delete bitmap;
		return;
	}

	if (saveToFile) {
		BDirectory dir(&dirRef);
		SaveToFile(&dir, name.String(), bitmap, &format);
		delete bitmap;
	} else if (sendInMessage) {
		SendInMessage(msg, bitmap, &format);
	} else {
		delete bitmap;
	}
}


void
ShowImageView::MoveImage()
{
	BPoint point, delta;
	uint32 buttons;
	// get CURRENT position
	GetMouse(&point, &buttons);
	point = ConvertToScreen(point);
	delta = fFirstPoint - point;
	fFirstPoint = point;
	ScrollRestrictedBy(delta.x, delta.y);

	// in case we miss MouseUp
	if ((GetMouseButtons() & B_TERTIARY_MOUSE_BUTTON) == 0)
		fMovesImage = false;
}


uint32
ShowImageView::GetMouseButtons()
{
	uint32 buttons;
	BPoint point;
	GetMouse(&point, &buttons);
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		if ((modifiers() & B_CONTROL_KEY) != 0) {
			buttons = B_SECONDARY_MOUSE_BUTTON; // simulate second button 
		} else if ((modifiers() & B_SHIFT_KEY) != 0) {
			buttons = B_TERTIARY_MOUSE_BUTTON; // simulate third button 
		}
	}
	return buttons;
}


void
ShowImageView::GetMergeRects(BBitmap *merge, BRect selection, BRect &srcBits,
	BRect &destRect)
{
	destRect = selection;
	ConstrainToImage(destRect);

	srcBits = selection;
	if (srcBits.left < 0)
		srcBits.left = -(srcBits.left);
	else
		srcBits.left = 0;

	if (srcBits.top < 0)
		srcBits.top = -(srcBits.top);
	else
		srcBits.top = 0;

	if (srcBits.right > fBitmap->Bounds().right)
		srcBits.right = srcBits.left + destRect.Width();
	else
		srcBits.right = merge->Bounds().right;

	if (srcBits.bottom > fBitmap->Bounds().bottom)
		srcBits.bottom = srcBits.top + destRect.Height();
	else
		srcBits.bottom = merge->Bounds().bottom;	
}


void
ShowImageView::GetSelMergeRects(BRect &srcBits, BRect &destRect)
{
	GetMergeRects(fSelBitmap, fSelectionRect, srcBits, destRect);
}


void
ShowImageView::MergeWithBitmap(BBitmap *merge, BRect selection)
{
	BView view(fBitmap->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap *bitmap = new(nothrow) BBitmap(fBitmap->Bounds(), fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		view.DrawBitmap(fBitmap, fBitmap->Bounds());
		BRect srcBits, destRect;
		GetMergeRects(merge, selection, srcBits, destRect);
		view.DrawBitmap(merge, srcBits, destRect);
		
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
		
		DeleteBitmap();
		fBitmap = bitmap;
		
		BMessenger msgr(Window());
		msgr.SendMessage(MSG_MODIFIED);
	} else
		delete bitmap;
}


void
ShowImageView::MergeSelection()
{
	if (!HasSelection())
		return;
		
	if (!fSelBitmap) {
		// Even though the merge will not change
		// the background image, I still need to save
		// some undo information here
		fUndo.SetTo(fSelectionRect, NULL, CopySelection());
		return;
	}

	// Merge selection with background
	fUndo.SetTo(fSelectionRect, CopyFromRect(fSelectionRect), CopySelection());
	MergeWithBitmap(fSelBitmap, fSelectionRect);
}


void
ShowImageView::MouseDown(BPoint position)
{
	BPoint point;
	uint32 buttons;
	MakeFocus(true);

	point = ViewToImage(position);
	buttons = GetMouseButtons();
	
	if (HasSelection() && fSelectionRect.Contains(point)
		&& (buttons & (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON))) {
		if (!fSelBitmap)
			fSelBitmap = CopySelection();

		BPoint sourcePoint = point;
		BeginDrag(sourcePoint);
		
		while (buttons) {
			// Keep reading mouse movement until
			// the user lets up on all mouse buttons
			GetMouse(&point, &buttons);		
			snooze(25 * 1000);
				// sleep for 25 milliseconds to minimize CPU usage during loop
		}

		if (Bounds().Contains(point)) {
			// If selection stayed inside this view
			// (Some of the selection may be in the border area, which can be OK)
			BPoint last, diff;
			last = ViewToImage(point);
			diff = last - sourcePoint;	
			
			BRect newSelection = fSelectionRect;
			newSelection.OffsetBy(diff);
			
			if (fBitmap->Bounds().Intersects(newSelection)) {
				// Do not accept the new selection box location
				// if it does not intersect with the bitmap rectangle
				fSelectionRect = newSelection;
				Invalidate();
			}
		}

		AnimateSelection(true);
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		MergeSelection();
			// If there is an existing selection, 
			// Make it part of the background image

		// begin new selection
		SetHasSelection(true);
		fMakesSelection = true;
		SetMouseEventMask(B_POINTER_EVENTS);
		ConstrainToImage(point);
		fFirstPoint = point;
		fCopyFromRect.Set(point.x, point.y, point.x, point.y);
		fSelectionRect = fCopyFromRect;
		Invalidate(); 
	} else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowPopUpMenu(ConvertToScreen(position));
	} else if (buttons == B_TERTIARY_MOUSE_BUTTON) {
		// move image in window
		SetMouseEventMask(B_POINTER_EVENTS);
		fMovesImage = true;
		fFirstPoint = ConvertToScreen(position);
	}
}


void
ShowImageView::UpdateSelectionRect(BPoint point, bool final)
{
	BRect oldSelection = fCopyFromRect;
	point = ViewToImage(point);
	ConstrainToImage(point);
	fCopyFromRect.left = min_c(fFirstPoint.x, point.x);
	fCopyFromRect.right = max_c(fFirstPoint.x, point.x);
	fCopyFromRect.top = min_c(fFirstPoint.y, point.y);
	fCopyFromRect.bottom = max_c(fFirstPoint.y, point.y);
	fSelectionRect = fCopyFromRect;

	if (final) {
		// selection must be at least 2 pixels wide or 2 pixels tall
		if (fCopyFromRect.Width() < 1.0 && fCopyFromRect.Height() < 1.0)
			SetHasSelection(false);
	}
	if (oldSelection != fCopyFromRect || !HasSelection()) {
		BRect updateRect;
		updateRect = oldSelection | fCopyFromRect;
		updateRect = ImageToView(updateRect);
		updateRect.InsetBy(-PEN_SIZE, -PEN_SIZE);
		Invalidate(updateRect);
	}
}


void
ShowImageView::MouseMoved(BPoint point, uint32 state, const BMessage *message)
{
	fHideCursorCountDown = HIDE_CURSOR_DELAY_TIME;
	if (fMakesSelection) {
		UpdateSelectionRect(point, false);
	} else if (fMovesImage) {
		MoveImage();
	}
}


void
ShowImageView::MouseUp(BPoint point)
{
	if (fMakesSelection) {
		UpdateSelectionRect(point, true);
		fMakesSelection = false;
	} else if (fMovesImage) {
		MoveImage();
		if (fMovesImage)
			fMovesImage = false;
	}
	AnimateSelection(true);
}


float
ShowImageView::LimitToRange(float v, orientation o, bool absolute)
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
ShowImageView::ScrollRestricted(float x, float y, bool absolute)
{
	if (x != 0)
		x = LimitToRange(x, B_HORIZONTAL, absolute);

	if (y != 0)
		y = LimitToRange(y, B_VERTICAL, absolute);

	// hide the caption when using mouse wheel
	// in full screen mode
	// to prevent the caption from dirtying up the image
	// during scrolling.
	bool caption = fShowCaption;
	if (caption) {
		fShowCaption = false;
		UpdateCaption();
	}

	ScrollBy(x, y);

	if (caption) {
		// show the caption again
		fShowCaption = true;
		UpdateCaption();
	}
}


// XXX method is not unused
void
ShowImageView::ScrollRestrictedTo(float x, float y)
{
	ScrollRestricted(x, y, true);
}


void
ShowImageView::ScrollRestrictedBy(float x, float y)
{
	ScrollRestricted(x, y, false);
}


void
ShowImageView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes != 1) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (*bytes) {
		case B_DOWN_ARROW: 
			ScrollRestrictedBy(0, 10);
			break;
		case B_UP_ARROW: 
			ScrollRestrictedBy(0, -10);
			break;
		case B_LEFT_ARROW: 
			ScrollRestrictedBy(-10, 0);
			break;
		case B_RIGHT_ARROW: 
			ScrollRestrictedBy(10, 0);
			break;
		case B_ENTER:
			NextFile();
			break;
		case B_BACKSPACE:
			PrevFile();
			break;
		case B_HOME:
			break;
		case B_END:
			break;
		case B_SPACE:
			ToggleSlideShow();
			break;
		case B_ESCAPE:
			// stop slide show
			if (fSlideShow)
				ToggleSlideShow();

			ExitFullScreen();

			ClearSelection();
			break;
		case B_DELETE:
		{
			// Move image to Trash
			BMessage trash(BPrivate::kMoveToTrash);
			trash.AddRef("refs", &fCurrentRef);
			// We create our own messenger because the member fTrackerMessenger
			// could be invalid
			BMessenger tracker(kTrackerSignature);
			if (tracker.SendMessage(&trash) == B_OK)
				if (!NextFile()) {
					// This is the last (or only file) in this directory, 
					// close the window
					BMessenger msgr(Window());
					msgr.SendMessage(B_QUIT_REQUESTED);
				}
			break;
		}
		case '+':
		case '=':
			ZoomIn();
			break;
		case '-':
			ZoomOut();
			break;
	}	
}


void
ShowImageView::MouseWheelChanged(BMessage *msg)
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

	ScrollRestrictedBy(x, y);
}


void
ShowImageView::ShowPopUpMenu(BPoint screen)
{
	BPopUpMenu* menu = new PopUpMenu("PopUpMenu", this);

	ShowImageWindow* showImage = dynamic_cast<ShowImageWindow*>(Window());
	if (showImage)
		showImage->BuildContextMenu(menu);

	screen -= BPoint(10, 10);
	menu->Go(screen, true, false, true);
	fShowingPopUpMenu = true;
}


void
ShowImageView::SettingsSetBool(const char* name, bool value)
{
	ShowImageSettings* settings;
	settings = my_app->Settings();
	if (settings->Lock()) {
		settings->SetBool(name, value);
		settings->Unlock();
	}
}


void
ShowImageView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_SELECTION_BITMAP:
		{
			// In response to a B_SIMPLE_DATA message, a view will
			// send this message and expect a reply with a pointer to
			// the currently selected bitmap clip. Although this view
			// allocates the BBitmap * sent in the reply, it is only
			// to be used and deleted by the view that is being replied to.
			BMessage msg;
			msg.AddPointer("be:_bitmap_ptr", CopySelection());
			message->SendReply(&msg);
			break;
		}

		case B_SIMPLE_DATA:
			if (message->WasDropped()) {
				uint32 type;
				int32 count;
				status_t ret = message->GetInfo("refs", &type, &count);
				if (ret == B_OK && type == B_REF_TYPE) {
					// If file was dropped, open it as the selection
					entry_ref ref;
					if (message->FindRef("refs", 0, &ref) == B_OK) {
						BPoint point = message->DropPoint();
						point = ConvertFromScreen(point);
						point = ViewToImage(point);
						SetSelection(&ref, point);
					}
				} else {
					// If a user drags a clip from another ShowImage window,
					// request a BBitmap pointer to that clip, allocated by the
					// other view, for use solely by this view, so that it can
					// be dropped/pasted onto this view.
					BMessenger retMsgr, localMsgr(this);
					retMsgr = message->ReturnAddress();
					if (retMsgr != localMsgr) {
						BMessage msgReply;
						retMsgr.SendMessage(MSG_SELECTION_BITMAP, &msgReply);
						BBitmap *bitmap = NULL;
						if (msgReply.FindPointer("be:_bitmap_ptr",
							reinterpret_cast<void **>(&bitmap)) == B_OK) {
							BRect sourceRect;
							BPoint point, sourcePoint;
							message->FindPoint("be:_source_point", &sourcePoint);
							message->FindRect("be:_frame", &sourceRect);
							point = message->DropPoint();
							point.Set(point.x - (sourcePoint.x - sourceRect.left),
								point.y - (sourcePoint.y - sourceRect.top));
								// adjust drop point before scaling is factored in
							point = ConvertFromScreen(point);
							point = ViewToImage(point);
							
							PasteBitmap(bitmap, point);
						}
					}
				}
			}
			break;
			
		case B_COPY_TARGET:
			HandleDrop(message);
			break;
		case B_MOUSE_WHEEL_CHANGED:
			MouseWheelChanged(message);
			break;
		case MSG_INVALIDATE:
			Invalidate();
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
ShowImageView::FixupScrollBar(orientation o, float bitmapLength, float viewLength)
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
	BRect rctview = Bounds(), rctbitmap(0, 0, 0, 0);
	if (fBitmap) {
		rctbitmap = AlignBitmap();
		rctbitmap.OffsetTo(0, 0);
	}
	
	FixupScrollBar(B_HORIZONTAL, rctbitmap.Width() + 2 * PEN_SIZE, rctview.Width());
	FixupScrollBar(B_VERTICAL, rctbitmap.Height() + 2 * PEN_SIZE, rctview.Height());
}


int32
ShowImageView::CurrentPage()
{
	return fDocumentIndex;
}


int32
ShowImageView::PageCount()
{
	return fDocumentCount;
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
	undoneSelRect = fSelectionRect;
	undoneSelection = CopySelection();
	
	if (undoType == UNDO_UNDO) {
		BBitmap *undoRestore;
		undoRestore = fUndo.GetRestoreBitmap();
		if (undoRestore)
			MergeWithBitmap(undoRestore, fUndo.GetRect());
	}

	// restore previous image/selection
	BBitmap *undoSelection;
	undoSelection = fUndo.GetSelectionBitmap();
		// NOTE: ShowImageView is responsible for deleting this bitmap
		// (Which it will, as it would with a fSelBitmap that it allocated itself)
	if (!undoSelection)
		SetHasSelection(false);
	else {
		SetHasSelection(true);
		fCopyFromRect = BRect();
		fSelectionRect = fUndo.GetRect();
		fSelBitmap = undoSelection;
	}

	fUndo.Undo(undoneSelRect, NULL, undoneSelection);

	Invalidate();
}


void
ShowImageView::AddWhiteRect(BRect &rect)
{
	// Paint white rectangle, using rect, into the background image
	BView view(fBitmap->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap *bitmap = new(nothrow) BBitmap(fBitmap->Bounds(), fBitmap->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		view.DrawBitmap(fBitmap, fBitmap->Bounds());
		
		view.FillRect(rect, B_SOLID_LOW);
			// draw white rect
		
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
		
		DeleteBitmap();
		fBitmap = bitmap;
		
		BMessenger msgr(Window());
		msgr.SendMessage(MSG_MODIFIED);
	} else
		delete bitmap;
}


void
ShowImageView::RemoveSelection(bool toClipboard)
{
	if (!HasSelection())
		return;

	BRect rect = fSelectionRect;
	bool cutBackground = (fSelBitmap) ? false : true;
	BBitmap *selection, *restore = NULL;
	selection = CopySelection();

	if (toClipboard)
		CopySelectionToClipboard();

	SetHasSelection(false);

	if (cutBackground) {
		// If the user hasn't dragged the selection,
		// paint a white rectangle where the selection was
		restore = CopyFromRect(rect);
		AddWhiteRect(rect);
	}

	fUndo.SetTo(rect, restore, selection);
	Invalidate();
}


void
ShowImageView::Cut()
{
	// Copy the selection to the clipboard,
	// then remove it
	RemoveSelection(true);
}


status_t
ShowImageView::PasteBitmap(BBitmap *bitmap, BPoint point)
{
	if (bitmap && bitmap->IsValid()) {
		MergeSelection();

		SetHasSelection(true);
		fSelBitmap = bitmap;
		fCopyFromRect = BRect();
		fSelectionRect = bitmap->Bounds();

		BRect offsetRect = fSelectionRect;
		offsetRect.OffsetBy(point);
		if (fBitmap->Bounds().Intersects(offsetRect))
			// Move the selection rectangle to desired origin,
			// but only if the resulting selection rectangle
			// intersects with the background bitmap rectangle
			fSelectionRect = offsetRect;
		
		Invalidate();
		
		return B_OK;
	}
	
	return B_ERROR;
}


void
ShowImageView::Paste()
{	
	if (be_clipboard->Lock()) {	
		BMessage *pclip;
		if ((pclip = be_clipboard->Data()) != NULL) {
			BPoint point(0, 0);
			pclip->FindPoint("be:location", &point);
			BBitmap *pbits;
			pbits = dynamic_cast<BBitmap *>(BBitmap::Instantiate(pclip));
			PasteBitmap(pbits, point);
		}

		be_clipboard->Unlock(); 
	}
}


void
ShowImageView::SelectAll()
{
	SetHasSelection(true);
	fCopyFromRect.Set(0, 0, fBitmap->Bounds().Width(), fBitmap->Bounds().Height());
	fSelectionRect = fCopyFromRect;
	Invalidate();
}


void
ShowImageView::ClearSelection()
{
	// Remove the selection,
	// DON'T copy it to the clipboard
	RemoveSelection(false);
}


void
ShowImageView::SetHasSelection(bool bHasSelection)
{
	DeleteSelBitmap();
	fHasSelection = bHasSelection;
	
	BMessage msg(MSG_SELECTION);
	msg.AddBool("has_selection", fHasSelection);
	BMessenger msgr(Window());
	msgr.SendMessage(&msg);
}


void
ShowImageView::CopySelectionToClipboard()
{
	if (HasSelection() && be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage *clip = NULL;
		if ((clip = be_clipboard->Data()) != NULL) {
			BMessage data;
			BBitmap* bitmap = CopySelection();
			if (bitmap != NULL) {
				#if 0
				// According to BeBook and Becasso, Gobe Productive do the following.
				// Paste works in Productive, but not in Becasso and original ShowImage.
				BMessage msg(B_OK); // Becasso uses B_TRANSLATOR_BITMAP, BeBook says its unused
				bitmap->Archive(&msg);
				clip->AddMessage("image/x-be-bitmap", &msg);
				#else
				// original ShowImage performs this. Paste works with original ShowImage.
				bitmap->Archive(clip);
				// original ShowImage uses be:location for insertion point
				clip->AddPoint("be:location", BPoint(fSelectionRect.left, fSelectionRect.top));
				#endif
				delete bitmap;
				be_clipboard->Commit();
			}
		}
		be_clipboard->Unlock();
	}
}


void
ShowImageView::FirstPage()
{
	if (fDocumentIndex != 1) {
		fDocumentIndex = 1;
		SetImage(NULL);
	}
}


void
ShowImageView::LastPage()
{
	if (fDocumentIndex != fDocumentCount) {
		fDocumentIndex = fDocumentCount;
		SetImage(NULL);
	}
}


void
ShowImageView::NextPage()
{
	if (fDocumentIndex < fDocumentCount) {
		fDocumentIndex++;
		SetImage(NULL);
	}
}


void
ShowImageView::PrevPage()
{
	if (fDocumentIndex > 1) {
		fDocumentIndex--;
		SetImage(NULL);
	}
}


int 
ShowImageView::CompareEntries(const void* a, const void* b)
{
	entry_ref *r1, *r2;
	r1 = *(entry_ref**)a;
	r2 = *(entry_ref**)b;
	return strcasecmp(r1->name, r2->name);
}


void
ShowImageView::GoToPage(int32 page)
{
	if (page > 0 && page <= fDocumentCount && page != fDocumentIndex) {
		fDocumentIndex = page;
		SetImage(NULL);
	}
}


void
ShowImageView::FreeEntries(BList* entries)
{
	const int32 n = entries->CountItems();
	for (int32 i = 0; i < n; i ++) {
		entry_ref* ref = (entry_ref*)entries->ItemAt(i);
		delete ref;		
	}
	entries->MakeEmpty();
}


void 
ShowImageView::SetTrackerSelectionToCurrent()
{
	BMessage setsel(B_SET_PROPERTY);
	setsel.AddSpecifier("Selection");
	setsel.AddRef("data", &fCurrentRef);
	fTrackerMessenger.SendMessage(&setsel);
}


bool
ShowImageView::FindNextImageByDir(entry_ref *in_current, entry_ref *out_image, bool next, bool rewind)
{
	ASSERT(next || !rewind);
	BEntry curImage(in_current);
	entry_ref entry, *ref;
	BDirectory parent;
	BList entries;
	bool found = false;
	int32 cur;

	if (curImage.GetParent(&parent) != B_OK) {
		return false;
	}

	// insert current ref, so we can find it easily after sorting
	entries.AddItem(in_current);

	while (parent.GetNextRef(&entry) == B_OK) {
		if (entry != *in_current) {
			entries.AddItem(new entry_ref(entry));
		}
	}
	
	entries.SortItems(CompareEntries);
	
	cur = entries.IndexOf(in_current);
	ASSERT(cur >= 0);

	// remove it so FreeEntries() does not delete it
	entries.RemoveItem(in_current);
	
	if (next) {
		// find the next image in the list
		if (rewind) cur = 0; // start with first
		for (; (ref = (entry_ref*)entries.ItemAt(cur)) != NULL; cur ++) {
			if (IsImage(ref)) {
				found = true;
				*out_image = (const entry_ref)*ref;
				break;
			}
		}
	} else {
		// find the previous image in the list
		cur --;
		for (; cur >= 0; cur --) {
			ref = (entry_ref*)entries.ItemAt(cur);
			if (IsImage(ref)) {
				found = true;
				*out_image = (const entry_ref)*ref;
				break;
			}
		}
	}

	FreeEntries(&entries);
	return found;
}

bool
ShowImageView::FindNextImage(entry_ref *in_current, entry_ref *ref, bool next, bool rewind)
{
	// Based on similar function from BeMail!
	if (!fTrackerMessenger.IsValid())
		// If tracker scripting is not available,
		// fall back on directory searching code
		return FindNextImageByDir(in_current, ref, next, rewind);

	//
	//	Ask the Tracker what the next/prev file in the window is.
	//	Continue asking for the next reference until a valid
	//	image is found.
	//
	entry_ref nextRef = *in_current;
	bool foundRef = false;	
	while (!foundRef)
	{
		BMessage request(B_GET_PROPERTY);
		BMessage spc;
		if (rewind)
			spc.what = B_DIRECT_SPECIFIER;
		else if (next)
			spc.what = 'snxt';
		else
			spc.what = 'sprv';
		spc.AddString("property", "Entry");
		if (rewind)
			// if rewinding, ask for the ref to the
			// first item in the directory
			spc.AddInt32("data", 0);
		else
			spc.AddRef("data", &nextRef);
		request.AddSpecifier(&spc);
		
		BMessage reply;
		if (fTrackerMessenger.SendMessage(&request, &reply) != B_OK)
			return FindNextImageByDir(in_current, ref, next, rewind);;
		if (reply.FindRef("result", &nextRef) != B_OK)
			return FindNextImageByDir(in_current, ref, next, rewind);;
			
		if (IsImage(&nextRef))
			foundRef = true;
		
		rewind = false;
			// stop asking for the first ref in the directory
	}

	*ref = nextRef;
	return foundRef;
}

bool
ShowImageView::ShowNextImage(bool next, bool rewind)
{
	entry_ref curRef = fCurrentRef;
	entry_ref imgRef;
	bool found = FindNextImage(&curRef, &imgRef, next, rewind);
	if (found) {
		// Keep trying to load images until:
		// 1. The image loads successfully
		// 2. The last file in the directory is found (for find next or find first)
		// 3. The first file in the directory is found (for find prev)
		// 4. The call to FindNextImage fails for any other reason
		while (SetImage(&imgRef) != B_OK) {
			curRef = imgRef;
			found = FindNextImage(&curRef, &imgRef, next, false);
			if (!found)
				return false;
		}
		SetTrackerSelectionToCurrent();
		return true;
	}
	return false;
}


bool
ShowImageView::NextFile()
{
	return ShowNextImage(true, false);
}


bool
ShowImageView::PrevFile()
{
	return ShowNextImage(false, false);
}


bool
ShowImageView::HasNextFile()
{
	entry_ref ref;
	return FindNextImage(&fCurrentRef, &ref, true, false);
}


bool
ShowImageView::HasPrevFile()
{
	entry_ref ref;
	return FindNextImage(&fCurrentRef, &ref, false, false);
}


bool
ShowImageView::FirstFile()
{
	return ShowNextImage(true, true);
}


void
ShowImageView::SetZoom(float zoom)
{
	if ((fScaleBilinear || fDither) && fZoom != zoom) {
		DeleteScaler();
	}
	fZoom = zoom;
	FixupScrollBars();
	Invalidate();
}


void
ShowImageView::ZoomIn()
{
	if (fZoom < 16)
		SetZoom(fZoom + 0.25);
}


void
ShowImageView::ZoomOut()
{
	if (fZoom > 0.25)
		SetZoom(fZoom - 0.25);
}


void
ShowImageView::SetSlideShowDelay(float seconds)
{
	ShowImageSettings* settings;
	int32 delay = (int)(seconds * 10.0);
	if (fSlideShowDelay != delay) {
		// update counter
		fSlideShowCountDown = delay - (fSlideShowDelay - fSlideShowCountDown);
		if (fSlideShowCountDown <= 0) {
			// show next image on next Pulse()
			fSlideShowCountDown = 1;
		}
		fSlideShowDelay = delay;
		settings = my_app->Settings();
		if (settings->Lock()) {
			settings->SetInt32("SlideShowDelay", fSlideShowDelay);
			settings->Unlock();
		}
	}
}


void
ShowImageView::StartSlideShow()
{
	fSlideShow = true; fSlideShowCountDown = fSlideShowDelay;
}


void
ShowImageView::StopSlideShow()
{
	fSlideShow = false;
}


void
ShowImageView::DoImageOperation(ImageProcessor::operation op, bool quiet) 
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
		ASSERT(ImageProcessor::kRotateClockwise < ImageProcessor::kNumberOfAffineTransformations);
		ASSERT(ImageProcessor::kRotateAntiClockwise < ImageProcessor::kNumberOfAffineTransformations);
		ASSERT(ImageProcessor::kFlipLeftToRight < ImageProcessor::kNumberOfAffineTransformations);
		ASSERT(ImageProcessor::kFlipTopToBottom < ImageProcessor::kNumberOfAffineTransformations);
		fImageOrientation = fTransformation[op][fImageOrientation];
	} else {
		fInverted = !fInverted;
	}

	if (!quiet) {
		// write orientation state
		BNode node(&fCurrentRef);
		int32 orientation = fImageOrientation;
		if (fInverted) orientation += 256;
		if (orientation != k0) {
			node.WriteAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE, B_INT32_TYPE, 0, &orientation, sizeof(orientation));
		} else {
			node.RemoveAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE);
		}
	}

	// set new bitmap
	DeleteBitmap();
	fBitmap = bm; 

	if (!quiet) {
		// remove selection
		SetHasSelection(false);
		// Pull the image type name from fCaption (hackish?)
		int32 last_comma_index = fCaption.FindLast(',');
		if (last_comma_index != B_ERROR) {
			// The -1 at the end is -2 to get rid of the comma and space, +1 for the \0
			int32 name_size = fCaption.CountChars() - last_comma_index - 1;
			char *name = new char[name_size];
			fCaption.CopyInto(name, last_comma_index + 2, name_size - 1);
			name[name_size - 1] = '\0';
			Notify(name);
			delete name;
		}
		else
			Notify(NULL);
	}	
}


//! image operation initiated by user
void
ShowImageView::UserDoImageOperation(ImageProcessor::operation op, bool quiet)
{
	fUndo.Clear();
	DoImageOperation(op, quiet);
}


void
ShowImageView::Rotate(int degree)
{
	if (degree == 90) {
		UserDoImageOperation(ImageProcessor::kRotateClockwise);
	} else if (degree == 270) {
		UserDoImageOperation(ImageProcessor::kRotateAntiClockwise);
	}
}


void
ShowImageView::Flip(bool vertical) 
{
	if (vertical) {
		UserDoImageOperation(ImageProcessor::kFlipLeftToRight);
	} else {
		UserDoImageOperation(ImageProcessor::kFlipTopToBottom);
	}
}


void
ShowImageView::Invert()
{
	if (fBitmap->ColorSpace() != B_CMAP8) {
		// Only allow an invert operation if the
		// bitmap color space is supported by the
		// invert algorithm
		UserDoImageOperation(ImageProcessor::kInvert);
	}
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
	SetHasSelection(false);
	fUndo.Clear();
	DeleteBitmap();
	fBitmap = scaled;

	BMessenger msgr(Window());
	msgr.SendMessage(MSG_MODIFIED);

	Notify(NULL);
}

void
ShowImageView::SetIcon(bool clear, icon_size which)
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
	SetIcon(clear, B_MINI_ICON);
	SetIcon(clear, B_LARGE_ICON);
}


void
ShowImageView::ToggleSlideShow()
{
	BMessenger msgr(Window());
	msgr.SendMessage(MSG_SLIDE_SHOW);
}


void
ShowImageView::ExitFullScreen()
{
	be_app->ShowCursor();
	BMessenger m(Window());
	m.SendMessage(MSG_EXIT_FULL_SCREEN);
}


void 
ShowImageView::WindowActivated(bool active)
{
	fIsActiveWin = active;
	fHideCursorCountDown = HIDE_CURSOR_DELAY_TIME;
}

