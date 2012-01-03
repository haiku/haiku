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

#include <Alert.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Directory.h>
#include <File.h>
#include <FilePanel.h>
#include <IconUtils.h>
#include <LayoutUtils.h>
#include <PopUpMenu.h>
#include <DataIO.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <String.h>
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
	fShowingPopUpMenu(false),
	fPictureType(0),
	fFocusChanging(false),
	fOpenPanel(new BFilePanel(B_OPEN_PANEL))
{
	SetViewColor(255, 255, 255);

	SetToolTip(B_TRANSLATE(
		"Drop an image here,\n"
		"or use the contextual menu."));

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

	delete fOpenPanel;
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
PictureView::Update()
{
	if (fOriginalPicture != fPicture) {
		delete fOriginalPicture;
		fOriginalPicture = fPicture;
	}
}


void
PictureView::Update(const entry_ref* ref)
{
	// Don't update when user has modified the picture
	if (HasChanged())
		return;

	if (_LoadPicture(ref) == B_OK) {
		delete fOriginalPicture;
		fOriginalPicture = fPicture;
	}
}


BBitmap*
PictureView::Bitmap()
{
	return fPicture;
}


uint32
PictureView::SuggestedType()
{
	return fPictureType;
}


const char*
PictureView::SuggestedMIMEType()
{
	if (fPictureMIMEType == "")
		return NULL;

	return fPictureMIMEType.String();
}


void
PictureView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK
				&& _LoadPicture(&ref) == B_OK)
				MakeFocus(true);
			else
				_HandleDrop(message);
			break;
		}

		case B_MIME_DATA:
			// TODO
			break;

		case B_COPY_TARGET:
			_HandleDrop(message);
			break;

		case B_PASTE:
		{
			if (be_clipboard->Lock() != B_OK)
				break;

			BMessage* data = be_clipboard->Data();
			BMessage archivedBitmap;
			if (data->FindMessage("image/bitmap", &archivedBitmap) == B_OK) {
				BBitmap* picture = new(std::nothrow) BBitmap(&archivedBitmap);
				_SetPicture(picture);
			}

			be_clipboard->Unlock();
			break;
		}

		case B_DELETE:
		case B_TRASH_TARGET:
			_SetPicture(NULL);
			break;

		case kMsgLoadImage:
			fOpenPanel->SetTarget(BMessenger(this));
			fOpenPanel->Show();
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

	// Draw the outer frame
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (IsFocus() && Window() && Window()->IsActive())
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		SetHighColor(tint_color(base, B_DARKEN_3_TINT));
	StrokeRect(rect);

	if (fFocusChanging) {
		// focus frame is already redraw, stop here
		return;
	}

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

 		DrawBitmapAsync(picture, srcRect, fPictureRect,
 			B_FILTER_BITMAP_BILINEAR);

		SetDrawingMode(B_OP_OVER);
	}
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
	if (focused == IsFocus())
		return;

	BView::MakeFocus(focused);

	if (Window()) {
		fFocusChanging = true;
		Invalidate();
		Flush();
		fFocusChanging = false;
	}
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
	if (roster == NULL) {
		delete bitmap;
		return;
	}

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
PictureView::_HandleDrop(BMessage* msg)
{
	entry_ref dirRef;
	BString name, type;
	bool saveToFile = msg->FindString("be:filetypes", &type) == B_OK
		&& msg->FindRef("directory", &dirRef) == B_OK
		&& msg->FindString("name", &name) == B_OK;

	bool sendInMessage = !saveToFile
		&& msg->FindString("be:types", &type) == B_OK;

	if (!sendInMessage && !saveToFile)
		return;

	BBitmap* bitmap = fPicture;
	if (bitmap == NULL)
		return;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return;

	BBitmapStream stream(bitmap);

	// find translation format we're asked for
	translator_info* outInfo;
	int32 outNumInfo;
	bool found = false;
	translation_format format;

	if (roster->GetTranslators(&stream, NULL, &outInfo, &outNumInfo) == B_OK) {
		for (int32 i = 0; i < outNumInfo; i++) {
			const translation_format* formats;
			int32 formatCount;
			roster->GetOutputFormats(outInfo[i].translator, &formats,
					&formatCount);
			for (int32 j = 0; j < formatCount; j++) {
				if (strcmp(formats[j].MIME, type.String()) == 0) {
					format = formats[j];
					found = true;
					break;
				}
			}
		}
	}

	if (!found) {
		stream.DetachBitmap(&bitmap);
		return;
	}

	if (sendInMessage) {

		BMessage reply(B_MIME_DATA);
		BMallocIO memStream;
		if (roster->Translate(&stream, NULL, NULL, &memStream,
			format.type) == B_OK) {
			reply.AddData(format.MIME, B_MIME_TYPE, memStream.Buffer(),
				memStream.BufferLength());
			msg->SendReply(&reply);
		}

	} else {

		BDirectory dir(&dirRef);
		BFile file(&dir, name.String(), B_WRITE_ONLY | B_CREATE_FILE
			| B_ERASE_FILE);

		if (file.InitCheck() == B_OK
			&& roster->Translate(&stream, NULL, NULL, &file,
				format.type) == B_OK) {
			BNodeInfo nodeInfo(&file);
			if (nodeInfo.InitCheck() == B_OK)
				nodeInfo.SetType(type.String());
		} else {
			BString text = B_TRANSLATE("The file '%name%' could not "
				"be written.");
			text.ReplaceFirst("%name%", name);
			BAlert* alert = new BAlert(B_TRANSLATE("Error"), text.String(),
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go();
		}
	}

	// Detach, as we don't want our fPicture to be deleted
	stream.DetachBitmap(&bitmap);
}


status_t
PictureView::_LoadPicture(const entry_ref* ref)
{
	BFile file;
	status_t status = file.SetTo(ref, B_READ_ONLY);
	if (status != B_OK)
		return status;

	off_t fileSize;
	status = file.GetSize(&fileSize);
	if (status != B_OK)
		return status;

	// Check that we've at least some data to translate...
	if (fileSize < 1)
		return B_OK;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return B_ERROR;

	status = roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP);

	BBitmapStream stream;

	if (status == B_OK) {
		status = roster->Translate(&file, &info, &ioExtension, &stream,
			B_TRANSLATOR_BITMAP);
	}
	if (status != B_OK)
		return status;

	BBitmap* picture = NULL;
	if (stream.DetachBitmap(&picture) != B_OK
		|| picture == NULL)
		return B_ERROR;

	// Remember image format so we could store using the same
	fPictureMIMEType = info.MIME;
	fPictureType = info.type;

	_SetPicture(picture);
	return B_OK;
}


void
PictureView::_SetPicture(BBitmap* picture)
{
	if (fPicture != fOriginalPicture)
		delete fPicture;

	fPicture = picture;

	if (picture == NULL) {
		fPictureType = 0;
		fPictureMIMEType = "";
	}

	Invalidate();
}


