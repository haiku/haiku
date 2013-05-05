#include "BitmapView.h"
#include <Alert.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <Font.h>
#include <MenuItem.h>
#include <Entry.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <TranslatorFormats.h>

// TODO: Add support for labels

#define M_REMOVE_IMAGE 'mrmi'
#define M_PASTE_IMAGE 'mpsi'

enum
{
	CLIP_NONE = 0,
	CLIP_BEOS = 1,
	CLIP_SHOWIMAGE = 2,
	CLIP_PRODUCTIVE = 3
};


inline void SetRGBColor(rgb_color *col, uint8 r, uint8 g, uint8 b, uint8 a = 255);


void
SetRGBColor(rgb_color *col, uint8 r, uint8 g, uint8 b, uint8 a)
{
	if (col) {
		col->red = r;
		col->green = g;
		col->blue = b;
		col->alpha = a;
	}
}


BitmapView::BitmapView(BRect frame, const char *name, BMessage *mod, BBitmap *bitmap,
						const char *label, border_style borderstyle, int32 resize, int32 flags)
  :	BView(frame, name, resize, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	if (bitmap && bitmap->IsValid())
		fBitmap = bitmap;
	else
		fBitmap = NULL;
	
	if (mod)
		SetMessage(mod);
	
	fLabel = label;
	fBorderStyle = borderstyle;
	fFixedSize = false;
	fEnabled = true;
	fRemovableBitmap = false;
	fAcceptDrops = true;
	fAcceptPaste = true;
	fConstrainDrops = true;
	fMaxWidth = 100;
	fMaxHeight = 100;
	
	fPopUpMenu = new BPopUpMenu("deletepopup", false, false);
	fPopUpMenu->AddItem(new BMenuItem("Close This Menu", new BMessage(B_CANCEL)));
	fPopUpMenu->AddSeparatorItem();
	
	fPasteItem = new BMenuItem("Paste Photo from Clipboard", new BMessage(M_PASTE_IMAGE));
	fPopUpMenu->AddItem(fPasteItem);
	
	fPopUpMenu->AddSeparatorItem();
	
	fRemoveItem = new BMenuItem("Remove Photo", new BMessage(M_REMOVE_IMAGE));
	fPopUpMenu->AddItem(fRemoveItem);
	
	CalculateBitmapRect();
	
	// Calculate the offsets for each of the words -- the phrase will be center justified
	fNoPhotoWidths[0] = StringWidth("Drop");
	fNoPhotoWidths[1] = StringWidth("a");
	fNoPhotoWidths[2] = StringWidth("Photo");
	fNoPhotoWidths[3] = StringWidth("Here");
	
	font_height fh;
	GetFontHeight(&fh);
	float totalheight = fh.ascent + fh.descent + fh.leading;
	float yoffset = (Bounds().Height() - 10 - (totalheight * 4)) / 2;
	fNoPhotoOffsets[0].Set((Bounds().Width() - fNoPhotoWidths[0]) / 2, totalheight + yoffset);
	fNoPhotoOffsets[1].Set((Bounds().Width() - fNoPhotoWidths[1]) / 2,
							fNoPhotoOffsets[0].y + totalheight);
	fNoPhotoOffsets[2].Set((Bounds().Width() - fNoPhotoWidths[2]) / 2,
							fNoPhotoOffsets[1].y + totalheight);
	fNoPhotoOffsets[3].Set((Bounds().Width() - fNoPhotoWidths[3]) / 2,
							fNoPhotoOffsets[2].y + totalheight);
}


BitmapView::~BitmapView(void)
{
	delete fPopUpMenu;
}


void
BitmapView::AttachedToWindow(void)
{
	SetTarget((BHandler*)Window());
	fPopUpMenu->SetTargetForItems(this);
}


void
BitmapView::SetBitmap(BBitmap *bitmap)
{
	if (bitmap && bitmap->IsValid()) {
		if (fBitmap == bitmap)
			return;
		fBitmap = bitmap;
	} else {
		if (!fBitmap)
			return;
		fBitmap = NULL;
	}
	
	CalculateBitmapRect();
	if (!IsHidden())
		Invalidate();
}


void
BitmapView::SetEnabled(bool value)
{
	if (fEnabled != value) {
		fEnabled = value;
		Invalidate();
	}
}


/*
void
BitmapView::SetLabel(const char *label)
{
	if (fLabel.Compare(label) != 0)	{
		fLabel = label;
		
		CalculateBitmapRect();
		if (!IsHidden())
			Invalidate();
	}
}
*/


void
BitmapView::SetStyle(border_style style)
{
	if (fBorderStyle != style) {
		fBorderStyle = style;
		
		CalculateBitmapRect();
		if (!IsHidden())
			Invalidate();
	}
}


void
BitmapView::SetFixedSize(bool isfixed)
{
	if (fFixedSize != isfixed) {
		fFixedSize = isfixed;
		
		CalculateBitmapRect();
		if (!IsHidden())
			Invalidate();
	}
}


void
BitmapView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped() && AcceptsDrops()) {
		// We'll handle two types of drops: those from Tracker and those from ShowImage
		if (msg->what == B_SIMPLE_DATA) {
			int32 actions;
			if (msg->FindInt32("be:actions", &actions) == B_OK) {
				// ShowImage drop. This is a negotiated drag&drop, so send a reply
				BMessage reply(B_COPY_TARGET), response;
				reply.AddString("be:types", "image/jpeg");
				reply.AddString("be:types", "image/png");
				
				msg->SendReply(&reply, &response);
				
				// now, we've gotten the response
				if (response.what == B_MIME_DATA) {
					// Obtain and translate the received data
					uint8 *imagedata;
					ssize_t datasize;
										
					// Try JPEG first
					if (response.FindData("image/jpeg", B_MIME_DATA, 
						(const void **)&imagedata, &datasize) != B_OK) {
						// Try PNG next and piddle out if unsuccessful
						if (response.FindData("image/png", B_PNG_FORMAT, 
							(const void **)&imagedata, &datasize) != B_OK)
							return;
					}
					
					// Set up to decode into memory
					BMemoryIO memio(imagedata, datasize);
					BTranslatorRoster *roster = BTranslatorRoster::Default();
					BBitmapStream bstream;
					
					if (roster->Translate(&memio, NULL, NULL, &bstream, B_TRANSLATOR_BITMAP) == B_OK)
					{
						BBitmap *bmp;
						if (bstream.DetachBitmap(&bmp) != B_OK)
							return;
						SetBitmap(bmp);
						
						if (fConstrainDrops)
							ConstrainBitmap();
						Invoke();
					}
				}
				return;
			}
			
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				// Tracker drop
				BBitmap *bmp = BTranslationUtils::GetBitmap(&ref);
				if (bmp) {
					SetBitmap(bmp);
					
					if (fConstrainDrops)
						ConstrainBitmap();
					Invoke();
				}
			}
		}
		return;
	}
	
	switch (msg->what)
	{
		case M_REMOVE_IMAGE: {
			BAlert *alert = new BAlert("ResEdit", "This cannot be undone. "
				"Remove the image?", "Remove", "Cancel");
			alert->SetShortcut(1, B_ESCAPE);
			int32 value = alert->Go();
			if (value == 0) {
				SetBitmap(NULL);
				
				if (Target()) {
					BMessenger msgr(Target());
					
					msgr.SendMessage(new BMessage(M_BITMAP_REMOVED));
					return;
				}
			}
		}
		case M_PASTE_IMAGE:
		{
			PasteBitmap();
			Invoke();
		}
	}
	BView::MessageReceived(msg);
}


void
BitmapView::Draw(BRect rect)
{
	if (fBitmap)
		DrawBitmap(fBitmap, fBitmap->Bounds(), fBitmapRect);
	else {
		SetHighColor(0, 0, 0, 80);
		SetDrawingMode(B_OP_ALPHA);
		DrawString("Drop", fNoPhotoOffsets[0]);
		DrawString("a", fNoPhotoOffsets[1]);
		DrawString("Photo", fNoPhotoOffsets[2]);
		DrawString("Here", fNoPhotoOffsets[3]);
		SetDrawingMode(B_OP_COPY);
	}
	
	if (fBorderStyle == B_FANCY_BORDER) {
		rgb_color base= { 216, 216, 216, 255 };
		rgb_color work;
		
		SetHighColor(base);
		StrokeRect(Bounds().InsetByCopy(2, 2));
		
		BeginLineArray(12);
		
		BRect r(Bounds());

		work = tint_color(base, B_DARKEN_2_TINT);
		AddLine(r.LeftTop(), r.RightTop(), work);
		AddLine(r.LeftTop(), r.LeftBottom(), work);
		r.left++;
		
		work = tint_color(base, B_DARKEN_4_TINT);
		AddLine(r.RightTop(), r.RightBottom(), work);
		AddLine(r.LeftBottom(), r.RightBottom(), work);
		
		r.right--;
		r.top++;
		r.bottom--;

		
		work = tint_color(base, B_LIGHTEN_MAX_TINT);
		AddLine(r.LeftTop(), r.RightTop(), work);
		AddLine(r.LeftTop(), r.LeftBottom(), work);
		r.left++;
		
		work = tint_color(base, B_DARKEN_3_TINT);
		AddLine(r.RightTop(), r.RightBottom(), work);
		AddLine(r.LeftBottom(), r.RightBottom(), work);
		
		// this rect handled by the above StrokeRect, so inset a total of 2 pixels
		r.left++;
		r.right -= 2;
		r.top += 2;
		r.bottom -= 2;
		
		
		work = tint_color(base, B_DARKEN_3_TINT);
		AddLine(r.LeftTop(), r.RightTop(), work);
		AddLine(r.LeftTop(), r.LeftBottom(), work);
		r.left++;
		
		work = tint_color(base, B_LIGHTEN_MAX_TINT);
		AddLine(r.RightTop(), r.RightBottom(), work);
		AddLine(r.LeftBottom(), r.RightBottom(), work);

		r.right--;
		r.top++;
		r.bottom--;
		EndLineArray();
		
		SetHighColor(tint_color(base, B_DARKEN_2_TINT));
		StrokeRect(r);
	} else {
		// Plain border
		SetHighColor(0, 0, 0);	
		StrokeRect(fBitmapRect);
	}
}


void
BitmapView::MouseDown(BPoint pt)
{
	BPoint mousept;
	uint32 buttons;
	
	GetMouse(&mousept, &buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		ConvertToScreen(&mousept);
		
		mousept.x= (mousept.x>5) ? mousept.x-5 : 0;
		mousept.y= (mousept.y>5) ? mousept.y-5 : 0;
		
		if (AcceptsPaste() && ClipboardHasBitmap())
			fPasteItem->SetEnabled(true);
		else
			fPasteItem->SetEnabled(false);
		
		if (fRemovableBitmap && fBitmap)
			fRemoveItem->SetEnabled(true);
		else
			fRemoveItem->SetEnabled(false);
		
		fPopUpMenu->Go(mousept, true, true, true);
	}
}


void
BitmapView::FrameResized(float w, float h)
{
	CalculateBitmapRect();
}


void
BitmapView::CalculateBitmapRect(void)
{
	if (!fBitmap || fFixedSize) {
		fBitmapRect = Bounds().InsetByCopy(1, 1);
		return;
	}
	
	uint8 borderwidth = (fBorderStyle == B_FANCY_BORDER) ? 5 : 1;

	BRect r(Bounds());
	fBitmapRect= ScaleRectToFit(fBitmap->Bounds(), r.InsetByCopy(borderwidth, borderwidth));
}


void
BitmapView::SetAcceptDrops(bool accept)
{
	fAcceptDrops = accept;
}


void
BitmapView::SetAcceptPaste(bool accept)
{
	fAcceptPaste = accept;
}


void
BitmapView::SetConstrainDrops(bool value)
{
	fConstrainDrops = value;
}


void
BitmapView::MaxBitmapSize(float *width, float *height) const
{
	*width = fMaxWidth;
	*height = fMaxHeight;
}


void
BitmapView::SetMaxBitmapSize(const float &width, const float &height)
{
	fMaxWidth = width;
	fMaxHeight = height;
	
	ConstrainBitmap();
}


void
BitmapView::SetBitmapRemovable(bool isremovable)
{
	fRemovableBitmap = isremovable;
}


void
BitmapView::ConstrainBitmap(void)
{
	if (!fBitmap || fMaxWidth < 1 || fMaxHeight < 1)
		return;
	
	BRect r = ScaleRectToFit(fBitmap->Bounds(), BRect(0, 0, fMaxWidth - 1, fMaxHeight - 1));
	r.OffsetTo(0, 0);
	
	BBitmap *scaled = new BBitmap(r, fBitmap->ColorSpace(), true);
	BView *view = new BView(r, "drawview", 0, 0);
	
	scaled->Lock();
	scaled->AddChild(view);
	view->DrawBitmap(fBitmap, fBitmap->Bounds(), scaled->Bounds());
	scaled->Unlock();
	
	delete fBitmap;
	fBitmap = new BBitmap(scaled, false);
}


bool
BitmapView::ClipboardHasBitmap(void)
{
	BMessage *clip = NULL, flattened;
	uint8 clipval = CLIP_NONE;
	bool returnval;
	
	if (be_clipboard->Lock()) {
		clip = be_clipboard->Data();
		if (!clip->IsEmpty()) {
			returnval = (clip->FindMessage("image/bitmap", &flattened) == B_OK);
			if (returnval)
				clipval = CLIP_BEOS;
			else {
				BString string;
				returnval = (clip->FindString("class", &string) == B_OK && string == "BBitmap");
				
				// Try method Gobe Productive uses if that, too, didn't work
				if (returnval)
					clipval = CLIP_SHOWIMAGE;
				else {
					returnval = (clip->FindMessage("image/x-vnd.Be-bitmap", &flattened) == B_OK);
					if (returnval)
						clipval = CLIP_SHOWIMAGE;
					else
						clipval = CLIP_NONE;
				}
			}
		}
		be_clipboard->Unlock();
	}
	return (clipval != CLIP_NONE)?true:false;
}


BBitmap *
BitmapView::BitmapFromClipboard(void)
{
	BMessage *clip = NULL, flattened;
	BBitmap *bitmap;
	
	if (!be_clipboard->Lock()) 
		return NULL;
	
	clip = be_clipboard->Data();
	if (!clip)
		return NULL;
	
	uint8 clipval = CLIP_NONE;
	
	// Try ArtPaint-style storage
	status_t status = clip->FindMessage("image/bitmap", &flattened);
	
	// If that didn't work, try ShowImage-style
	if (status != B_OK) {
		BString string;
		status = clip->FindString("class", &string);
		
		// Try method Gobe Productive uses if that, too, didn't work
		if (status == B_OK && string == "BBitmap")
			clipval = CLIP_SHOWIMAGE;
		else {
			status = clip->FindMessage("image/x-vnd.Be-bitmap", &flattened);
			if (status == B_OK)
				clipval = CLIP_PRODUCTIVE;
			else
				clipval = CLIP_NONE;
		}
	}
	else
		clipval = CLIP_BEOS;
	
	be_clipboard->Unlock();
	
	switch (clipval) {
		case CLIP_SHOWIMAGE: {
			// Showimage does it a slightly different way -- it dumps the BBitmap
			// data directly to the clipboard message instead of packaging it in
			// a bitmap like everyone else.
			
			if (!be_clipboard->Lock())
				return NULL;
			
			BMessage datamsg(*be_clipboard->Data());
			
			be_clipboard->Unlock();
			
			const void *buffer;
			int32 bufferLength;
			
			BRect frame;
			color_space cspace = B_NO_COLOR_SPACE;
			
			status = datamsg.FindRect("_frame", &frame);
			if (status != B_OK)
				return NULL;
			
			status = datamsg.FindInt32("_cspace", (int32)cspace);
			if (status != B_OK)
				return NULL;
			cspace = B_RGBA32;
			bitmap = new BBitmap(frame, cspace, true);
			
			status = datamsg.FindData("_data", B_RAW_TYPE, (const void **)&buffer, &bufferLength);
			if (status != B_OK) {
				delete bitmap;
				return NULL;
			}
			
			memcpy(bitmap->Bits(), buffer, bufferLength);
			return bitmap;
		}
		case CLIP_PRODUCTIVE:
		// Productive doesn't name the packaged BBitmap data message the same, but
		// uses exactly the same data format.
		
		case CLIP_BEOS: {
			const void *buffer;
			int32 bufferLength;
			
			BRect frame;
			color_space cspace = B_NO_COLOR_SPACE;
			
			status = flattened.FindRect("_frame", &frame);
			if (status != B_OK)
				return NULL;
			
			status = flattened.FindInt32("_cspace", (int32)cspace);
			if (status != B_OK)
				return NULL;
			cspace = B_RGBA32;
			bitmap = new BBitmap(frame, cspace, true);
			
			status = flattened.FindData("_data", B_RAW_TYPE, (const void **)&buffer, &bufferLength);
			if (status != B_OK) {
				delete bitmap;
				return NULL;
			}
			
			memcpy(bitmap->Bits(), buffer, bufferLength);
			return bitmap;
		}
		default:
			return NULL;
	}
	
	// shut the compiler up
	return NULL;
}


BRect
ScaleRectToFit(const BRect &from, const BRect &to)
{
	// Dynamic sizing algorithm
	// 1) Check to see if either dimension is bigger than the view's display area
	// 2) If smaller along both axes, make bitmap rect centered and return
	// 3) Check to see if scaling is to be horizontal or vertical on basis of longer axis
	// 4) Calculate scaling factor
	// 5) Scale both axes down by scaling factor, accounting for border width
	// 6) Center the rectangle in the direction of the smaller axis
	
	if (!to.IsValid())
		return from;
	if (!from.IsValid())
		return to;
	
	BRect r(to);
	
	if ((from.Width() <= r.Width()) && (from.Height() <= r.Height())) {
		// Smaller than view, so just center and return
		r = from;
		r.OffsetBy((to.Width() - r.Width()) / 2, (to.Height() - r.Height()) / 2);
		return r;
	}
	
	float multiplier = from.Width()/from.Height();
	if (multiplier > 1)	{
		// Landscape orientation
		
		// Scale rectangle to bounds width and center height
		r.bottom = r.top + (r.Width() / multiplier);
		r.OffsetBy(0, (to.Height() - r.Height()) / 2);
	} else {
		// Portrait orientation

		// Scale rectangle to bounds height and center width
		r.right = r.left + (r.Height() * multiplier);
		r.OffsetBy((to.Width() - r.Width()) / 2, 0);
	}
	return r;
}


void
BitmapView::RemoveBitmap(void)
{
	SetBitmap(NULL);
}


void
BitmapView::PasteBitmap(void)
{
	BBitmap *bmp = BitmapFromClipboard();
	if (bmp)
		SetBitmap(bmp);
	
	if (fConstrainDrops)
		ConstrainBitmap();
}
