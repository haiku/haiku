/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ExtensionWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <string.h>


const uint32 kMsgExtensionUpdated = 'exup';
const uint32 kMsgAccept = 'acpt';


static int
compare_extensions(const void* _a, const void* _b)
{
	const char* a = *(const char **)_a;
	const char* b = *(const char **)_b;

	int compare = strcasecmp(a, b);
	if (compare != 0)
		return compare;

	// sort lower case characters first
	return -strcmp(a, b);
}


status_t
add_extensions(BMimeType& type, BList& list, const char* removeExtension)
{
	BMessage extensions;
	status_t status = type.GetFileExtensions(&extensions);
	if (status < B_OK)
		return status;

	// replace the entry, and remove any equivalent entries
	BList newList;
	newList.AddList(&list);

	const char* extension;
	for (int32 i = 0; extensions.FindString("extensions", i,
			&extension) == B_OK; i++) {
		bool add = true;
		for (int32 j = list.CountItems(); j-- > 0;) {
			if ((removeExtension && !strcmp(removeExtension, extension))
				|| !strcmp((const char*)list.ItemAt(j), extension)) {
				// remove this item
				continue;
			}
		}

		if (add)
			newList.AddItem((void *)extension);
	}

	newList.SortItems(compare_extensions);

	// Copy them to a new message (their memory is still part of the
	// original BMessage)
	BMessage newExtensions;
	for (int32 i = 0; i < newList.CountItems(); i++) {
		newExtensions.AddString("extensions", (const char*)newList.ItemAt(i));
	}

	return type.SetFileExtensions(&newExtensions);
}


status_t
replace_extension(BMimeType& type, const char* newExtension, const char* oldExtension)
{
	BList list;
	list.AddItem((void *)newExtension);

	return add_extensions(type, list, oldExtension);
}


//	#pragma mark -


ExtensionWindow::ExtensionWindow(FileTypesWindow* target, BMimeType& type,
		const char* extension)
	: BWindow(BRect(100, 100, 350, 200), "Extension", B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fMimeType(type.Type()),
	fExtension(extension)
{
	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	rect.InsetBy(8.0f, 8.0f);
	fExtensionControl = new BTextControl(rect, "extension", "Extension:", extension,
		NULL, B_FOLLOW_LEFT_RIGHT);

	float labelWidth = fExtensionControl->StringWidth(fExtensionControl->Label()) + 2.0f;
	fExtensionControl->SetModificationMessage(new BMessage(kMsgExtensionUpdated));
	fExtensionControl->SetDivider(labelWidth);
	fExtensionControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of an extension
	BTextView* textView = fExtensionControl->TextView();
	const char* disallowedCharacters = "/:";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	float width, height;
	fExtensionControl->GetPreferredSize(&width, &height);
	fExtensionControl->ResizeTo(rect.Width(), height);
	topView->AddChild(fExtensionControl);

	fAcceptButton = new BButton(rect, "add", extension ? "Done" : "Add",
		new BMessage(kMsgAccept), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fAcceptButton->ResizeToPreferred();
	fAcceptButton->MoveTo(Bounds().Width() - 8.0f - fAcceptButton->Bounds().Width(),
		Bounds().Height() - 8.0f - fAcceptButton->Bounds().Height());
	fAcceptButton->SetEnabled(false);
	topView->AddChild(fAcceptButton);

	BButton* button = new BButton(rect, "cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(fAcceptButton->Frame().left - 10.0f - button->Bounds().Width(),
		fAcceptButton->Frame().top);
	topView->AddChild(button);

	ResizeTo(labelWidth * 4.0f + 24.0f, fExtensionControl->Bounds().Height()
		+ fAcceptButton->Bounds().Height() + 28.0f);
	SetSizeLimits(button->Bounds().Width() + fAcceptButton->Bounds().Width() + 26.0f,
		32767.0f, Frame().Height(), Frame().Height());

	// omit the leading dot
	if (fExtension.ByteAt(0) == '.')
		fExtension.Remove(0, 1);

	fAcceptButton->MakeDefault(true);
	fExtensionControl->MakeFocus(true);

	target->PlaceSubWindow(this);
	AddToSubset(target);
}


ExtensionWindow::~ExtensionWindow()
{
}


void
ExtensionWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgExtensionUpdated:
		{
			bool enabled = fExtensionControl->Text() != NULL
				&& fExtensionControl->Text()[0] != '\0';
			if (enabled) {
				// There is some text, but we only accept it, if it
				// changed the previous extension
				enabled = strcmp(fExtensionControl->Text(), fExtension.String());
			}

			if (fAcceptButton->IsEnabled() != enabled)
				fAcceptButton->SetEnabled(enabled);
			break;
		}

		case kMsgAccept:
		{
			const char* newExtension = fExtensionControl->Text();
			// omit the leading dot
			if (newExtension[0] == '.')
				newExtension++;

			status_t status = replace_extension(fMimeType, newExtension,
				fExtension.String());
			if (status != B_OK)
				error_alert("Could not change file extensions", status);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

