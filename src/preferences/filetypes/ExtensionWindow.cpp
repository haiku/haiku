/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ExtensionWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Extension Window"


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


//! newExtensionsList contains all the entries (char*) which are to be added.
status_t
merge_extensions(BMimeType& type, const BList& newExtensionsList,
	const char* removeExtension)
{
	BMessage extensions;
	status_t status = type.GetFileExtensions(&extensions);
	if (status < B_OK)
		return status;

	// replace the entry, and remove any equivalent entries
	BList mergedList;
	mergedList.AddList(&newExtensionsList);
	int32 originalCount = mergedList.CountItems();

	const char* extension;
	for (int32 i = 0; extensions.FindString("extensions", i,
			&extension) == B_OK; i++) {

		for (int32 j = originalCount; j-- > 0;) {
			if (!strcmp((const char*)mergedList.ItemAt(j), extension)) {
				// Do not add this old item again, since it's already
				// there.
				mergedList.RemoveItem(j);
				originalCount--;
			}
		}

		// The item will be added behind "originalCount", so we cannot
		// remove it accidentally in the next iterations, it's is added
		// for good.
		if (removeExtension == NULL || strcmp(removeExtension, extension))
			mergedList.AddItem((void *)extension);
	}

	mergedList.SortItems(compare_extensions);

	// Copy them to a new message (their memory is still part of the
	// original BMessage)
	BMessage newExtensions;
	for (int32 i = 0; i < mergedList.CountItems(); i++) {
		newExtensions.AddString("extensions",
			(const char*)mergedList.ItemAt(i));
	}

	return type.SetFileExtensions(&newExtensions);
}


status_t
replace_extension(BMimeType& type, const char* newExtension,
	const char* oldExtension)
{
	BList list;
	list.AddItem((void *)newExtension);

	return merge_extensions(type, list, oldExtension);
}


//	#pragma mark -


ExtensionWindow::ExtensionWindow(FileTypesWindow* target, BMimeType& type,
		const char* extension)
	: BWindow(BRect(100, 100, 350, 200), B_TRANSLATE("Extension"),
		B_MODAL_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target),
	fMimeType(type.Type()),
	fExtension(extension)
{
	fExtensionControl = new BTextControl(B_TRANSLATE("Extension:"),
		extension, NULL);
	fExtensionControl->SetModificationMessage(
		new BMessage(kMsgExtensionUpdated));
	fExtensionControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of an extension
	BTextView* textView = fExtensionControl->TextView();
	const char* disallowedCharacters = "/:";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	fAcceptButton = new BButton(extension
		? B_TRANSLATE("Done") : B_TRANSLATE("Add"),
		new BMessage(kMsgAccept));
	fAcceptButton->SetEnabled(false);

	BButton* button = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	float padding = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Grid<>(this, padding, padding)
		.SetInsets(padding, padding, padding, padding)
		.AddTextControl(fExtensionControl, 0, 0)
		.Add(fAcceptButton, 0, 1)
		.Add(button, 1, 1);

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
				error_alert(B_TRANSLATE("Could not change file extensions"),
					status);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
