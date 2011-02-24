/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "PictureView.h"

#include <math.h>
#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <LayoutUtils.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MimeType.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "PeopleApp.h"	// for B_PERSON_MIMETYPE


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "People"


const uint32 kMsgPopUpMenuClosed = 'pmcl';

class PopUpMenu : public BPopUpMenu {
public:
							PopUpMenu(const char* name, BMessenger target);
	virtual 				~PopUpMenu();

private:
		BMessenger 			fTarget;
};


PopUpMenu::PopUpMenu(const char* name, BMessenger target)
	:
	BPopUpMenu(name, false, false),	fTarget(target)
{
	SetAsyncAutoDestruct(true);
}


PopUpMenu::~PopUpMenu()
{
	fTarget.SendMessage(kMsgPopUpMenuClosed);
}


// #pragma mark -

using std::nothrow;


const float kPictureMargin = 6.0;

PictureView::PictureView(float width, float height, const entry_ref* ref)
	:
	BView("pictureview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
	fPicture(NULL),
	fOriginalPicture(NULL),
	fDefaultPicture(NULL),
	fShowingPopUpMenu(false)
{
	SetViewColor(255, 255, 255);

	BSize size(width + 2 * kPictureMargin, height + 2 * kPictureMargin);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);

	BMimeType mime(B_PERSON_MIMETYPE);
	uint8* iconData;
	size_t iconDataSize;
	if (mime.GetIcon(&iconData, &iconDataSize) == B_OK) {
		float size = width < height ? width : height;
		fDefaultPicture = new BBitmap(BRect(0, 0, size, size),
			B_RGB32);
		if (fDefaultPicture->InitCheck() != B_OK
			|| BIconUtils::GetVectorIcon(iconData, iconDataSize,
				fDefaultPicture) != B_OK) {
			delete fDefaultPicture;
			fDefaultPicture = NULL;
		}
	}

	Update(ref);
}


PictureView::~PictureView()
{
	delete fDefaultPicture;
	delete fPicture;
	if (fOriginalPicture != fPicture)
		delete fOriginalPicture;
}


bool
PictureView::HasChanged()
{
	return fPicture != fOriginalPicture;
}


void
PictureView::Revert()
{
	if (!HasChanged())
		return;

	_SetPicture(fOriginalPicture);
}


void
PictureView::Update(const entry_ref* ref)
{
	// Don't update when user has modified the picture
	if (HasChanged())
		return;

	BBitmap* bitmap = BTranslationUtils::GetBitmap(ref);
	_SetPicture(bitmap);

	delete fOriginalPicture;
	fOriginalPicture = fPicture;
}


BBitmap*
PictureView::Bitmap()
{
	return fPicture;
}


void
PictureView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BBitmap* picture = BTranslationUtils::GetBitmap(&ref);
			if (picture == NULL)
				break;

			_SetPicture(picture);
			MakeFocus(true);
			break;
		}

		case B_COPY_TARGET:
			_HandleDrop(message);
			break;

		case B_DELETE:
		case B_TRASH_TARGET:
			_SetPicture(NULL);
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
PictureView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	BBitmap* picture = fPicture ? fPicture : fDefaultPicture;
	if (picture != NULL) {
		// scale to fit and center picture in frame
		BRect frame = rect.InsetByCopy(kPictureMargin, kPictureMargin);
		BRect srcRect = picture->Bounds();
		BSize size = frame.Size();
		if (srcRect.Width() > srcRect.Height())
			size.height = srcRect.Height() * size.width / srcRect.Width();
		else
			size.width = srcRect.Width() * size.height / srcRect.Height();

		fPictureRect = BLayoutUtils::AlignInFrame(frame, size,
			BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));

		SetDrawingMode(B_OP_ALPHA);
		if (picture == fDefaultPicture) {
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetHighColor(0, 0, 0, 24);
		}

 		DrawBitmapAsync(picture, srcRect, fPictureRect, B_FILTER_BITMAP_BILINEAR);

		SetDrawingMode(B_OP_OVER);
	}

	// Draw the outer frame
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (IsFocus() && Window() && Window()->IsActive())
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		SetHighColor(tint_color(base, B_DARKEN_3_TINT));
	StrokeRect(rect);
}


void
PictureView::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
PictureView::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);

	Invalidate();
}


void
PictureView::MouseDown(BPoint position)
{
	MakeFocus(true);

	uint32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		buttons = Window()->CurrentMessage()->FindInt32("buttons");

	if (fPicture != NULL && fPictureRect.Contains(position)
		&& (buttons
			& (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON)) != 0) {

		_BeginDrag(position);

	} else if (buttons == B_SECONDARY_MOUSE_BUTTON)
		_ShowPopUpMenu(ConvertToScreen(position));
}


void
PictureView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes != 1) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (*bytes) {
		case B_DELETE:
			_SetPicture(NULL);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// #pragma mark -


void
PictureView::_ShowPopUpMenu(BPoint screen)
{
	if (fShowingPopUpMenu)
		return;

	PopUpMenu* menu = new PopUpMenu("PopUpMenu", this);

	BMenuItem* item = new BMenuItem(B_TRANSLATE("Load image" B_UTF8_ELLIPSIS),
		new BMessage(kMsgLoadImage));
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Remove image"), new BMessage(B_DELETE));
	item->SetEnabled(fPicture != NULL);
	menu->AddItem(item);

	menu->SetTargetForItems(this);
	menu->Go(screen, true, true, true);
	fShowingPopUpMenu = true;
}


BBitmap*
PictureView::_CopyPicture(uint8 alpha)
{
	bool hasAlpha = alpha != 255;

	if (!fPicture)
		return NULL;

	BRect rect = fPictureRect.OffsetToCopy(B_ORIGIN);
	BView view(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	BBitmap* bitmap = new(nothrow) BBitmap(rect, hasAlpha ? B_RGBA32
		: fPicture->ColorSpace(), true);
	if (bitmap == NULL || !bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	if (bitmap->Lock()) {
		bitmap->AddChild(&view);
		if (hasAlpha) {
			view.SetHighColor(0, 0, 0, 0);
			view.FillRect(rect);
			view.SetDrawingMode(B_OP_ALPHA);
			view.SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			view.SetHighColor(0, 0, 0, alpha);
		}
		view.DrawBitmap(fPicture, fPicture->Bounds().OffsetToCopy(B_ORIGIN),
			rect, B_FILTER_BITMAP_BILINEAR);
		view.Sync();
		bitmap->RemoveChild(&view);
		bitmap->Unlock();
	}

	return bitmap;
}


void
PictureView::_BeginDrag(BPoint sourcePoint)
{
	BBitmap* bitmap = _CopyPicture(128);
	if (bitmap == NULL)
		return;

	// fill the drag message
	BMessage drag(B_SIMPLE_DATA);
	drag.AddInt32("be:actions", B_COPY_TARGET);
	drag.AddInt32("be:actions", B_TRASH_TARGET);

	// name the clip after person name, if any
	BString name = B_TRANSLATE("%name% picture");
	name.ReplaceFirst("%name%", Window() ? Window()->Title() :
		B_TRANSLATE("Unnamed person"));
	drag.AddString("be:clip_name", name.String());

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return;

	int32 infoCount;
	translator_info* info;
	BBitmapStream stream(bitmap);
	if (roster->GetTranslators(&stream, NULL, &info, &infoCount) == B_OK) {
		for (int32 i = 0; i < infoCount; i++) {
			const translation_format* formats;
			int32 count;
			roster->GetOutputFormats(info[i].translator, &formats, &count);
			for (int32 j = 0; j < count; j++) {
				if (strcmp(formats[j].MIME, "image/x-be-bitmap") != 0) {
					// needed to send data in message
					drag.AddString("be:types", formats[j].MIME);
					// needed to pass data via file
					drag.AddString("be:filetypes", formats[j].MIME);
					drag.AddString("be:type_descriptions", formats[j].name);
				}
			}
		}
	}
	stream.DetachBitmap(&bitmap);

	// we also support "Passing Data via File" protocol
	drag.AddString("be:types", B_FILE_MIME_TYPE);

	sourcePoint -= fPictureRect.LeftTop();

	SetMouseEventMask(B_POINTER_EVENTS);

	DragMessage(&drag, bitmap, B_OP_ALPHA, sourcePoint);
	bitmap = NULL;
}


void
PictureView::_HandleDrop(BMessage* message)
{
	// TODO
	printf("PictureView::_HandleDrop(): \n");
	message->PrintToStream();
}


void
PictureView::_SetPicture(BBitmap* picture)
{
	if (fPicture != fOriginalPicture)
		delete fPicture;

	fPicture = picture;
	Invalidate();
}


