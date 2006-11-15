/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeListView.h"
#include "AttributeWindow.h"
#include "DropTargetListView.h"
#include "ExtensionWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "IconView.h"
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
#include <stdlib.h>


const uint32 kMsgTypeSelected = 'typs';
const uint32 kMsgAddType = 'atyp';
const uint32 kMsgRemoveType = 'rtyp';

const uint32 kMsgExtensionSelected = 'exts';
const uint32 kMsgExtensionInvoked = 'exti';
const uint32 kMsgAddExtension = 'aext';
const uint32 kMsgRemoveExtension = 'rext';
const uint32 kMsgRuleEntered = 'rule';

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
const uint32 kMsgToggleRule = 'tgrl';

class TypeIconView : public IconView {
	public:
		TypeIconView(BRect frame, const char* name,
			int32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
		virtual ~TypeIconView();

		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float* _width, float* _height);

	protected:
		virtual BRect BitmapRect() const;
};

class ExtensionListView : public DropTargetListView {
	public:
		ExtensionListView(BRect frame, const char* name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
		virtual ~ExtensionListView();

		virtual void MessageReceived(BMessage* message);
		virtual bool AcceptsDrag(const BMessage* message);

		void SetType(BMimeType* type);

	private:
		BMimeType	fType;
};


//	#pragma mark -


TypeIconView::TypeIconView(BRect frame, const char* name, int32 resizingMode)
	: IconView(frame, name, resizingMode)
{
	ShowEmptyFrame(false);
}


TypeIconView::~TypeIconView()
{
}


void
TypeIconView::Draw(BRect updateRect)
{
	if (!IsEnabled())
		return;

	IconView::Draw(updateRect);

	const char* text = NULL;

	switch (IconSource()) {
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
	if (IconSource() == kNoIcon) {
		// center text in the middle of the icon
		y += (IconSize() - fontHeight.ascent - fontHeight.descent) / 2.0f;
	} else
		y += IconSize() + 3.0f;

	DrawString(text, BPoint(ceilf((Bounds().Width() - StringWidth(text)) / 2.0f),
		ceilf(y)));
}


void
TypeIconView::GetPreferredSize(float* _width, float* _height)
{
	if (_width) {
		float a = StringWidth("(from application)");
		float b = StringWidth("(from super type)");
		float width = max_c(a, b);
		if (width < IconSize())
			width = IconSize();

		*_width = ceilf(width);
	}

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = IconSize() + 3.0f + ceilf(fontHeight.ascent + fontHeight.descent);
	}
}


BRect
TypeIconView::BitmapRect() const
{
	if (IconSource() == kNoIcon) {
		// this also defines the drop target area
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		float width = StringWidth("no icon") + 8.0f;
		float height = ceilf(fontHeight.ascent + fontHeight.descent) + 6.0f;
		float x = (Bounds().Width() - width) / 2.0f;
		float y = ceilf((IconSize() - fontHeight.ascent - fontHeight.descent) / 2.0f) - 3.0f;

		return BRect(x, y, x + width, y + height);
	}

	float x = (Bounds().Width() - IconSize()) / 2.0f;
	return BRect(x, 0.0f, x + IconSize() - 1, IconSize() - 1);
}


//	#pragma mark -


ExtensionListView::ExtensionListView(BRect frame, const char* name,
		list_view_type type, uint32 resizeMask, uint32 flags)
	: DropTargetListView(frame, name, type, resizeMask, flags)
{
}


ExtensionListView::~ExtensionListView()
{
}


void
ExtensionListView::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && AcceptsDrag(message)) {
		// create extension list
		BList list;
		entry_ref ref;
		for (int32 index = 0; message->FindRef("refs", index++, &ref) == B_OK; ) {
			const char* point = strchr(ref.name, '.');
			if (point != NULL && point[1])
				list.AddItem(strdup(++point));
		}

		add_extensions(fType, list);

		// delete extension list
		for (int32 index = list.CountItems(); index-- > 0;) {
			free(list.ItemAt(index));
		}
	} else
		DropTargetListView::MessageReceived(message);
}


bool
ExtensionListView::AcceptsDrag(const BMessage* message)
{
	if (fType.Type() == NULL)
		return false;

	int32 count = 0;
	entry_ref ref;

	for (int32 index = 0; message->FindRef("refs", index++, &ref) == B_OK; ) {
		const char* point = strchr(ref.name, '.');
		if (point != NULL && point[1])
			count++;
	}

	return count > 0;
}


void
ExtensionListView::SetType(BMimeType* type)
{
	if (type != NULL)
		fType.SetTo(type->Type());
	else
		fType.Unset();
}


//	#pragma mark -


FileTypesWindow::FileTypesWindow(const BMessage& settings)
	: BWindow(_Frame(settings), "FileTypes", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fNewTypeWindow(NULL)
{
	bool showIcons;
	bool showRule;
	if (settings.FindBool("show_icons", &showIcons) != B_OK)
		showIcons = true;
	if (settings.FindBool("show_rule", &showRule) != B_OK)
		showRule = false;

	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem("New Resource File" B_UTF8_ELLIPSIS,
		NULL, 'N', B_COMMAND_KEY));
	item->SetEnabled(false);

	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, kSignature);
	item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel));
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
	item->SetMarked(showIcons);
	item->SetTarget(this);
	menu->AddItem(item);

	item = new BMenuItem("Show Recognition Rule", new BMessage(kMsgToggleRule));
	item->SetMarked(showRule);
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

	fTypeListView = new MimeTypeListView(rect, "typeview", NULL, showIcons, false,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fTypeListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	// "Icon" group

	font_height plainHeight;
	be_plain_font->GetHeight(&plainHeight);
	float height = ceilf(plainHeight.ascent + plainHeight.descent
		+ plainHeight.leading) + 2.0f;

	BFont font(be_bold_font);
	float labelWidth = font.StringWidth("Icon");
	font_height boldHeight;
	font.GetHeight(&boldHeight);

	BRect innerRect;
	fIconView = new TypeIconView(innerRect, "icon",
		B_FOLLOW_LEFT | B_FOLLOW_V_CENTER);
	fIconView->ResizeToPreferred();

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.right = rect.left + max_c(fIconView->Bounds().Width(), labelWidth) + 16.0f;
	rect.bottom = rect.top + ceilf(boldHeight.ascent)
		+ max_c(fIconView->Bounds().Height(),
			button->Bounds().Height() * 2.0f + height + 4.0f) + 12.0f;
	rect.top -= 2.0f;
	fIconBox = new BBox(rect);
	fIconBox->SetLabel("Icon");
	topView->AddChild(fIconBox);

	innerRect.left = 8.0f;
	innerRect.top = plainHeight.ascent + 3.0f
		+ (rect.Height() - boldHeight.ascent - fIconView->Bounds().Height()) / 2.0f;
	if (innerRect.top + fIconView->Bounds().Height() > fIconBox->Bounds().Height() - 6.0f)
		innerRect.top = fIconBox->Bounds().Height() - 6.0f - fIconView->Bounds().Height();
	fIconView->MoveTo(innerRect.LeftTop());
	fIconBox->AddChild(fIconView);

	// "File Recognition" group

	BRect rightRect(rect);
	rightRect.left = rect.right + 8.0f;
	rightRect.right = topView->Bounds().Width() - 8.0f;
	fRecognitionBox = new BBox(rightRect, NULL, B_FOLLOW_LEFT_RIGHT);
	fRecognitionBox->SetLabel("File Recognition");
	topView->AddChild(fRecognitionBox);

	innerRect = fRecognitionBox->Bounds().InsetByCopy(8.0f, 4.0f);
	innerRect.top += ceilf(boldHeight.ascent);
	fExtensionLabel = new StringView(innerRect, "extension", "Extensions:", NULL);
	fExtensionLabel->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);
	fExtensionLabel->ResizeToPreferred();
	fRecognitionBox->AddChild(fExtensionLabel);

	innerRect.top += fExtensionLabel->Bounds().Height() + 2.0f;
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddExtensionButton = new BButton(innerRect, "add ext", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddExtension), B_FOLLOW_RIGHT);
	fRecognitionBox->AddChild(fAddExtensionButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveExtensionButton = new BButton(innerRect, "remove ext", "Remove",
		new BMessage(kMsgRemoveExtension), B_FOLLOW_RIGHT);
	fRecognitionBox->AddChild(fRemoveExtensionButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = fAddExtensionButton->Frame().top + 2.0f;
	innerRect.bottom = innerRect.bottom - 2.0f;
		// take scrollview border into account
	fExtensionListView = new ExtensionListView(innerRect, "listview ext",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT);
	fExtensionListView->SetSelectionMessage(new BMessage(kMsgExtensionSelected));
	fExtensionListView->SetInvocationMessage(new BMessage(kMsgExtensionInvoked));

	scrollView = new BScrollView("scrollview ext", fExtensionListView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	fRecognitionBox->AddChild(scrollView);

	innerRect.left = 8.0f;
	innerRect.top = innerRect.bottom + 10.0f;
	innerRect.right = fRecognitionBox->Bounds().right - 8.0f;
	innerRect.bottom = innerRect.top + 20.0f;
	fRuleControl = new BTextControl(innerRect, "rule", "Rule:", "",
		new BMessage(kMsgRuleEntered), B_FOLLOW_LEFT_RIGHT);
	//fRuleControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fRuleControl->SetDivider(fRuleControl->StringWidth(fRuleControl->Label()) + 6.0f);
	fRuleControl->Hide();
	fRecognitionBox->AddChild(fRuleControl);

	// "Description" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(boldHeight.ascent) + 24.0f;
	rect.right = rightRect.right;
	fDescriptionBox = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	fDescriptionBox->SetLabel("Description");
	topView->AddChild(fDescriptionBox);

	innerRect = fDescriptionBox->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(boldHeight.ascent);
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fInternalNameView = new StringView(innerRect, "internal", "Internal Name:", "",
		B_FOLLOW_LEFT_RIGHT);
	labelWidth = fInternalNameView->StringWidth(fInternalNameView->Label()) + 2.0f;
	fInternalNameView->SetDivider(labelWidth);
	fInternalNameView->SetEnabled(false);
	fInternalNameView->ResizeToPreferred();
	fDescriptionBox->AddChild(fInternalNameView);

	innerRect.OffsetBy(0, fInternalNameView->Bounds().Height() + 5.0f);
	fTypeNameControl = new BTextControl(innerRect, "type", "Type Name:", "",
		new BMessage(kMsgTypeEntered), B_FOLLOW_LEFT_RIGHT);
	fTypeNameControl->SetDivider(labelWidth);
	fTypeNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fDescriptionBox->ResizeBy(0, fInternalNameView->Bounds().Height()
		+ fTypeNameControl->Bounds().Height() * 2.0f);
	fDescriptionBox->AddChild(fTypeNameControl);

	innerRect.OffsetBy(0, fTypeNameControl->Bounds().Height() + 5.0f);
	fDescriptionControl = new BTextControl(innerRect, "description", "Description:", "",
		new BMessage(kMsgDescriptionEntered), B_FOLLOW_LEFT_RIGHT);
	fDescriptionControl->SetDivider(labelWidth);
	fDescriptionControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fDescriptionBox->AddChild(fDescriptionControl);

	// "Preferred Application" group

	rect = fDescriptionBox->Frame();
	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(boldHeight.ascent)
		+ button->Bounds().Height() + 14.0f;
	fPreferredBox = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	fPreferredBox->SetLabel("Preferred Application");
	topView->AddChild(fPreferredBox);

	innerRect = fPreferredBox->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(boldHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Same As" B_UTF8_ELLIPSIS) - 24.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fSameAsButton = new BButton(innerRect, "same as", "Same As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSamePreferredAppAs), B_FOLLOW_RIGHT);
	fPreferredBox->AddChild(fSameAsButton);

	innerRect.OffsetBy(-innerRect.Width() - 6.0f, 0.0f);
	fSelectButton = new BButton(innerRect, "select", "Select" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSelectPreferredApp), B_FOLLOW_RIGHT);
	fPreferredBox->AddChild(fSelectButton);

	menu = new BPopUpMenu("preferred");
	menu->AddItem(item = new BMenuItem("None", new BMessage(kMsgPreferredAppChosen)));
	item->SetMarked(true);

	innerRect.right = innerRect.left - 6.0f;
	innerRect.left = 8.0f;
	BView* constrainingView = new BView(innerRect, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	constrainingView->SetViewColor(topView->ViewColor());

	fPreferredField = new BMenuField(innerRect.OffsetToCopy(B_ORIGIN), "preferred",
		NULL, menu);
	float width;
	fPreferredField->GetPreferredSize(&width, &height);
	fPreferredField->ResizeTo(innerRect.Width(), height);
	fPreferredField->MoveBy(0.0f, (innerRect.Height() - height) / 2.0f);
	constrainingView->AddChild(fPreferredField);
		// we embed the menu field in another view to make it behave like
		// we want so that it can't obscure other elements with larger
		// labels

	fPreferredBox->AddChild(constrainingView);

	// "Extra Attributes" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = topView->Bounds().Height() - 8.0f;
	fAttributeBox = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	fAttributeBox->SetLabel("Extra Attributes");
	topView->AddChild(fAttributeBox);

	innerRect = fAttributeBox->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(boldHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddAttributeButton = new BButton(innerRect, "add attr", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddAttribute), B_FOLLOW_RIGHT);
	fAttributeBox->AddChild(fAddAttributeButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveAttributeButton = new BButton(innerRect, "remove attr", "Remove",
		new BMessage(kMsgRemoveAttribute), B_FOLLOW_RIGHT);
	fAttributeBox->AddChild(fRemoveAttributeButton);
/*
	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	button = new BButton(innerRect, "push attr", "Push Up",
		new BMessage(kMsgRemoveAttribute), B_FOLLOW_RIGHT);
	fAttributeBox->AddChild(button);
*/
	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(boldHeight.ascent);
	innerRect.bottom = fAttributeBox->Bounds().bottom - 10.0f;
		// take scrollview border into account
	fAttributeListView = new AttributeListView(innerRect, "listview attr",
		B_FOLLOW_ALL);
	fAttributeListView->SetSelectionMessage(new BMessage(kMsgAttributeSelected));
	fAttributeListView->SetInvocationMessage(new BMessage(kMsgAttributeInvoked));

	scrollView = new BScrollView("scrollview attr", fAttributeListView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	fAttributeBox->AddChild(scrollView);

	SetSizeLimits(rightRect.left + 72.0f + font.StringWidth("jpg")
		+ font.StringWidth(fRecognitionBox->Label()), 32767.0f,
		rect.top + 2.0f * button->Bounds().Height() + boldHeight.ascent
		+ 32.0f + menuBar->Bounds().Height(), 32767.0f);

	_SetType(NULL);
	_ShowSnifferRule(showRule);

	BMimeType::StartWatching(this);
}


FileTypesWindow::~FileTypesWindow()
{
	BMimeType::StopWatching(this);
}


BRect
FileTypesWindow::_Frame(const BMessage& settings) const
{
	BRect rect;
	if (settings.FindRect("file_types_frame", &rect) == B_OK)
		return rect;

	return BRect(80.0f, 80.0f, 600.0f, 480.0f);
}


void
FileTypesWindow::_ShowSnifferRule(bool show)
{
	if (fRuleControl->IsHidden() == !show)
		return;

	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	float diff = fRuleControl->Bounds().Height() + 8.0f;

	if (!show) {
		fRuleControl->Hide();
		diff = -diff;
	}

	// adjust other controls to make space or take it again

	fIconBox->ResizeBy(0.0f, diff);
	fRecognitionBox->ResizeBy(0.0f, diff);
	fDescriptionBox->MoveBy(0.0f, diff);
	fPreferredBox->MoveBy(0.0f, diff);
	fAttributeBox->MoveBy(0.0f, diff);
	fAttributeBox->ResizeBy(0.0f, -diff);

	if (show)
		fRuleControl->Show();

	SetSizeLimits(minWidth, maxWidth, minHeight + diff, maxHeight);
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
	if (type != NULL)
		fIconView->SetTo(*type);
	else
		fIconView->Unset();
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

		if ((forceUpdate & B_SNIFFER_RULE_CHANGED) != 0) {
			BString rule;
			if (type->GetSnifferRule(&rule) != B_OK)
				rule = "";
			fRuleControl->SetText(rule.String());
		}

		fExtensionListView->SetType(&fCurrentType);
	} else {
		fCurrentType.Unset();
		fInternalNameView->SetText(NULL);
		fTypeNameControl->SetText(NULL);
		fDescriptionControl->SetText(NULL);
		fRuleControl->SetText(NULL);
		fPreferredField->Menu()->ItemAt(0)->SetMarked(true);
		fExtensionListView->SetType(NULL);
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

	fExtensionLabel->SetEnabled(enabled);
	fAddExtensionButton->SetEnabled(enabled);
	fRemoveExtensionButton->SetEnabled(false);
	fRuleControl->SetEnabled(enabled);

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

			// update settings
			BMessage update(kMsgSettingsChanged);
			update.AddBool("show_icons", item->IsMarked());
			be_app_messenger.SendMessage(&update);
			break;
		}

		case kMsgToggleRule:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void **)&item) != B_OK)
				break;

			item->SetMarked(fRuleControl->IsHidden());
			_ShowSnifferRule(item->IsMarked());

			// update settings
			BMessage update(kMsgSettingsChanged);
			update.AddBool("show_rule", item->IsMarked());
			be_app_messenger.SendMessage(&update);
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

		// File Recognition group

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

		case kMsgRuleEntered:
		{
			// check rule
			BString parseError;
			if (BMimeType::CheckSnifferRule(fRuleControl->Text(), &parseError) != B_OK) {
				parseError.Prepend("Recognition rule is not valid:\n\n");
				error_alert(parseError.String());
			} else
				fCurrentType.SetSnifferRule(fRuleControl->Text());
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
#ifdef __HAIKU__
					|| which == B_SUPPORTED_TYPES_CHANGED
#endif
					|| which == B_PREFERRED_APP_CHANGED)
					_UpdatePreferredApps(&fCurrentType);
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
	BMessage update(kMsgSettingsChanged);
	update.AddRect("file_types_frame", Frame());
	be_app_messenger.SendMessage(&update);

	be_app->PostMessage(kMsgTypesWindowClosed);
	return true;
}


