/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeListView.h"
#include "AttributeWindow.h"
#include "ExtensionWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "MimeTypeListView.h"
#include "NewFileTypeWindow.h"
#include "PreferredAppMenu.h"
#include "StringView.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <OverrideAlert.h>
#include <be_apps/Tracker/RecentItems.h>

#include <stdio.h>


const uint32 kMsgTypeSelected = 'typs';
const uint32 kMsgAddType = 'atyp';
const uint32 kMsgRemoveType = 'rtyp';

const uint32 kMsgExtensionSelected = 'exts';
const uint32 kMsgExtensionInvoked = 'exti';
const uint32 kMsgAddExtension = 'aext';
const uint32 kMsgRemoveExtension = 'rext';

const uint32 kMsgAttributeSelected = 'atrs';
const uint32 kMsgAttributeInvoked = 'atri';
const uint32 kMsgAddAttribute = 'aatr';
const uint32 kMsgRemoveAttribute = 'ratr';

const uint32 kMsgPreferredAppChosen = 'papc';
const uint32 kMsgSelectPreferredApp = 'slpa';
const uint32 kMsgSamePreferredAppAs = 'spaa';

const uint32 kMsgPreferredAppOpened = 'paOp';
const uint32 kMsgSamePreferredAppAsOpened = 'spaO';

const uint32 kMsgTypeEntered = 'type';
const uint32 kMsgDescriptionEntered = 'dsce';

const uint32 kMsgToggleIcons = 'tgic';

class TypeIconView : public BControl {
	public:
		TypeIconView(BRect frame, const char* name, BMessage* message);
		virtual ~TypeIconView();

		void SetTo(BMimeType* type);

		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float* _width, float* _height);

#if 0
		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit,
			BMessage* dragMessage);
#endif

	private:
		BBitmap*	fIcon;
		icon_source	fIconSource;
};


//	#pragma mark -


TypeIconView::TypeIconView(BRect frame, const char* name, BMessage* message)
	: BControl(frame, name, NULL, message,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fIcon(NULL),
	fIconSource(kNoIcon)
{
}


TypeIconView::~TypeIconView()
{
	delete fIcon;
}


void
TypeIconView::SetTo(BMimeType* type)
{
	int32 sourceWas = fIconSource;
	fIconSource = kNoIcon;

	if (type != NULL) {
		if (fIcon == NULL) {
			fIcon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1),
				B_CMAP8);
		}

		icon_for_type(*type, *fIcon, B_LARGE_ICON, &fIconSource);
	}

	if (fIconSource == kNoIcon) {
		delete fIcon;
		fIcon = NULL;
	}

	if (sourceWas != fIconSource || sourceWas != kNoIcon)
		Invalidate();
}


void
TypeIconView::Draw(BRect updateRect)
{
	SetHighColor(ViewColor());
	FillRect(updateRect);

	if (!IsEnabled())
		return;

	if (fIcon != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon,
			BPoint((Bounds().Width() - fIcon->Bounds().Width()) / 2.0f, 0.0f));
	}

	const char* text = NULL;

	switch (fIconSource) {
		case kNoIcon:
			text = "no icon";
			break;
		case kApplicationIcon:
			text = "(from application)";
			break;
		case kSupertypeIcon:
			text = "(from super type)";
			break;

		default:
			return;
	}

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));
	SetLowColor(ViewColor());

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float y = fontHeight.ascent;
	if (fIconSource == kNoIcon) {
		// center text in the middle of the icon
		y += (B_LARGE_ICON - fontHeight.ascent - fontHeight.descent) / 2.0f;
	} else
		y += B_LARGE_ICON + 3.0f;

	DrawString(text, BPoint((Bounds().Width() - StringWidth(text)) / 2.0f,
		y));
}


void
TypeIconView::GetPreferredSize(float* _width, float* _height)
{
	if (_width) {
		float a = StringWidth("(from application)");
		float b = StringWidth("(from super type)");
		float width = max_c(a, b);
		if (width < B_LARGE_ICON)
			width = B_LARGE_ICON;

		*_width = ceilf(width);
	}

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = B_LARGE_ICON + 3.0f + ceilf(fontHeight.ascent + fontHeight.descent);
	}
}


//	#pragma mark -


FileTypesWindow::FileTypesWindow(BRect frame)
	: BWindow(frame, "FileTypes", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fNewTypeWindow(NULL)
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
		new BMessage(kMsgOpenApplicationTypesWindow)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About FileTypes" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	menuBar->AddItem(menu);

	menu = new BMenu("Settings");
	item = new BMenuItem("Show Icons in List", new BMessage(kMsgToggleIcons));
	item->SetTarget(this);
	menu->AddItem(item);
	menuBar->AddItem(menu);

	// MIME Types list

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1.0f;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BButton* button = new BButton(rect, "add", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddType), B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(8.0f, topView->Bounds().bottom - 8.0f - button->Bounds().Height());
	topView->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 8.0f, 0.0f);
	fRemoveTypeButton = new BButton(rect, "remove", "Remove",
		new BMessage(kMsgRemoveType), B_FOLLOW_BOTTOM);
	fRemoveTypeButton->ResizeToPreferred();
	topView->AddChild(fRemoveTypeButton);

	rect.bottom = rect.top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 2.0f;
	if (rect.right < 180)
		rect.right = 180;

	fTypeListView = new MimeTypeListView(rect, "typeview", NULL, false, false,
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

	BRect innerRect;
	fIconView = new TypeIconView(innerRect, "icon", NULL);
	fIconView->ResizeToPreferred();

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.right = rect.left + max_c(fIconView->Bounds().Width(), labelWidth) + 16.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ max_c(fIconView->Bounds().Height(),
			button->Bounds().Height() * 2.0f + 4.0f) + 12.0f;
	rect.top -= 2.0f;
	BBox* box = new BBox(rect);
	box->SetLabel("Icon");
	topView->AddChild(box);

	innerRect.left = 8.0f;
	innerRect.top = fontHeight.ascent / 2.0f
		+ (rect.Height() - fontHeight.ascent / 2.0f - fIconView->Bounds().Height()) / 2.0f
		+ 3.0f + fontHeight.ascent;
	if (innerRect.top + fIconView->Bounds().Height() > box->Bounds().Height() - 6.0f)
		innerRect.top = box->Bounds().Height() - 6.0f - fIconView->Bounds().Height();
	fIconView->MoveTo(innerRect.LeftTop());
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
		new BMessage(kMsgAddExtension), B_FOLLOW_RIGHT);
	box->AddChild(fAddExtensionButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveExtensionButton = new BButton(innerRect, "remove ext", "Remove",
		new BMessage(kMsgRemoveExtension), B_FOLLOW_RIGHT);
	box->AddChild(fRemoveExtensionButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom -= 2.0f;
		// take scrollview border into account
	fExtensionListView = new BListView(innerRect, "listview ext",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT);
	fExtensionListView->SetSelectionMessage(new BMessage(kMsgExtensionSelected));
	fExtensionListView->SetInvocationMessage(new BMessage(kMsgExtensionInvoked));

	scrollView = new BScrollView("scrollview ext", fExtensionListView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	// "Description" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent) + 24.0f;
	rect.right = rightRect.right;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Description");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fInternalNameView = new StringView(innerRect, "internal", "Internal Name:", "",
		B_FOLLOW_LEFT_RIGHT);
	labelWidth = fInternalNameView->StringWidth(fInternalNameView->Label()) + 2.0f;
	fInternalNameView->SetDivider(labelWidth);
	fInternalNameView->SetEnabled(false);
	fInternalNameView->ResizeToPreferred();
	box->AddChild(fInternalNameView);

	innerRect.OffsetBy(0, fInternalNameView->Bounds().Height() + 5.0f);
	fTypeNameControl = new BTextControl(innerRect, "type", "Type Name:", "",
		new BMessage(kMsgTypeEntered), B_FOLLOW_LEFT_RIGHT);
	fTypeNameControl->SetDivider(labelWidth);
	fTypeNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	box->ResizeBy(0, fInternalNameView->Bounds().Height()
		+ fTypeNameControl->Bounds().Height() * 2.0f);
	box->AddChild(fTypeNameControl);

	innerRect.OffsetBy(0, fTypeNameControl->Bounds().Height() + 5.0f);
	fDescriptionControl = new BTextControl(innerRect, "description", "Description:", "",
		new BMessage(kMsgDescriptionEntered), B_FOLLOW_LEFT_RIGHT);
	fDescriptionControl->SetDivider(labelWidth);
	fDescriptionControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	box->AddChild(fDescriptionControl);

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
	fSameAsButton = new BButton(innerRect, "same as", "Same As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSamePreferredAppAs), B_FOLLOW_RIGHT);
	box->AddChild(fSameAsButton);

	innerRect.OffsetBy(-innerRect.Width() - 6.0f, 0.0f);
	fSelectButton = new BButton(innerRect, "select", "Select" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSelectPreferredApp), B_FOLLOW_RIGHT);
	box->AddChild(fSelectButton);

	menu = new BPopUpMenu("preferred");
	menu->AddItem(item = new BMenuItem("None", new BMessage(kMsgPreferredAppChosen)));
	item->SetMarked(true);

	innerRect.right = innerRect.left - 6.0f;
	innerRect.left = 8.0f;
	BView* constrainingView = new BView(innerRect, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	constrainingView->SetViewColor(topView->ViewColor());

	fPreferredField = new BMenuField(innerRect.OffsetToCopy(B_ORIGIN), "preferred",
		NULL, menu);
	float width, height;
	fPreferredField->GetPreferredSize(&width, &height);
	fPreferredField->ResizeTo(innerRect.Width(), height);
	fPreferredField->MoveBy(0.0f, (innerRect.Height() - height) / 2.0f);
	constrainingView->AddChild(fPreferredField);
		// we embed the menu field in another view to make it behave like
		// we want so that it can't obscure other elements with larger
		// labels

	box->AddChild(constrainingView);

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
	fAddAttributeButton = new BButton(innerRect, "add attr", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddAttribute), B_FOLLOW_RIGHT);
	box->AddChild(fAddAttributeButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveAttributeButton = new BButton(innerRect, "remove attr", "Remove",
		new BMessage(kMsgRemoveAttribute), B_FOLLOW_RIGHT);
	box->AddChild(fRemoveAttributeButton);
/*
	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	button = new BButton(innerRect, "push attr", "Push Up",
		new BMessage(kMsgRemoveAttribute), B_FOLLOW_RIGHT);
	box->AddChild(button);
*/
	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom = box->Bounds().bottom - 10.0f;
		// take scrollview border into account
	fAttributeListView = new AttributeListView(innerRect, "listview attr",
		B_FOLLOW_ALL);
	fAttributeListView->SetSelectionMessage(new BMessage(kMsgAttributeSelected));
	fAttributeListView->SetInvocationMessage(new BMessage(kMsgAttributeInvoked));

	scrollView = new BScrollView("scrollview attr", fAttributeListView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	SetSizeLimits(rightRect.left + 72.0f + font.StringWidth("File Extensions"), 32767.0f,
		rect.top + 2.0f * button->Bounds().Height() + fontHeight.ascent + 32.0f
		+ menuBar->Bounds().Height(), 32767.0f);

	_SetType(NULL);
	BMimeType::StartWatching(this);
}


FileTypesWindow::~FileTypesWindow()
{
	BMimeType::StopWatching(this);
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
FileTypesWindow::_AdoptPreferredApplication(BMessage* message, bool sameAs)
{
	if (fCurrentType.Type() == NULL)
		return;

	BString preferred;
	if (retrieve_preferred_app(message, sameAs, fCurrentType.Type(), preferred) != B_OK)
		return;

	status_t status = fCurrentType.SetPreferredApp(preferred.String());
	if (status != B_OK)
		error_alert("Could not set preferred application", status);
}


void
FileTypesWindow::_UpdatePreferredApps(BMimeType* type)
{
	update_preferred_app_menu(fPreferredField->Menu(), type, kMsgPreferredAppChosen);
}


void
FileTypesWindow::_UpdateIcon(BMimeType* type)
{
	fIconView->SetTo(type);
}


void
FileTypesWindow::_SetType(BMimeType* type, int32 forceUpdate)
{
	bool enabled = type != NULL;

	// update controls

	if (type != NULL) {
		if (fCurrentType == *type) {
			if (!forceUpdate)
				return;
		} else
			forceUpdate = B_EVERYTHING_CHANGED;

		if (&fCurrentType != type)
			fCurrentType.SetTo(type->Type());

		fInternalNameView->SetText(type->Type());

		char description[B_MIME_TYPE_LENGTH];

		if ((forceUpdate & B_SHORT_DESCRIPTION_CHANGED) != 0) {
			if (type->GetShortDescription(description) != B_OK)
				description[0] = '\0';
			fTypeNameControl->SetText(description);
		}

		if ((forceUpdate & B_LONG_DESCRIPTION_CHANGED) != 0) {
			if (type->GetLongDescription(description) != B_OK)
				description[0] = '\0';
			fDescriptionControl->SetText(description);
		}
	} else {
		fCurrentType.Unset();
		fInternalNameView->SetText(NULL);
		fTypeNameControl->SetText(NULL);
		fDescriptionControl->SetText(NULL);
	}

	if ((forceUpdate & B_FILE_EXTENSIONS_CHANGED) != 0)
		_UpdateExtensions(type);

	if ((forceUpdate & B_PREFERRED_APP_CHANGED) != 0)
		_UpdatePreferredApps(type);

	if ((forceUpdate & (B_ICON_CHANGED | B_PREFERRED_APP_CHANGED)) != 0)
		_UpdateIcon(type);

	if ((forceUpdate & B_ATTR_INFO_CHANGED) != 0)
		fAttributeListView->SetTo(type);

	// enable/disable controls

	fIconView->SetEnabled(enabled);

	fInternalNameView->SetEnabled(enabled);
	fTypeNameControl->SetEnabled(enabled);
	fDescriptionControl->SetEnabled(enabled);
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
FileTypesWindow::PlaceSubWindow(BWindow* window)
{
	window->MoveTo(Frame().left + (Frame().Width() - window->Frame().Width()) / 2.0f,
		Frame().top + (Frame().Height() - window->Frame().Height()) / 2.0f);
}


void
FileTypesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
			type_code type;
			if (message->GetInfo("refs", &type) == B_OK
				&& type == B_REF_TYPE) {
				be_app->PostMessage(message);
			}
			break;

		case kMsgToggleIcons:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void **)&item) != B_OK)
				break;

			item->SetMarked(!fTypeListView->IsShowingIcons());
			fTypeListView->ShowIcons(item->IsMarked());
			break;
		}

		case kMsgTypeSelected:
		{
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
		}

		case kMsgAddType:
		{
			if (fNewTypeWindow == NULL) {
				fNewTypeWindow = new NewFileTypeWindow(this, fCurrentType.Type());
				fNewTypeWindow->Show();
			} else
				fNewTypeWindow->Activate();
			break;
		}
		case kMsgNewTypeWindowClosed:
			fNewTypeWindow = NULL;
			break;

		case kMsgRemoveType:
		{
			if (fCurrentType.Type() == NULL)
				break;

			BAlert* alert;
			if (fCurrentType.IsSupertypeOnly()) {
				alert = new BPrivate::OverrideAlert("FileTypes Request",
					"Removing a super type cannot be reverted.\n"
					"All file types that belong to this super type "
					"will be lost!\n\n"
					"Are you sure you want to do this? To remove the whole "
					"group, hold down the Shift key and press \"Remove\".",
					"Remove", B_SHIFT_KEY, "Cancel", 0, NULL, 0,
					B_WIDTH_AS_USUAL, B_STOP_ALERT);
			} else {
				alert = new BAlert("FileTypes Request",
					"Removing a file type cannot be reverted.\n"
					"Are you sure you want to remove it?",
					"Remove", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			}
			if (alert->Go())
				break;

			status_t status = fCurrentType.Delete();
			if (status != B_OK)
				fprintf(stderr, "Could not remove file type: %s\n", strerror(status));
			break;
		}

		case kMsgSelectNewType:
		{
			const char* type;
			if (message->FindString("type", &type) == B_OK)
				fTypeListView->SelectNewType(type);
			break;
		}

		// File Extensions group

		case kMsgExtensionSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BStringItem* item = (BStringItem*)fExtensionListView->ItemAt(index);
				fRemoveExtensionButton->SetEnabled(item != NULL);
			}
			break;
		}

		case kMsgExtensionInvoked:
		{
			if (fCurrentType.Type() == NULL)
				break;

			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BStringItem* item = (BStringItem*)fExtensionListView->ItemAt(index);
				if (item == NULL)
					break;

				BWindow* window = new ExtensionWindow(this, fCurrentType, item->Text());
				window->Show();
			}
			break;
		}

		case kMsgAddExtension:
		{
			if (fCurrentType.Type() == NULL)
				break;

			BWindow* window = new ExtensionWindow(this, fCurrentType, NULL);
			window->Show();
			break;
		}

		case kMsgRemoveExtension:
		{
			int32 index = fExtensionListView->CurrentSelection();
			if (index < 0 || fCurrentType.Type() == NULL)
				break;

			BMessage extensions;
			if (fCurrentType.GetFileExtensions(&extensions) == B_OK) {
				extensions.RemoveData("extensions", index);
				fCurrentType.SetFileExtensions(&extensions);
			}
			break;
		}

		// Description group

		case kMsgTypeEntered:
		{
			fCurrentType.SetShortDescription(fTypeNameControl->Text());
			break;
		}

		case kMsgDescriptionEntered:
		{
			fCurrentType.SetLongDescription(fDescriptionControl->Text());
			break;
		}

		// Preferred Application group

		case kMsgPreferredAppChosen:
		{
			const char* signature;
			if (message->FindString("signature", &signature) != B_OK)
				signature = NULL;

			fCurrentType.SetPreferredApp(signature);
			break;
		}

		case kMsgSelectPreferredApp:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", "Select Preferred Application");
			panel.AddInt32("message", kMsgPreferredAppOpened);
			panel.AddMessenger("target", this);

			be_app_messenger.SendMessage(&panel);
			break;
		}
		case kMsgPreferredAppOpened:
			_AdoptPreferredApplication(message, false);
			break;

		case kMsgSamePreferredAppAs:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", "Select Same Preferred Application As");
			panel.AddInt32("message", kMsgSamePreferredAppAsOpened);
			panel.AddMessenger("target", this);

			be_app_messenger.SendMessage(&panel);
			break;
		}
		case kMsgSamePreferredAppAsOpened:
			_AdoptPreferredApplication(message, true);
			break;

		// Extra Attributes group

		case kMsgAttributeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				AttributeItem* item = (AttributeItem*)fAttributeListView->ItemAt(index);
				fRemoveAttributeButton->SetEnabled(item != NULL);
			}
			break;
		}

		case kMsgAttributeInvoked:
		{
			if (fCurrentType.Type() == NULL)
				break;

			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				AttributeItem* item = (AttributeItem*)fAttributeListView->ItemAt(index);
				if (item == NULL)
					break;

				BWindow* window = new AttributeWindow(this, fCurrentType,
					item);
				window->Show();
			}
			break;
		}

		case kMsgAddAttribute:
		{
			if (fCurrentType.Type() == NULL)
				break;

			BWindow* window = new AttributeWindow(this, fCurrentType, NULL);
			window->Show();
			break;
		}

		case kMsgRemoveAttribute:
		{
			int32 index = fAttributeListView->CurrentSelection();
			if (index < 0 || fCurrentType.Type() == NULL)
				break;

			BMessage attributes;
			if (fCurrentType.GetAttrInfo(&attributes) == B_OK) {
				const char* kAttributeNames[] = {
					"attr:public_name", "attr:name", "attr:type",
					"attr:editable", "attr:viewable", "attr:extra",
					"attr:alignment", "attr:width", "attr:display_as"
				};

				for (uint32 i = 0; i <
						sizeof(kAttributeNames) / sizeof(kAttributeNames[0]); i++) {
					attributes.RemoveData(kAttributeNames[i], index);
				}

				fCurrentType.SetAttrInfo(&attributes);
			}
			break;
		}

		case B_META_MIME_CHANGED:
		{
			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			if (fCurrentType.Type() == NULL)
				break;

			if (!strcasecmp(fCurrentType.Type(), type)) {
				if (which != B_MIME_TYPE_DELETED)
					_SetType(&fCurrentType, which);
				else
					_SetType(NULL);
			} else {
				// this change could still affect our current type

				if (which == B_MIME_TYPE_DELETED
					|| which == B_PREFERRED_APP_CHANGED
#ifdef __HAIKU__
					|| which == B_SUPPORTED_TYPES_CHANGED
#endif
					|| which == B_ICON_FOR_TYPE_CHANGED) {
					_UpdatePreferredApps(&fCurrentType);
					_UpdateIcon(&fCurrentType);
				}
			}
			break;
		}

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


