/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "MimeTypeListView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <be_apps/Tracker/RecentItems.h>

#include <stdio.h>


const uint32 kMsgTypeSelected = 'typs';


class IconView : public BControl {
	public:
		IconView(BRect rect, const char* name, BMessage* message);
		virtual ~IconView();

		void SetTo(BMimeType* type);

		virtual void Draw(BRect updateRect);

#if 0
		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit,
			BMessage* dragMessage);
#endif

	private:
		BBitmap*	fIcon;
};


IconView::IconView(BRect rect, const char* name, BMessage* message)
	: BControl(rect, name, NULL, message,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fIcon(NULL)
{
}


IconView::~IconView()
{
	delete fIcon;
}


void
IconView::SetTo(BMimeType* type)
{
	bool hadIcon = fIcon != NULL;

	if (type != NULL) {
		if (fIcon == NULL)
			fIcon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_CMAP8);
		
		if (type->GetIcon(fIcon, B_LARGE_ICON) == B_OK) {
			Invalidate();
			return;
		}
	}

	delete fIcon;
	fIcon = NULL;

	if (hadIcon)
		Invalidate();
}


void
IconView::Draw(BRect updateRect)
{
	if (fIcon != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon, BPoint(0, 0));
	} else if (IsEnabled()) {
		BRect rect = Bounds();
	
		rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
		rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);
	
		BeginLineArray(8);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
				BPoint(rect.right, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), light);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
				BPoint(rect.right, rect.top + 1.0f), light);

		rect.InsetBy(1.0, 1.0);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), light);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
				BPoint(rect.right, rect.top), light);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), shadow);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
				BPoint(rect.right, rect.top + 1.0f), shadow);

		EndLineArray();
	} else {
		SetHighColor(ViewColor());
		FillRect(updateRect);
	}
}


//	#pragma mark -


FileTypesWindow::FileTypesWindow(BRect frame)
	: BWindow(frame, "FileTypes", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New Resource File" B_UTF8_ELLIPSIS,
		NULL, 'N', B_COMMAND_KEY));

	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, kSignature);
	BMenuItem* item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel));
	item->SetShortcut('O', B_COMMAND_KEY);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Application Types" B_UTF8_ELLIPSIS,
		new BMessage));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About FileTypes" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	item->SetTarget(this);
	menuBar->AddItem(menu);

	// MIME Types list

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1.0f;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BButton* button = new BButton(rect, "add", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(8.0f, topView->Bounds().bottom - 8.0f - button->Bounds().Height());
	topView->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 8.0f, 0.0f);
	fRemoveTypeButton = new BButton(rect, "remove", "Remove", NULL, B_FOLLOW_BOTTOM);
	fRemoveTypeButton->ResizeToPreferred();
	topView->AddChild(fRemoveTypeButton);

	rect.bottom = rect.top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 2.0f;
	if (rect.right < 180)
		rect.right = 180;

	fTypeListView = new MimeTypeListView(rect, "listview",
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fTypeListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	// "Icon" group

	BFont font(be_bold_font);
	float labelWidth = font.StringWidth("Icon");
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.right = rect.left + max_c(B_LARGE_ICON, labelWidth) + 32.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ max_c(B_LARGE_ICON, button->Bounds().Height() * 2.0f + 4.0f) + 12.0f;
	rect.top -= 2.0f;
	BBox* box = new BBox(rect);
	box->SetLabel("Icon");
	topView->AddChild(box);

	BRect innerRect = BRect(0.0f, 0.0f, B_LARGE_ICON - 1.0f, B_LARGE_ICON - 1.0f);
	innerRect.OffsetTo((rect.Width() - innerRect.Width()) / 2.0f,
		fontHeight.ascent / 2.0f
		+ (rect.Height() - fontHeight.ascent / 2.0f - innerRect.Height()) / 2.0f);
	fIconView = new IconView(innerRect, "icon box", NULL);
	box->AddChild(fIconView);

	// "File Extensions" group

	BRect rightRect(rect);
	rightRect.left = rect.right + 8.0f;
	rightRect.right = topView->Bounds().Width() - 8.0f;
	box = new BBox(rightRect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("File Extensions");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddExtensionButton = new BButton(innerRect, "add ext", "Add" B_UTF8_ELLIPSIS,
		NULL, B_FOLLOW_RIGHT);
	box->AddChild(fAddExtensionButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveExtensionButton = new BButton(innerRect, "remove ext", "Remove", NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fRemoveExtensionButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom -= 2.0f;
		// take scrollview border into account
	fExtensionListView = new BListView(innerRect, "listview ext",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT);
	scrollView = new BScrollView("scrollview ext", fExtensionListView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	// "Description" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent) + 19.0f;
	rect.right = rightRect.right;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Description");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fInternalNameControl = new BTextControl(innerRect, "internal", "Internal Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	labelWidth = fInternalNameControl->StringWidth(fInternalNameControl->Label()) + 2.0f;
	fInternalNameControl->SetDivider(labelWidth);
	fInternalNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fInternalNameControl->SetEnabled(false);
	box->ResizeBy(0, fInternalNameControl->Bounds().Height() * 2.0f);
	box->AddChild(fInternalNameControl);

	innerRect.OffsetBy(0, fInternalNameControl->Bounds().Height() + 5.0f);
	fTypeNameControl = new BTextControl(innerRect, "type", "Type Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	fTypeNameControl->SetDivider(labelWidth);
	fTypeNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	box->AddChild(fTypeNameControl);

	// "Preferred Application" group

	rect = box->Frame();
	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ button->Bounds().Height() + 14.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Preferred Application");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Same As" B_UTF8_ELLIPSIS) - 24.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fSameAsButton = new BButton(innerRect, "same as", "Same As" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fSameAsButton);

	innerRect.OffsetBy(-innerRect.Width() - 6.0f, 0.0f);
	fSelectButton = new BButton(innerRect, "select", "Select" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fSelectButton);

	menu = new BPopUpMenu("preferred");
	menu->AddItem(item = new BMenuItem("None", NULL));
	item->SetMarked(true);

	innerRect.right = innerRect.left - 6.0f;
	innerRect.left = 8.0f;
	fPreferredField = new BMenuField(innerRect, "preferred", NULL, menu,
		B_FOLLOW_LEFT_RIGHT);
	float width, height;
	fPreferredField->GetPreferredSize(&width, &height);
	fPreferredField->ResizeTo(innerRect.Width(), height);
	fPreferredField->MoveBy(0.0f, (innerRect.Height() - height) / 2.0f);
	box->AddChild(fPreferredField);

	// "Extra Attributes" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = topView->Bounds().Height() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel("Extra Attributes");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddAttributeButton = new BButton(innerRect, "add attr", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fAddAttributeButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveAttributeButton = new BButton(innerRect, "remove attr", "Remove",
		NULL, B_FOLLOW_RIGHT);
	box->AddChild(fRemoveAttributeButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom = box->Bounds().bottom - 10.0f;
		// take scrollview border into account
	BListView* listView = new BListView(innerRect, "listview attr",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	scrollView = new BScrollView("scrollview attr", listView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	SetSizeLimits(rightRect.left + 72.0f + font.StringWidth("File Extensions"), 32767.0f,
		rect.top + 2.0f * button->Bounds().Height() + fontHeight.ascent + 32.0f
		+ menuBar->Bounds().Height(), 32767.0f);

	_SetType(NULL);
}


FileTypesWindow::~FileTypesWindow()
{
}


void
FileTypesWindow::_UpdateExtensions(BMimeType* type)
{
	// clear list

	for (int32 i = fExtensionListView->CountItems(); i-- > 0;) {
		delete fExtensionListView->ItemAt(i);
	}
	fExtensionListView->MakeEmpty();

	// fill it again
	
	if (type == NULL)
		return;

	BMessage extensions;
	if (type->GetFileExtensions(&extensions) != B_OK)
		return;

	const char* extension;
	int32 i = 0;
	while (extensions.FindString("extensions", i++, &extension) == B_OK) {
		char dotExtension[B_FILE_NAME_LENGTH];
		snprintf(dotExtension, B_FILE_NAME_LENGTH, ".%s", extension);

		fExtensionListView->AddItem(new BStringItem(dotExtension));
	}
}


void
FileTypesWindow::_UpdatePreferredApps(BMimeType* type)
{
	// clear menu

	BMenu* menu = fPreferredField->Menu();
	for (int32 i = menu->CountItems(); i-- > 1;) {
		delete menu->RemoveItem(i);
	}

	// fill it again

	menu->ItemAt(0)->SetMarked(true);

	BMessage applications;
	if (type == NULL || type->GetSupportingApps(&applications) != B_OK)
		return;

	char preferred[B_MIME_TYPE_LENGTH];
	if (type->GetPreferredApp(preferred) != B_OK)
		preferred[0] = '\0';

	int32 lastFullSupport;
	if (applications.FindInt32("be:sub", &lastFullSupport) != B_OK)
		lastFullSupport = -1;

	const char* signature;
	int32 i = 0;
	while (applications.FindString("applications", i, &signature) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		BMenuItem *item;

		BMimeType applicationType(signature);
		if (applicationType.GetShortDescription(name) == B_OK)
			item = new BMenuItem(name, NULL);
		else
			item = new BMenuItem(signature, NULL);

		// Add type separator
		if (i == 0 || i == lastFullSupport)
			menu->AddSeparatorItem();

		if (!strcasecmp(signature, preferred))
			item->SetMarked(true);

		menu->AddItem(item);
		i++;
	}
}


void
FileTypesWindow::_SetType(BMimeType* type)
{
	bool enabled = type != NULL;

	if (type != NULL) {
		fInternalNameControl->SetText(type->Type());
		
		char description[B_MIME_TYPE_LENGTH];
		if (type->GetShortDescription(description) != B_OK)
			description[0] = '\0';
		fTypeNameControl->SetText(description);
	} else {
		fInternalNameControl->SetText(NULL);
		fTypeNameControl->SetText(NULL);
	}

	_UpdateExtensions(type);
	_UpdatePreferredApps(type);
	fIconView->SetTo(type);

	fIconView->SetEnabled(enabled);

	fTypeNameControl->SetEnabled(enabled);
	fPreferredField->SetEnabled(enabled);

	fRemoveTypeButton->SetEnabled(enabled);

	fSelectButton->SetEnabled(enabled);
	fSameAsButton->SetEnabled(enabled);

	fAddExtensionButton->SetEnabled(enabled);
	fRemoveExtensionButton->SetEnabled(false);

	fAddAttributeButton->SetEnabled(enabled);
	fRemoveAttributeButton->SetEnabled(false);
}


void
FileTypesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTypeSelected:
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				MimeTypeItem* item = (MimeTypeItem*)fTypeListView->ItemAt(index);
				if (item != NULL) {
					BMimeType type(item->Type());
					_SetType(&type);
				} else
					_SetType(NULL);
			}
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
FileTypesWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypesWindowClosed);
	return true;
}


