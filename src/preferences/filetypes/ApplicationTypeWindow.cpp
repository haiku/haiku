/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ApplicationTypeWindow.h"
#include "DropTargetListView.h"
#include "FileTypes.h"
#include "IconView.h"
#include "PreferredAppMenu.h"
#include "StringView.h"
#include "TypeListWindow.h"

#include <AppFileInfo.h>
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
#include <Roster.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const uint32 kMsgSave = 'save';
const uint32 kMsgFlagsChanged = 'flgc';

const uint32 kMsgTypeSelected = 'tpsl';
const uint32 kMsgAddType = 'adtp';
const uint32 kMsgTypeAdded = 'tpad';
const uint32 kMsgRemoveType = 'rmtp';


class SupportedTypeItem : public BStringItem {
	public:
		SupportedTypeItem(const char* type);
		~SupportedTypeItem();

		const char* Type() const { return fType.String(); }
		::Icon& Icon() { return fIcon; }
		void SetIcon(::Icon* icon);
		void SetIcon(entry_ref& ref, const char* type);

		static int Compare(const void* _a, const void* _b);

	private:
		BString	fType;
		::Icon	fIcon;
};

class SupportedTypeListView : public DropTargetListView {
	public:
		SupportedTypeListView(BRect frame, const char* name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
		virtual ~SupportedTypeListView();

		virtual void MessageReceived(BMessage* message);
		virtual bool AcceptsDrag(const BMessage* message);
};


SupportedTypeItem::SupportedTypeItem(const char* type)
	: BStringItem(type),
	fType(type)
{
	BMimeType mimeType(type);

	char description[B_MIME_TYPE_LENGTH];
	if (mimeType.GetShortDescription(description) == B_OK && description[0])
		SetText(description);
}


SupportedTypeItem::~SupportedTypeItem()
{
}


void
SupportedTypeItem::SetIcon(::Icon* icon)
{
	if (icon != NULL)
		fIcon = *icon;
	else
		fIcon.Unset();
}


void
SupportedTypeItem::SetIcon(entry_ref& ref, const char* type)
{
	fIcon.SetTo(ref, type);
}


/*static*/
int
SupportedTypeItem::Compare(const void* _a, const void* _b)
{
	const SupportedTypeItem* a = *(const SupportedTypeItem**)_a;
	const SupportedTypeItem* b = *(const SupportedTypeItem**)_b;

	int compare = strcasecmp(a->Text(), b->Text());
	if (compare != 0)
		return compare;

	return strcasecmp(a->Type(), b->Type());
}


//	#pragma mark -


SupportedTypeListView::SupportedTypeListView(BRect frame, const char* name,
		list_view_type type, uint32 resizeMask, uint32 flags)
	: DropTargetListView(frame, name, type, resizeMask, flags)
{
}


SupportedTypeListView::~SupportedTypeListView()
{
}


void
SupportedTypeListView::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && AcceptsDrag(message)) {
		// Add unique types
		entry_ref ref;
		for (int32 index = 0; message->FindRef("refs", index++, &ref) == B_OK; ) {
			BNode node(&ref);
			BNodeInfo info(&node);
			if (node.InitCheck() != B_OK || info.InitCheck() != B_OK)
				continue;

			// TODO: we could identify the file in case it doesn't have a type...
			char type[B_MIME_TYPE_LENGTH];
			if (info.GetType(type) != B_OK)
				continue;

			// check if that type is already in our list
			bool found = false;
			for (int32 i = CountItems(); i-- > 0;) {
				SupportedTypeItem* item = (SupportedTypeItem*)ItemAt(i);
				if (!strcmp(item->Text(), type)) {
					found = true;
					break;
				}
			}

			if (!found) {
				// add type
				AddItem(new SupportedTypeItem(type));
			}
		}

		SortItems(&SupportedTypeItem::Compare);
	} else
		DropTargetListView::MessageReceived(message);
}


bool
SupportedTypeListView::AcceptsDrag(const BMessage* message)
{
	type_code type;
	return message->GetInfo("refs", &type) == B_OK && type == B_REF_TYPE;
}


//	#pragma mark -


ApplicationTypeWindow::ApplicationTypeWindow(BPoint position, const BEntry& entry)
	: BWindow(BRect(0.0f, 0.0f, 250.0f, 340.0f).OffsetBySelf(position),
		"Application Type", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Save", new BMessage(kMsgSave), 'S', B_COMMAND_KEY));
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem("Save Into Resource File" B_UTF8_ELLIPSIS,
		NULL));
	item->SetEnabled(false);

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
	textView->SetMaxBytes(B_MIME_TYPE_LENGTH);
	const char* disallowedCharacters = "<>@,;:\"()[]?=";
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
		new BMessage(kMsgFlagsChanged));
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
	BBox* typeBox = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	typeBox->SetLabel("Supported Types");
	topView->AddChild(typeBox);

	rect = typeBox->Bounds().InsetByCopy(8.0f, 6.0f);
	rect.top += ceilf(fontHeight.ascent);
	fAddTypeButton = new BButton(rect, "add type", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddType), B_FOLLOW_RIGHT);
	fAddTypeButton->ResizeToPreferred();
	fAddTypeButton->MoveBy(rect.right - fAddTypeButton->Bounds().Width()
		- B_LARGE_ICON - 16.0f, 0.0f);
	typeBox->AddChild(fAddTypeButton);

	rect = fAddTypeButton->Frame();
	rect.OffsetBy(0, rect.Height() + 4.0f);
	fRemoveTypeButton = new BButton(rect, "remove type", "Remove",
		new BMessage(kMsgRemoveType), B_FOLLOW_RIGHT);
	typeBox->AddChild(fRemoveTypeButton);

	rect.right = rect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	rect.left = 10.0f;
	rect.top = 8.0f + ceilf(fontHeight.ascent);
	rect.bottom -= 2.0f;
		// take scrollview border into account
	fTypeListView = new SupportedTypeListView(rect, "type listview",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("type scrollview", fTypeListView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	typeBox->ResizeTo(typeBox->Bounds().Width(), fRemoveTypeButton->Frame().bottom + 8.0f);
	typeBox->AddChild(scrollView);

	rect.left = fRemoveTypeButton->Frame().right + 8.0f;
#ifdef __HAIKU__
	rect.top = (box->Bounds().Height() + box->TopBorderOffset() - B_LARGE_ICON) / 2.0f;
#else
	rect.top = (box->Bounds().Height() - B_LARGE_ICON) / 2.0f;
#endif
	rect.right = rect.left + B_LARGE_ICON - 1.0f;
	rect.bottom = rect.top + B_LARGE_ICON - 1.0f;
	fTypeIconView = new IconView(rect, "type icon", B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	typeBox->AddChild(fTypeIconView);

	// "Version Info" group

	rect.top = typeBox->Frame().bottom + 8.0f;
	rect.bottom = rect.top + typeBox->Bounds().Height();
	rect.left = 8.0f;
	rect.right = Bounds().Width() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
		// the resizing mode will later also be set to B_FOLLOW_BOTTOM
	box->SetLabel("Version Info");
	topView->AddChild(box);

	BMenuField* menuField;
#if 0
	BPopUpMenu *popUpMenu = new BPopUpMenu("version info", true, true);
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
	_MakeNumberTextControl(fMajorVersionControl);
	box->AddChild(fMajorVersionControl);

	rect.left = fMajorVersionControl->Frame().right + 1.0f;
	fMiddleVersionControl = new BTextControl(rect, "middle", ".", NULL,
		NULL);
	fMiddleVersionControl->SetDivider(fMiddleVersionControl->StringWidth(
		fMiddleVersionControl->Label()) + 4.0f);
	fMiddleVersionControl->ResizeTo(fMiddleVersionControl->Divider() + width, height);
	_MakeNumberTextControl(fMiddleVersionControl);
	box->AddChild(fMiddleVersionControl);

	rect.left = fMiddleVersionControl->Frame().right + 1.0f;
	fMinorVersionControl = new BTextControl(rect, "middle", ".", NULL,
		NULL);
	fMinorVersionControl->SetDivider(fMinorVersionControl->StringWidth(
		fMinorVersionControl->Label()) + 4.0f);
	fMinorVersionControl->ResizeTo(fMinorVersionControl->Divider() + width, height);
	_MakeNumberTextControl(fMinorVersionControl);
	box->AddChild(fMinorVersionControl);

	fVarietyMenu = new BPopUpMenu("variety", true, true);
	fVarietyMenu->AddItem(new BMenuItem("Development", NULL));
	fVarietyMenu->AddItem(new BMenuItem("Alpha", NULL));
	fVarietyMenu->AddItem(new BMenuItem("Beta", NULL));
	fVarietyMenu->AddItem(new BMenuItem("Gamma", NULL));
	fVarietyMenu->AddItem(item = new BMenuItem("Golden Master", NULL));
	item->SetMarked(true);
	fVarietyMenu->AddItem(new BMenuItem("Final", NULL));

	rect.top--;
		// BMenuField oddity
	rect.left = fMinorVersionControl->Frame().right + 6.0f;
	menuField = new BMenuField(rect,
		"variety", NULL, fVarietyMenu, true);
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
		NULL, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	float labelWidth = fShortDescriptionControl->StringWidth(
		fShortDescriptionControl->Label()) + 4.0f;
	fShortDescriptionControl->SetDivider(labelWidth);
	fShortDescriptionControl->GetPreferredSize(&width, &height);
	fShortDescriptionControl->ResizeTo(rect.Width(), height);

	// TODO: workaround for a GCC 4.1.0 bug? Or is that really what the standard says?
	version_info versionInfo;
	fShortDescriptionControl->TextView()->SetMaxBytes(sizeof(versionInfo.short_info));
	box->AddChild(fShortDescriptionControl);

	rect.OffsetBy(0.0f, fShortDescriptionControl->Bounds().Height() + 5.0f);
	rect.right = rect.left + labelWidth;
	StringView* label = new StringView(rect, NULL, "Long Description:", NULL);
	label->SetDivider(labelWidth);
	box->AddChild(label);

	rect.left = rect.right + 3.0f;
	rect.top += 1.0f;
	rect.right = box->Bounds().Width() - 10.0f - B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + fShortDescriptionControl->Bounds().Height() * 3.0f - 1.0f;
	fLongDescriptionView = new BTextView(rect, "long desc",
		rect.OffsetToCopy(B_ORIGIN), B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	fLongDescriptionView->SetMaxBytes(sizeof(versionInfo.long_info));

	scrollView = new BScrollView("desc scrollview", fLongDescriptionView,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
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
	typeBox->SetResizingMode(B_FOLLOW_ALL);
	box->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);

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

	// Retrieve Info

	BFile file(&entry, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BAppFileInfo info(&file);
	if (info.InitCheck() != B_OK)
		return;

	char signature[B_MIME_TYPE_LENGTH];
	if (info.GetSignature(signature) != B_OK)
		signature[0] = '\0';

	bool gotFlags = false;
	uint32 flags;
	if (info.GetAppFlags(&flags) == B_OK)
		gotFlags = true;
	else
		flags = B_MULTIPLE_LAUNCH;

	BMessage supportedTypes;
	info.GetSupportedTypes(&supportedTypes);

	version_info versionInfo;
	if (info.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) != B_OK)
		memset(&versionInfo, 0, sizeof(version_info));

	// Set Controls

	fSignatureControl->SetText(signature);

	// flags

	switch (flags & (B_SINGLE_LAUNCH | B_MULTIPLE_LAUNCH | B_EXCLUSIVE_LAUNCH)) {
		case B_SINGLE_LAUNCH:
			fSingleLaunchButton->SetValue(B_CONTROL_ON);
			break;

		case B_EXCLUSIVE_LAUNCH:
			fExclusiveLaunchButton->SetValue(B_CONTROL_ON);
			break;

		case B_MULTIPLE_LAUNCH:
		default:
			fMultipleLaunchButton->SetValue(B_CONTROL_ON);
			break;
	}

	fArgsOnlyCheckBox->SetValue((flags & B_ARGV_ONLY) != 0);
	fBackgroundAppCheckBox->SetValue((flags & B_BACKGROUND_APP) != 0);
	fFlagsCheckBox->SetValue(gotFlags);

	_UpdateAppFlagsEnabled();

	// icon

	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK)
		fIcon.SetTo(ref);
	else
		fIcon.Unset();

	fIconView->SetTo(&fIcon);

	// supported types

	for (int32 i = fTypeListView->CountItems(); i-- > 0;) {
		BListItem* item = fTypeListView->RemoveItem(i);
		delete item;
	}

	const char* type;
	for (int32 i = 0; supportedTypes.FindString("types", i, &type) == B_OK; i++) {
		SupportedTypeItem* item = new SupportedTypeItem(type);

		entry_ref ref;
		if (fEntry.GetRef(&ref) == B_OK)
			item->SetIcon(ref, type);

		fTypeListView->AddItem(item);
	}
	fTypeListView->SortItems(&SupportedTypeItem::Compare);
	fTypeIconView->SetTo(NULL);
	fTypeIconView->SetEnabled(false);
	fRemoveTypeButton->SetEnabled(false);

	// version info
	
	char text[256];
	snprintf(text, sizeof(text), "%ld", versionInfo.major);
	fMajorVersionControl->SetText(text);
	snprintf(text, sizeof(text), "%ld", versionInfo.middle);
	fMiddleVersionControl->SetText(text);
	snprintf(text, sizeof(text), "%ld", versionInfo.minor);
	fMinorVersionControl->SetText(text);

	if (versionInfo.variety >= (uint32)fVarietyMenu->CountItems())
		versionInfo.variety = 0;
	BMenuItem* item = fVarietyMenu->ItemAt(versionInfo.variety);
	if (item != NULL)
		item->SetMarked(true);

	snprintf(text, sizeof(text), "%ld", versionInfo.internal);
	fInternalVersionControl->SetText(text);

	fShortDescriptionControl->SetText(versionInfo.short_info);
	fLongDescriptionView->SetText(versionInfo.long_info);
}


void
ApplicationTypeWindow::_UpdateAppFlagsEnabled()
{
	bool enabled = fFlagsCheckBox->Value() != B_CONTROL_OFF;

	fSingleLaunchButton->SetEnabled(enabled);
	fMultipleLaunchButton->SetEnabled(enabled);
	fExclusiveLaunchButton->SetEnabled(enabled);
	fArgsOnlyCheckBox->SetEnabled(enabled);
	fBackgroundAppCheckBox->SetEnabled(enabled);
}


void
ApplicationTypeWindow::_MakeNumberTextControl(BTextControl* control)
{
	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = control->TextView();
	textView->SetMaxBytes(10);

	for (int32 i = 0; i < 256; i++) {
		if (!isdigit(i))
			textView->DisallowChar(i);
	}
}


void
ApplicationTypeWindow::_Save()
{
	BFile file;
	status_t status = file.SetTo(&fEntry, B_READ_WRITE);
	if (status != B_OK)
		return;

	BAppFileInfo info(&file);
	status = info.InitCheck();
	if (status != B_OK)
		return;

	// Retrieve Info

	uint32 flags = 0;
	if (fFlagsCheckBox->Value() != B_CONTROL_OFF) {
		if (fSingleLaunchButton->Value() != B_CONTROL_OFF)
			flags |= B_SINGLE_LAUNCH;
		else if (fMultipleLaunchButton->Value() != B_CONTROL_OFF)
			flags |= B_MULTIPLE_LAUNCH;
		else if (fExclusiveLaunchButton->Value() != B_CONTROL_OFF)
			flags |= B_EXCLUSIVE_LAUNCH;

		if (fArgsOnlyCheckBox->Value() != B_CONTROL_OFF)
			flags |= B_ARGV_ONLY;
		if (fBackgroundAppCheckBox->Value() != B_CONTROL_OFF)
			flags |= B_BACKGROUND_APP;
	}

	BMessage supportedTypes;
	for (int32 i = 0; i < fTypeListView->CountItems(); i++) {
		SupportedTypeItem* item = dynamic_cast<SupportedTypeItem*>(
			fTypeListView->ItemAt(i));

		supportedTypes.AddString("types", item->Type());
	}

	version_info versionInfo;
	versionInfo.major = atol(fMajorVersionControl->Text());
	versionInfo.middle = atol(fMiddleVersionControl->Text());
	versionInfo.minor = atol(fMinorVersionControl->Text());
	versionInfo.variety = fVarietyMenu->IndexOf(fVarietyMenu->FindMarked());
	versionInfo.internal = atol(fInternalVersionControl->Text());
	strlcpy(versionInfo.short_info, fShortDescriptionControl->Text(),
		sizeof(versionInfo.short_info));
	strlcpy(versionInfo.long_info, fLongDescriptionView->Text(),
		sizeof(versionInfo.long_info));

	// Save

	status = info.SetSignature(fSignatureControl->Text());
	if (status == B_OK)
		status = info.SetAppFlags(flags);
	if (status == B_OK)
		status = info.SetVersionInfo(&versionInfo, B_APP_VERSION_KIND);
	if (status == B_OK)
		fIcon.CopyTo(info, NULL, true);

	// supported types and their icons
	if (status == B_OK)
		status = info.SetSupportedTypes(&supportedTypes);

	for (int32 i = 0; i < fTypeListView->CountItems(); i++) {
		SupportedTypeItem* item = dynamic_cast<SupportedTypeItem*>(
			fTypeListView->ItemAt(i));

		item->Icon().CopyTo(info, item->Type(), true);
	}
}


void
ApplicationTypeWindow::FrameResized(float width, float height)
{
	// This works around a flaw of BTextView
	fLongDescriptionView->SetTextRect(fLongDescriptionView->Bounds());
}


void
ApplicationTypeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFlagsChanged:
			_UpdateAppFlagsEnabled();
			break;

		case kMsgSave:
			_Save();
			break;

		case kMsgTypeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				SupportedTypeItem* item = (SupportedTypeItem*)fTypeListView->ItemAt(index);

				fTypeIconView->SetTo(item != NULL ? &item->Icon() : NULL);
				fTypeIconView->SetEnabled(item != NULL);
				fRemoveTypeButton->SetEnabled(item != NULL);
			}
			break;
		}

		case kMsgAddType:
		{
			BWindow* window = new TypeListWindow(NULL,
				kMsgTypeAdded, this);
			window->Show();
			break;
		}

		case kMsgTypeAdded:
		{
			const char* type;
			if (message->FindString("type", &type) != B_OK)
				break;

			// check if this type already exists

			SupportedTypeItem* newItem = new SupportedTypeItem(type);
			int32 insertAt = 0;

			for (int32 i = fTypeListView->CountItems(); i-- > 0;) {
				SupportedTypeItem* item = dynamic_cast<SupportedTypeItem*>(
					fTypeListView->ItemAt(i));
				if (item == NULL)
					continue;

				int compare = strcasecmp(item->Type(), type);
				if (!compare) {
					// type does already exist, select it and bail out
					delete newItem;
					newItem = NULL;
					fTypeListView->Select(i);
					break;
				}
				if (compare < 0)
					insertAt = i + 1;
			}

			if (newItem == NULL)
				break;

			fTypeListView->AddItem(newItem, insertAt);
			fTypeListView->Select(insertAt);
			break;
		}

		case kMsgRemoveType:
		{
			int32 index = fTypeListView->CurrentSelection();
			if (index < 0)
				break;

			delete fTypeListView->RemoveItem(index);
			fTypeIconView->SetTo(NULL);
			fTypeIconView->SetEnabled(false);
			fRemoveTypeButton->SetEnabled(false);
			break;
		}

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

