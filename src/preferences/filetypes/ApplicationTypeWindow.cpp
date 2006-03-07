/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ApplicationTypeWindow.h"
#include "FileTypes.h"
#include "IconView.h"
#include "PreferredAppMenu.h"
#include "StringView.h"
#include "TypeListWindow.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <File.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <stdio.h>


const uint32 kMsgAddType = 'adtp';
const uint32 kMsgRemoveType = 'rmtp';


ApplicationTypeWindow::ApplicationTypeWindow(BPoint position, const BEntry& entry)
	: BWindow(BRect(0.0f, 0.0f, 250.0f, 340.0f).OffsetBySelf(position),
		"Application Type", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Save", NULL, 'S', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Save Into Resource File" B_UTF8_ELLIPSIS,
		NULL));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED),
		'W', B_COMMAND_KEY));
	menuBar->AddItem(menu);

	// Top view and signature

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1.0f;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	rect = topView->Bounds().InsetByCopy(8.0f, 8.0f);
	fSignatureControl = new BTextControl(rect, "signature", "Signature:", NULL,
		NULL, B_FOLLOW_LEFT_RIGHT);
	fSignatureControl->SetDivider(fSignatureControl->StringWidth(
		fSignatureControl->Label()) + 4.0f);
	float width, height;
	fSignatureControl->GetPreferredSize(&width, &height);
	fSignatureControl->ResizeTo(rect.Width(), height);
	topView->AddChild(fSignatureControl);

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = fSignatureControl->TextView();
	const char* disallowedCharacters = "/<>@,;:\"()[]?=";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	// "Application Flags" group

	BFont font(be_bold_font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	width = font.StringWidth("Icon") + 16.0f;
	if (width < B_LARGE_ICON + 16.0f)
		width = B_LARGE_ICON + 16.0f;

	rect.top = fSignatureControl->Frame().bottom + 4.0f;
	rect.bottom = rect.top + 100.0f;
	rect.right -= width + 8.0f;
	BBox* box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	topView->AddChild(box);

	fFlagsCheckBox = new BCheckBox(rect, "flags", "Application Flags",
		NULL); //new BMessage(kMsgFlagChanged));
	fFlagsCheckBox->SetValue(B_CONTROL_ON);
	fFlagsCheckBox->ResizeToPreferred();
	box->SetLabel(fFlagsCheckBox);

	rect.top = fFlagsCheckBox->Bounds().Height() + 4.0f;
	fSingleLaunchButton = new BRadioButton(rect, "single", "Single Launch", NULL);
	fSingleLaunchButton->ResizeToPreferred();
	box->AddChild(fSingleLaunchButton);

	rect.OffsetBy(0.0f, fSingleLaunchButton->Bounds().Height() + 0.0f);
	fMultipleLaunchButton = new BRadioButton(rect, "multiple", "Multiple Launch", NULL);
	fMultipleLaunchButton->ResizeToPreferred();
	box->AddChild(fMultipleLaunchButton);

	rect.OffsetBy(0.0f, fSingleLaunchButton->Bounds().Height() + 0.0f);
	fExclusiveLaunchButton = new BRadioButton(rect, "exclusive", "Exclusive Launch", NULL);
	fExclusiveLaunchButton->ResizeToPreferred();
	box->AddChild(fExclusiveLaunchButton);

	rect.top = fSingleLaunchButton->Frame().top;
	rect.left = fExclusiveLaunchButton->Frame().right + 4.0f;
	fArgsOnlyCheckBox = new BCheckBox(rect, "args only", "Args Only", NULL);
	fArgsOnlyCheckBox->ResizeToPreferred();
	box->AddChild(fArgsOnlyCheckBox);

	rect.top += fArgsOnlyCheckBox->Bounds().Height();
	fBackgroundAppCheckBox = new BCheckBox(rect, "background", "Background App", NULL);
	fBackgroundAppCheckBox->ResizeToPreferred();
	box->AddChild(fBackgroundAppCheckBox);

	box->ResizeTo(box->Bounds().Width(), fExclusiveLaunchButton->Frame().bottom + 8.0f);

	// "Icon" group

	rect = box->Frame();
#ifdef __HAIKU__
	rect.top += box->TopBorderOffset();
#endif
	rect.left = rect.right + 8.0f;
	rect.right += width + 8.0f;
	float iconBoxWidth = rect.Width();
	box = new BBox(rect, NULL, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	box->SetLabel("Icon");
#ifdef __HAIKU__
	box->MoveBy(0.0f, -box->TopBorderOffset());
	box->ResizeBy(0.0f, box->TopBorderOffset());
#endif
	topView->AddChild(box);

	rect = BRect(8.0f, 0.0f, 7.0f + B_LARGE_ICON, B_LARGE_ICON - 1.0f);
#ifdef __HAIKU__
	rect.OffsetBy(0.0f, (box->Bounds().Height() + box->TopBorderOffset()
		- rect.Height()) / 2.0f);
#else
	rect.OffsetBy(0.0f, (box->Bounds().Height() - rect.Height()) / 2.0f);
#endif
	if (rect.top < fontHeight.ascent + fontHeight.descent + 4.0f)
		rect.top = fontHeight.ascent + fontHeight.descent + 4.0f;
	fIconView = new IconView(rect, "icon");
	box->AddChild(fIconView);

	// "Supported Types" group

	rect.top = box->Frame().bottom + 8.0f;
	rect.bottom = rect.top + box->Bounds().Height();
	rect.left = 8.0f;
	rect.right = Bounds().Width() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Supported Types");
	topView->AddChild(box);

	rect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	rect.top += ceilf(fontHeight.ascent);
	fAddTypeButton = new BButton(rect, "add type", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddType), B_FOLLOW_RIGHT);
	fAddTypeButton->ResizeToPreferred();
	fAddTypeButton->MoveBy(rect.right - fAddTypeButton->Bounds().Width()
		- B_LARGE_ICON - 16.0f, 0.0f);
	box->AddChild(fAddTypeButton);

	rect = fAddTypeButton->Frame();
	rect.OffsetBy(0, rect.Height() + 4.0f);
	fRemoveTypeButton = new BButton(rect, "remove type", "Remove",
		new BMessage(kMsgRemoveType), B_FOLLOW_RIGHT);
	box->AddChild(fRemoveTypeButton);

	rect.right = rect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	rect.left = 10.0f;
	rect.top = 8.0f + ceilf(fontHeight.ascent);
	rect.bottom -= 2.0f;
		// take scrollview border into account
	fTypeListView = new BListView(rect, "type listview",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT);
//	fTypeListView->SetSelectionMessage(new BMessage(kMsgExtensionSelected));
//	fTypeListView->SetInvocationMessage(new BMessage(kMsgExtensionInvoked));

	BScrollView* scrollView = new BScrollView("type scrollview", fTypeListView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	box->ResizeTo(box->Bounds().Width(), fRemoveTypeButton->Frame().bottom + 8.0f);

	rect.left = fRemoveTypeButton->Frame().right + 8.0f;
#ifdef __HAIKU__
	rect.top = (box->Bounds().Height() + box->TopBorderOffset() - B_LARGE_ICON) / 2.0f;
#else
	rect.top = (box->Bounds().Height() - B_LARGE_ICON) / 2.0f;
#endif
	rect.right = rect.left + B_LARGE_ICON - 1.0f;
	rect.bottom = rect.top + B_LARGE_ICON - 1.0f;
	fTypeIconView = new IconView(rect, "type icon", B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	box->AddChild(fTypeIconView);

	// "Version Info" group

	rect.top = box->Frame().bottom + 8.0f;
	rect.bottom = rect.top + box->Bounds().Height();
	rect.left = 8.0f;
	rect.right = Bounds().Width() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
		// the resizing mode will later be set to B_FOLLOW_ALL
	box->SetLabel("Version Info");
	topView->AddChild(box);

	BMenuField* menuField;
	BPopUpMenu *popUpMenu;
	BMenuItem* item;
#if 0
	popUpMenu = new BPopUpMenu("version info", true, true);
	item = new BMenuItem("Version Info", NULL);
	item->SetMarked(true);
	popUpMenu->AddItem(item);
	item = new BMenuItem("System Version Info", NULL);
	popUpMenu->AddItem(item);

	menuField = new BMenuField(BRect(0, 0, 100, 15),
		"version kind", NULL, popUpMenu, true);
	menuField->ResizeToPreferred();
	box->SetLabel(menuField);
#endif

	rect.top = 4.0f + ceilf(fontHeight.ascent + fontHeight.descent);
	fMajorVersionControl = new BTextControl(rect, "major", "Version:", NULL,
		NULL);
	fMajorVersionControl->SetDivider(fMajorVersionControl->StringWidth(
		fMajorVersionControl->Label()) + 4.0f);
	fMajorVersionControl->GetPreferredSize(&width, &height);
	width = 12.0f + fMajorVersionControl->StringWidth("99");
	fMajorVersionControl->ResizeTo(fMajorVersionControl->Divider() + width, height);
	box->AddChild(fMajorVersionControl);

	rect.left = fMajorVersionControl->Frame().right;
	fMiddleVersionControl = new BTextControl(rect, "middle", ".", NULL,
		NULL);
	fMiddleVersionControl->SetDivider(fMiddleVersionControl->StringWidth(
		fMiddleVersionControl->Label()) + 4.0f);
	fMiddleVersionControl->ResizeTo(fMiddleVersionControl->Divider() + width, height);
	box->AddChild(fMiddleVersionControl);

	rect.left = fMiddleVersionControl->Frame().right;
	fMinorVersionControl = new BTextControl(rect, "middle", ".", NULL,
		NULL);
	fMinorVersionControl->SetDivider(fMinorVersionControl->StringWidth(
		fMinorVersionControl->Label()) + 4.0f);
	fMinorVersionControl->ResizeTo(fMinorVersionControl->Divider() + width, height);
	box->AddChild(fMinorVersionControl);

	popUpMenu = new BPopUpMenu("variety", true, true);
	popUpMenu->AddItem(new BMenuItem("Development", NULL));
	popUpMenu->AddItem(new BMenuItem("Alpha", NULL));
	popUpMenu->AddItem(new BMenuItem("Beta", NULL));
	popUpMenu->AddItem(new BMenuItem("Gamma", NULL));
	popUpMenu->AddItem(item = new BMenuItem("Golden Master", NULL));
	item->SetMarked(true);
	popUpMenu->AddItem(new BMenuItem("Final", NULL));

	rect.top--;
		// BMenuField oddity
	rect.left = fMinorVersionControl->Frame().right + 6.0f;
	menuField = new BMenuField(rect,
		"variety", NULL, popUpMenu, true);
	menuField->ResizeToPreferred();
	box->AddChild(menuField);

	rect.top++;
	rect.left = menuField->Frame().right;
	rect.right = rect.left + 30.0f;	
	fInternalVersionControl = new BTextControl(rect, "internal", "/", NULL,
		NULL);
	fInternalVersionControl->SetDivider(fInternalVersionControl->StringWidth(
		fInternalVersionControl->Label()) + 4.0f);
	fInternalVersionControl->ResizeTo(fInternalVersionControl->Divider() + width, height);
	box->AddChild(fInternalVersionControl);

	rect = box->Bounds().InsetByCopy(8.0f, 0.0f);
	rect.top = fInternalVersionControl->Frame().bottom + 8.0f;
	fShortDescriptionControl = new BTextControl(rect, "short desc", "Short Description:",
		NULL, NULL, B_FOLLOW_LEFT_RIGHT);
	float labelWidth = fShortDescriptionControl->StringWidth(
		fShortDescriptionControl->Label()) + 4.0f;
	fShortDescriptionControl->SetDivider(labelWidth);
	fShortDescriptionControl->GetPreferredSize(&width, &height);
	fShortDescriptionControl->ResizeTo(rect.Width(), height);
	box->AddChild(fShortDescriptionControl);

	rect.OffsetBy(0.0f, fShortDescriptionControl->Bounds().Height() + 5.0f);
	rect.right = rect.left + labelWidth;
	StringView* label = new StringView(rect, NULL, "Long Description:", NULL);
	label->SetDivider(labelWidth);
	box->AddChild(label);

	rect.left = rect.right + 3.0f;
	rect.top += 1.0f;
	rect.right = box->Bounds().Width() - 8.0f;
	rect.bottom = rect.top + fShortDescriptionControl->Bounds().Height() * 3.0f - 1.0f;
	fLongDescriptionView = new BTextView(rect, "long desc",
		rect.OffsetToCopy(B_ORIGIN), B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);

	scrollView = new BScrollView("desc scrollview", fLongDescriptionView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, false);
	box->ResizeTo(box->Bounds().Width(), scrollView->Frame().bottom + 8.0f);
	box->AddChild(scrollView);

	// Adjust window size and limits

	width = fInternalVersionControl->Frame().right + 16.0f;
	float minWidth = fBackgroundAppCheckBox->Frame().right + iconBoxWidth + 32.0f;
	if (width > minWidth)
		minWidth = width;

	ResizeTo(Bounds().Width() > minWidth ? Bounds().Width() : minWidth,
		box->Frame().bottom + topView->Frame().top + 8.0f);
	SetSizeLimits(minWidth, 32767.0f, Bounds().Height(), 32767.0f);
	box->SetResizingMode(B_FOLLOW_ALL);

	fSignatureControl->MakeFocus(true);

	BMimeType::StartWatching(this);
	_SetTo(entry);
}


ApplicationTypeWindow::~ApplicationTypeWindow()
{
	BMimeType::StopWatching(this);
}


BString
ApplicationTypeWindow::_Title(const BEntry& entry)
{
	char name[B_FILE_NAME_LENGTH];
	if (entry.GetName(name) != B_OK)
		strcpy(name, "\"-\"");

	BString title(name);
	title.Append(" Application Type");
	return title;
}


void
ApplicationTypeWindow::_SetTo(const BEntry& entry)
{
	SetTitle(_Title(entry).String());
	fEntry = entry;
}


void
ApplicationTypeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			// TODO: add to supported types
			break;
		}

		case B_META_MIME_CHANGED:
			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			// TODO: update supported types names
//			if (which == B_MIME_TYPE_DELETED)
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
ApplicationTypeWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypeWindowClosed);
	return true;
}

