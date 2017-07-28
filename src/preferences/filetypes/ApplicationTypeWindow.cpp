/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ApplicationTypeWindow.h"
#include "DropTargetListView.h"
#include "FileTypes.h"
#include "IconView.h"
#include "PreferredAppMenu.h"
#include "StringView.h"
#include "TypeListWindow.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <File.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Roster.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application Type Window"


const uint32 kMsgSave = 'save';
const uint32 kMsgSignatureChanged = 'sgch';
const uint32 kMsgToggleAppFlags = 'tglf';
const uint32 kMsgAppFlagsChanged = 'afch';

const uint32 kMsgIconChanged = 'icch';
const uint32 kMsgTypeIconsChanged = 'tich';

const uint32 kMsgVersionInfoChanged = 'tvch';

const uint32 kMsgTypeSelected = 'tpsl';
const uint32 kMsgAddType = 'adtp';
const uint32 kMsgTypeAdded = 'tpad';
const uint32 kMsgRemoveType = 'rmtp';
const uint32 kMsgTypeRemoved = 'tprm';


//! TextView that filters the tab key to be able to tab-navigate while editing
class TabFilteringTextView : public BTextView {
public:
								TabFilteringTextView(const char* name,
									uint32 changedMessageWhat = 0);
	virtual						~TabFilteringTextView();

	virtual void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);
	virtual	void				KeyDown(const char* bytes, int32 count);
	virtual	void				TargetedByScrollView(BScrollView* scroller);
	virtual	void				MakeFocus(bool focused = true);

private:
			BScrollView*		fScrollView;
			uint32				fChangedMessageWhat;
};


class SupportedTypeItem : public BStringItem {
public:
								SupportedTypeItem(const char* type);
	virtual						~SupportedTypeItem();

			const char*			Type() const { return fType.String(); }
			::Icon&				Icon() { return fIcon; }

			void				SetIcon(::Icon* icon);
			void				SetIcon(entry_ref& ref, const char* type);

	static	int					Compare(const void* _a, const void* _b);

private:
			BString				fType;
			::Icon				fIcon;
};


class SupportedTypeListView : public DropTargetListView {
public:
								SupportedTypeListView(const char* name,
									list_view_type
										type = B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
	virtual						~SupportedTypeListView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				AcceptsDrag(const BMessage* message);
};


// #pragma mark -


TabFilteringTextView::TabFilteringTextView(const char* name,
	uint32 changedMessageWhat)
	:
	BTextView(name, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE),
	fScrollView(NULL),
	fChangedMessageWhat(changedMessageWhat)
{
}


TabFilteringTextView::~TabFilteringTextView()
{
}


void
TabFilteringTextView::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	BTextView::InsertText(text, length, offset, runs);
	if (fChangedMessageWhat != 0)
		Window()->PostMessage(fChangedMessageWhat);
}


void
TabFilteringTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	if (fChangedMessageWhat != 0)
		Window()->PostMessage(fChangedMessageWhat);
}


void
TabFilteringTextView::KeyDown(const char* bytes, int32 count)
{
	if (bytes[0] == B_TAB)
		BView::KeyDown(bytes, count);
	else
		BTextView::KeyDown(bytes, count);
}


void
TabFilteringTextView::TargetedByScrollView(BScrollView* scroller)
{
	fScrollView = scroller;
}


void
TabFilteringTextView::MakeFocus(bool focused)
{
	BTextView::MakeFocus(focused);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(focused);
}


// #pragma mark -


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


SupportedTypeListView::SupportedTypeListView(const char* name,
	list_view_type type, uint32 flags)
	:
	DropTargetListView(name, type, flags)
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


ApplicationTypeWindow::ApplicationTypeWindow(BPoint position,
	const BEntry& entry)
	:
	BWindow(BRect(0.0f, 0.0f, 250.0f, 340.0f).OffsetBySelf(position),
		B_TRANSLATE("Application type"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS |
			B_FRAME_EVENTS | B_AUTO_UPDATE_SIZE_LIMITS),
	fChangedProperties(0)
{
	float padding = be_control_look->DefaultItemSpacing();
	BAlignment labelAlignment = be_control_look->DefaultLabelAlignment();

	BMenuBar* menuBar = new BMenuBar((char*)NULL);
	menuBar->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));

	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(kMsgSave), 'S');
	fSaveMenuItem->SetEnabled(false);
	menu->AddItem(fSaveMenuItem);
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Save into resource file" B_UTF8_ELLIPSIS), NULL));
	item->SetEnabled(false);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W', B_COMMAND_KEY));
	menuBar->AddItem(menu);

	// Signature

	fSignatureControl = new BTextControl("signature",
		B_TRANSLATE("Signature:"), new BMessage(kMsgSignatureChanged));
	fSignatureControl->SetModificationMessage(
		new BMessage(kMsgSignatureChanged));

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = fSignatureControl->TextView();
	textView->SetMaxBytes(B_MIME_TYPE_LENGTH);
	const char* disallowedCharacters = "<>@,;:\"()[]?= ";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	// "Application Flags" group

	BBox* flagsBox = new BBox("flagsBox");

	fFlagsCheckBox = new BCheckBox("flags", B_TRANSLATE("Application flags"),
		new BMessage(kMsgToggleAppFlags));
	fFlagsCheckBox->SetValue(B_CONTROL_ON);

	fSingleLaunchButton = new BRadioButton("single",
		B_TRANSLATE("Single launch"), new BMessage(kMsgAppFlagsChanged));

	fMultipleLaunchButton = new BRadioButton("multiple",
		B_TRANSLATE("Multiple launch"), new BMessage(kMsgAppFlagsChanged));

	fExclusiveLaunchButton = new BRadioButton("exclusive",
		B_TRANSLATE("Exclusive launch"), new BMessage(kMsgAppFlagsChanged));

	fArgsOnlyCheckBox = new BCheckBox("args only", B_TRANSLATE("Args only"),
		new BMessage(kMsgAppFlagsChanged));

	fBackgroundAppCheckBox = new BCheckBox("background",
		B_TRANSLATE("Background app"), new BMessage(kMsgAppFlagsChanged));

	BLayoutBuilder::Grid<>(flagsBox, 0, 0)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fSingleLaunchButton, 0, 0).Add(fArgsOnlyCheckBox, 1, 0)
		.Add(fMultipleLaunchButton, 0, 1).Add(fBackgroundAppCheckBox, 1, 1)
		.Add(fExclusiveLaunchButton, 0, 2);
	flagsBox->SetLabel(fFlagsCheckBox);

	// "Icon" group

	BBox* iconBox = new BBox("IconBox");
	iconBox->SetLabel(B_TRANSLATE("Icon"));
	fIconView = new IconView("icon");
	fIconView->SetModificationMessage(new BMessage(kMsgIconChanged));
	BLayoutBuilder::Group<>(iconBox, B_HORIZONTAL)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fIconView);

	// "Supported Types" group

	BBox* typeBox = new BBox("typesBox");
	typeBox->SetLabel(B_TRANSLATE("Supported types"));

	fTypeListView = new SupportedTypeListView("Suppported Types",
		B_SINGLE_SELECTION_LIST);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("type scrollview", fTypeListView,
		B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	fAddTypeButton = new BButton("add type",
		B_TRANSLATE("Add" B_UTF8_ELLIPSIS), new BMessage(kMsgAddType));

	fRemoveTypeButton = new BButton("remove type", B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveType));

	fTypeIconView = new IconView("type icon");
	BGroupView* iconHolder = new BGroupView(B_HORIZONTAL);
	iconHolder->AddChild(fTypeIconView);
	fTypeIconView->SetModificationMessage(new BMessage(kMsgTypeIconsChanged));

	BLayoutBuilder::Grid<>(typeBox, padding, padding)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(scrollView, 0, 0, 1, 4)
		.Add(fAddTypeButton, 1, 0, 1, 2)
		.Add(fRemoveTypeButton, 1, 2, 1, 2)
		.Add(iconHolder, 2, 1, 1, 2)
		.SetColumnWeight(0, 3)
		.SetColumnWeight(1, 2)
		.SetColumnWeight(2, 1);

	iconHolder->SetExplicitAlignment(
		BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

	// "Version Info" group

	BBox* versionBox = new BBox("versionBox");
	versionBox->SetLabel(B_TRANSLATE("Version info"));

	fMajorVersionControl = new BTextControl("version major",
		B_TRANSLATE("Version:"), NULL, new BMessage(kMsgVersionInfoChanged));
	_MakeNumberTextControl(fMajorVersionControl);

	fMiddleVersionControl = new BTextControl("version middle", ".", NULL,
		new BMessage(kMsgVersionInfoChanged));
	_MakeNumberTextControl(fMiddleVersionControl);

	fMinorVersionControl = new BTextControl("version minor", ".", NULL,
		new BMessage(kMsgVersionInfoChanged));
	_MakeNumberTextControl(fMinorVersionControl);

	fVarietyMenu = new BPopUpMenu("variety", true, true);
	fVarietyMenu->AddItem(new BMenuItem(B_TRANSLATE("Development"),
		new BMessage(kMsgVersionInfoChanged)));
	fVarietyMenu->AddItem(new BMenuItem(B_TRANSLATE("Alpha"),
		new BMessage(kMsgVersionInfoChanged)));
	fVarietyMenu->AddItem(new BMenuItem(B_TRANSLATE("Beta"),
		new BMessage(kMsgVersionInfoChanged)));
	fVarietyMenu->AddItem(new BMenuItem(B_TRANSLATE("Gamma"),
		new BMessage(kMsgVersionInfoChanged)));
	item = new BMenuItem(B_TRANSLATE("Golden master"),
		new BMessage(kMsgVersionInfoChanged));
	fVarietyMenu->AddItem(item);
	item->SetMarked(true);
	fVarietyMenu->AddItem(new BMenuItem(B_TRANSLATE("Final"),
		new BMessage(kMsgVersionInfoChanged)));

	BMenuField* varietyField = new BMenuField("", fVarietyMenu);
	fInternalVersionControl = new BTextControl("version internal", "/", NULL,
		new BMessage(kMsgVersionInfoChanged));
	fShortDescriptionControl = new BTextControl("short description",
		B_TRANSLATE("Short description:"), NULL,
		new BMessage(kMsgVersionInfoChanged));

	// TODO: workaround for a GCC 4.1.0 bug? Or is that really what the standard says?
	version_info versionInfo;
	fShortDescriptionControl->TextView()->SetMaxBytes(
		sizeof(versionInfo.short_info));

	BStringView* longLabel = new BStringView(NULL,
		B_TRANSLATE("Long description:"));
	longLabel->SetExplicitAlignment(labelAlignment);
	fLongDescriptionView = new TabFilteringTextView("long desc",
		kMsgVersionInfoChanged);
	fLongDescriptionView->SetMaxBytes(sizeof(versionInfo.long_info));

	scrollView = new BScrollView("desc scrollview", fLongDescriptionView,
		B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	// TODO: remove workaround (bug #5678)
	BSize minScrollSize = scrollView->ScrollBar(B_VERTICAL)->MinSize();
	minScrollSize.width += fLongDescriptionView->MinSize().width;
	scrollView->SetExplicitMinSize(minScrollSize);

	// Manually set a minimum size for the version text controls
	// TODO: the same does not work when applied to the layout items
	float width = be_plain_font->StringWidth("99") + 16;
	fMajorVersionControl->TextView()->SetExplicitMinSize(
		BSize(width, fMajorVersionControl->MinSize().height));
	fMiddleVersionControl->TextView()->SetExplicitMinSize(
		BSize(width, fMiddleVersionControl->MinSize().height));
	fMinorVersionControl->TextView()->SetExplicitMinSize(
		BSize(width, fMinorVersionControl->MinSize().height));
	fInternalVersionControl->TextView()->SetExplicitMinSize(
		BSize(width, fInternalVersionControl->MinSize().height));

	BLayoutBuilder::Grid<>(versionBox, padding / 2, padding / 2)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fMajorVersionControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fMajorVersionControl->CreateTextViewLayoutItem(), 1, 0)
		.Add(fMiddleVersionControl, 2, 0, 2)
		.Add(fMinorVersionControl, 4, 0, 2)
		.Add(varietyField, 6, 0, 3)
		.Add(fInternalVersionControl, 9, 0, 2)
		.Add(fShortDescriptionControl->CreateLabelLayoutItem(), 0, 1)
		.Add(fShortDescriptionControl->CreateTextViewLayoutItem(), 1, 1, 10)
		.Add(longLabel, 0, 2)
		.Add(scrollView, 1, 2, 10, 3)
		.SetRowWeight(3, 3);

	// put it all together
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, 0, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, padding)
			.SetInsets(padding, padding, padding, padding)
			.Add(fSignatureControl)
			.AddGroup(B_HORIZONTAL, padding)
				.Add(flagsBox, 3)
				.Add(iconBox, 1)
				.End()
			.Add(typeBox)
			.Add(versionBox);

	SetKeyMenuBar(menuBar);

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

	BString title = B_TRANSLATE("%1 application type");
	title.ReplaceFirst("%1", name);
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

	version_info versionInfo;
	if (info.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) != B_OK)
		memset(&versionInfo, 0, sizeof(version_info));

	// Set Controls

	fSignatureControl->SetModificationMessage(NULL);
	fSignatureControl->SetText(signature);
	fSignatureControl->SetModificationMessage(
		new BMessage(kMsgSignatureChanged));

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

	fIconView->SetModificationMessage(NULL);
	fIconView->SetTo(&fIcon);
	fIconView->SetModificationMessage(new BMessage(kMsgIconChanged));

	// supported types

	BMessage supportedTypes;
	info.GetSupportedTypes(&supportedTypes);

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
	fTypeIconView->SetModificationMessage(NULL);
	fTypeIconView->SetTo(NULL);
	fTypeIconView->SetModificationMessage(new BMessage(kMsgTypeIconsChanged));
	fTypeIconView->SetEnabled(false);
	fRemoveTypeButton->SetEnabled(false);

	// version info

	char text[256];
	snprintf(text, sizeof(text), "%" B_PRId32, versionInfo.major);
	fMajorVersionControl->SetText(text);
	snprintf(text, sizeof(text), "%" B_PRId32, versionInfo.middle);
	fMiddleVersionControl->SetText(text);
	snprintf(text, sizeof(text), "%" B_PRId32, versionInfo.minor);
	fMinorVersionControl->SetText(text);

	if (versionInfo.variety >= (uint32)fVarietyMenu->CountItems())
		versionInfo.variety = 0;
	BMenuItem* item = fVarietyMenu->ItemAt(versionInfo.variety);
	if (item != NULL)
		item->SetMarked(true);

	snprintf(text, sizeof(text), "%" B_PRId32, versionInfo.internal);
	fInternalVersionControl->SetText(text);

	fShortDescriptionControl->SetText(versionInfo.short_info);
	fLongDescriptionView->SetText(versionInfo.long_info);

	// store original data

	fOriginalInfo.signature = signature;
	fOriginalInfo.gotFlags = gotFlags;
	fOriginalInfo.flags = gotFlags ? flags : 0;
	fOriginalInfo.versionInfo = versionInfo;
	fOriginalInfo.supportedTypes = _SupportedTypes();
		// The list view has the types sorted possibly differently
		// to the supportedTypes message, so don't use that here, but
		// get the sorted message instead.
	fOriginalInfo.iconChanged = false;
	fOriginalInfo.typeIconsChanged = false;

	fChangedProperties = 0;
	_CheckSaveMenuItem(0);
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
	bool gotFlags = _Flags(flags);
	BMessage supportedTypes = _SupportedTypes();
	version_info versionInfo = _VersionInfo();

	// Save

	status = info.SetSignature(fSignatureControl->Text());
	if (status == B_OK) {
		if (gotFlags)
			status = info.SetAppFlags(flags);
		else
			status = info.RemoveAppFlags();
	}
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

		if (item != NULL)
			item->Icon().CopyTo(info, item->Type(), true);
	}

	// reset the saved info
	fOriginalInfo.signature = fSignatureControl->Text();
	fOriginalInfo.gotFlags = gotFlags;
	fOriginalInfo.flags = flags;
	fOriginalInfo.versionInfo = versionInfo;
	fOriginalInfo.supportedTypes = supportedTypes;
	fOriginalInfo.iconChanged = false;
	fOriginalInfo.typeIconsChanged = false;

	fChangedProperties = 0;
	_CheckSaveMenuItem(0);
}


void
ApplicationTypeWindow::_CheckSaveMenuItem(uint32 flags)
{
	fChangedProperties = _NeedsSaving(flags);
	fSaveMenuItem->SetEnabled(fChangedProperties != 0);
}


bool
operator!=(const version_info& a, const version_info& b)
{
	return a.major != b.major || a.middle != b.middle || a.minor != b.minor
		|| a.variety != b.variety || a.internal != b.internal
		|| strcmp(a.short_info, b.short_info) != 0
		|| strcmp(a.long_info, b.long_info) != 0;
}


uint32
ApplicationTypeWindow::_NeedsSaving(uint32 _flags) const
{
	uint32 flags = fChangedProperties;
	if (_flags & CHECK_SIGNATUR) {
		if (fOriginalInfo.signature != fSignatureControl->Text())
			flags |= CHECK_SIGNATUR;
		else
			flags &= ~CHECK_SIGNATUR;
	}

	if (_flags & CHECK_FLAGS) {
		uint32 appFlags = 0;
		bool gotFlags = _Flags(appFlags);
		if (fOriginalInfo.gotFlags != gotFlags
			|| fOriginalInfo.flags != appFlags) {
			flags |= CHECK_FLAGS;
		} else
			flags &= ~CHECK_FLAGS;
	}

	if (_flags & CHECK_VERSION) {
		if (fOriginalInfo.versionInfo != _VersionInfo())
			flags |= CHECK_VERSION;
		else
			flags &= ~CHECK_VERSION;
	}

	if (_flags & CHECK_ICON) {
		if (fOriginalInfo.iconChanged)
			flags |= CHECK_ICON;
		else
			flags &= ~CHECK_ICON;
	}

	if (_flags & CHECK_TYPES) {
		if (!fOriginalInfo.supportedTypes.HasSameData(_SupportedTypes()))
			flags |= CHECK_TYPES;
		else
			flags &= ~CHECK_TYPES;
	}

	if (_flags & CHECK_TYPE_ICONS) {
		if (fOriginalInfo.typeIconsChanged)
			flags |= CHECK_TYPE_ICONS;
		else
			flags &= ~CHECK_TYPE_ICONS;
	}

	return flags;
}


// #pragma mark -


bool
ApplicationTypeWindow::_Flags(uint32& flags) const
{
	flags = 0;
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
		return true;
	}
	return false;
}


BMessage
ApplicationTypeWindow::_SupportedTypes() const
{
	BMessage supportedTypes;
	for (int32 i = 0; i < fTypeListView->CountItems(); i++) {
		SupportedTypeItem* item = dynamic_cast<SupportedTypeItem*>(
			fTypeListView->ItemAt(i));

		if (item != NULL)
			supportedTypes.AddString("types", item->Type());
	}
	return supportedTypes;
}


version_info
ApplicationTypeWindow::_VersionInfo() const
{
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
	return versionInfo;
}


// #pragma mark -


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
		case kMsgToggleAppFlags:
			_UpdateAppFlagsEnabled();
			_CheckSaveMenuItem(CHECK_FLAGS);
			break;

		case kMsgSignatureChanged:
			_CheckSaveMenuItem(CHECK_SIGNATUR);
			break;

		case kMsgAppFlagsChanged:
			_CheckSaveMenuItem(CHECK_FLAGS);
			break;

		case kMsgIconChanged:
			fOriginalInfo.iconChanged = true;
			_CheckSaveMenuItem(CHECK_ICON);
			break;

		case kMsgTypeIconsChanged:
			fOriginalInfo.typeIconsChanged = true;
			_CheckSaveMenuItem(CHECK_TYPE_ICONS);
			break;

		case kMsgVersionInfoChanged:
			_CheckSaveMenuItem(CHECK_VERSION);
			break;

		case kMsgSave:
			_Save();
			break;

		case kMsgTypeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				SupportedTypeItem* item
					= (SupportedTypeItem*)fTypeListView->ItemAt(index);

				fTypeIconView->SetModificationMessage(NULL);
				fTypeIconView->SetTo(item != NULL ? &item->Icon() : NULL);
				fTypeIconView->SetModificationMessage(
					new BMessage(kMsgTypeIconsChanged));
				fTypeIconView->SetEnabled(item != NULL);
				fRemoveTypeButton->SetEnabled(item != NULL);

				_CheckSaveMenuItem(CHECK_TYPES);
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

			_CheckSaveMenuItem(CHECK_TYPES);
			break;
		}

		case kMsgRemoveType:
		{
			int32 index = fTypeListView->CurrentSelection();
			if (index < 0)
				break;

			delete fTypeListView->RemoveItem(index);
			fTypeIconView->SetModificationMessage(NULL);
			fTypeIconView->SetTo(NULL);
			fTypeIconView->SetModificationMessage(
				new BMessage(kMsgTypeIconsChanged));
			fTypeIconView->SetEnabled(false);
			fRemoveTypeButton->SetEnabled(false);

			_CheckSaveMenuItem(CHECK_TYPES);
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

//			_CheckSaveMenuItem(...);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
ApplicationTypeWindow::QuitRequested()
{
	if (_NeedsSaving(CHECK_ALL) != 0) {
		BAlert* alert = new BAlert(B_TRANSLATE("Save request"),
			B_TRANSLATE("Save changes before closing?"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"),
			B_TRANSLATE("Save"), B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
			B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(1, 'd');
		alert->SetShortcut(2, 's');

		int32 choice = alert->Go();
		switch (choice) {
			case 0:
				return false;
			case 1:
				break;
			case 2:
				_Save();
				break;
		}
	}

	be_app->PostMessage(kMsgTypeWindowClosed);
	return true;
}

