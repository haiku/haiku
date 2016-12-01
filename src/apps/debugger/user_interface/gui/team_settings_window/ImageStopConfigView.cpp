/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ImageStopConfigView.h"

#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuField.h>
#include <ScrollView.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "MessageCodes.h"
#include "UserInterface.h"


enum {
	MSG_SET_STOP_FOR_ALL_IMAGES				= 'sfai',
	MSG_SET_STOP_FOR_CUSTOM_IMAGES			= 'sfci',
	MSG_IMAGE_NAME_SELECTION_CHANGED		= 'insc',
	MSG_ADD_IMAGE_NAME						= 'anin',
	MSG_REMOVE_IMAGE_NAME					= 'arin',
	MSG_IMAGE_NAME_INPUT_CHANGED			= 'inic'
};


static int SortStringItems(const void* a, const void* b)
{
	BStringItem* item1 = *(BStringItem**)a;
	BStringItem* item2 = *(BStringItem**)b;

	return strcmp(item1->Text(), item2->Text());
}


ImageStopConfigView::ImageStopConfigView(::Team* team,
	UserInterfaceListener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fListener(listener),
	fStopOnImageLoad(NULL),
	fStopImageConstraints(NULL),
	fStopImageNames(NULL),
	fStopImageNameInput(NULL),
	fAddImageNameButton(NULL),
	fRemoveImageNameButton(NULL),
	fCustomImageGroup(NULL),
	fStopOnLoadEnabled(false),
	fUseCustomImages(false)
{
	SetName("Images");
	fTeam->AddListener(this);
}


ImageStopConfigView::~ImageStopConfigView()
{
	fTeam->RemoveListener(this);
}


ImageStopConfigView*
ImageStopConfigView::Create(::Team* team, UserInterfaceListener* listener)
{
	ImageStopConfigView* self = new ImageStopConfigView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ImageStopConfigView::AttachedToWindow()
{
	fAddImageNameButton->SetEnabled(false);
	fRemoveImageNameButton->SetEnabled(false);

	fStopImageConstraints->Menu()->SetTargetForItems(this);
	fStopOnImageLoad->SetTarget(this);
	fAddImageNameButton->SetTarget(this);
	fRemoveImageNameButton->SetTarget(this);
	fStopImageNames->SetTarget(this);
	fStopImageNameInput->SetTarget(this);

	AutoLocker< ::Team> teamLocker(fTeam);
	_UpdateStopImageState();

	BGroupView::AttachedToWindow();
}


void
ImageStopConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_STOP_FOR_ALL_IMAGES:
		{
			fListener->SetStopOnImageLoadRequested(
				fStopOnImageLoad->Value() == B_CONTROL_ON,
				false);
			break;
		}

		case MSG_SET_STOP_FOR_CUSTOM_IMAGES:
		{
			fListener->SetStopOnImageLoadRequested(
				fStopOnImageLoad->Value() == B_CONTROL_ON,
				true);
			break;
		}

		case MSG_IMAGE_NAME_SELECTION_CHANGED:
		{
			if (!fUseCustomImages)
				break;

			fRemoveImageNameButton->SetEnabled(
				fStopImageNames->CurrentSelection() >= 0);
			break;
		}

		case MSG_IMAGE_NAME_INPUT_CHANGED:
		{
			BString imageName(fStopImageNameInput->Text());
			imageName.Trim();
			fAddImageNameButton->SetEnabled(!imageName.IsEmpty());
			break;
		}

		case MSG_STOP_ON_IMAGE_LOAD:
		{
			fListener->SetStopOnImageLoadRequested(
				fStopOnImageLoad->Value() == B_CONTROL_ON,
				fUseCustomImages);
			break;
		}

		case MSG_STOP_IMAGE_SETTINGS_CHANGED:
		{
			_UpdateStopImageState();
			break;
		}

		case MSG_ADD_IMAGE_NAME:
		{
			BString imageName(fStopImageNameInput->Text());
			imageName.Trim();
			AutoLocker< ::Team> teamLocker(fTeam);
			if (fTeam->StopImageNames().HasString(imageName))
				break;

			fStopImageNameInput->SetText("");
			fListener->AddStopImageNameRequested(imageName.String());
			break;
		}

		case MSG_STOP_IMAGE_NAME_ADDED:
		{
			const char* imageName;
			if (message->FindString("name", &imageName) != B_OK)
				break;

			BStringItem* item = new(std::nothrow) BStringItem(imageName);
			if (item == NULL)
				break;

			ObjectDeleter<BStringItem> itemDeleter(item);
			if (!fStopImageNames->AddItem(item)) {
				break;
			}
			itemDeleter.Detach();
			fStopImageNames->SortItems(SortStringItems);
			break;
		}

		case MSG_REMOVE_IMAGE_NAME:
		{
			BStringItem* item;
			int32 selectedIndex;
			AutoLocker< ::Team> teamLocker(fTeam);
			int32 i = 0;
			while ((selectedIndex = fStopImageNames->CurrentSelection(i++))
				>= 0) {
				item = (BStringItem*)fStopImageNames->ItemAt(selectedIndex);
				fListener->RemoveStopImageNameRequested(item->Text());
			}
			break;
		}

		case MSG_STOP_IMAGE_NAME_REMOVED:
		{
			const char* imageName;
			if (message->FindString("name", &imageName) != B_OK)
				break;

			for (int32 i = 0; i < fStopImageNames->CountItems(); i++) {
				BStringItem* item = (BStringItem*)fStopImageNames->ItemAt(i);
				if (strcmp(item->Text(), imageName) == 0) {
					fStopImageNames->RemoveItem(i);
					delete item;
				}
			}
			break;
		}


		default:
			BGroupView::MessageReceived(message);
			break;
	}

}


void
ImageStopConfigView::StopOnImageLoadSettingsChanged(
	const Team::ImageLoadEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_SETTINGS_CHANGED);
	message.AddBool("enabled", event.StopOnImageLoad());
	message.AddBool("useNameList", event.StopImageNameListEnabled());
	BMessenger(this).SendMessage(&message);
}


void
ImageStopConfigView::StopOnImageLoadNameAdded(
	const Team::ImageLoadNameEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_NAME_ADDED);
	message.AddString("name", event.ImageName());
	BMessenger(this).SendMessage(&message);
}


void
ImageStopConfigView::StopOnImageLoadNameRemoved(
	const Team::ImageLoadNameEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_NAME_REMOVED);
	message.AddString("name", event.ImageName());
	BMessenger(this).SendMessage(&message);
}


void
ImageStopConfigView::_Init()
{
	BMenu* stopImageMenu = new BMenu("stopImageTypesMenu");

	stopImageMenu->AddItem(new BMenuItem("All",
		new BMessage(MSG_SET_STOP_FOR_ALL_IMAGES)));
	stopImageMenu->AddItem(new BMenuItem("Custom",
		new BMessage(MSG_SET_STOP_FOR_CUSTOM_IMAGES)));

	fStopImageNames = new BListView("customImageList",
		B_MULTIPLE_SELECTION_LIST);
	fStopImageNames->SetSelectionMessage(
		new BMessage(MSG_IMAGE_NAME_SELECTION_CHANGED));

	fCustomImageGroup = new BGroupView();
	BLayoutBuilder::Group<>(fCustomImageGroup, B_VERTICAL, 0.0)
		.Add(new BScrollView("stopImageScroll", fStopImageNames,
			0, false, true))
		.Add(fStopImageNameInput = new BTextControl("stopImageName",
			"Image:", NULL,	NULL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(fAddImageNameButton = new BButton("Add",
				new BMessage(MSG_ADD_IMAGE_NAME)))
			.Add(fRemoveImageNameButton = new BButton("Remove",
				new BMessage(MSG_REMOVE_IMAGE_NAME)))
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(fStopOnImageLoad = new BCheckBox("stopOnImage",
			"Stop when an image is loaded",
			new BMessage(MSG_STOP_ON_IMAGE_LOAD)))
		.Add(fStopImageConstraints = new BMenuField(
			"stopTypes", "Types:", stopImageMenu))
		.Add(fCustomImageGroup);

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float minListHeight = 5 * (fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading);
	fStopImageNames->SetExplicitMinSize(BSize(B_SIZE_UNSET, minListHeight));

	stopImageMenu->SetLabelFromMarked(true);
	fStopImageNameInput->SetModificationMessage(
		new BMessage(MSG_IMAGE_NAME_INPUT_CHANGED));
}


void
ImageStopConfigView::_UpdateStopImageState()
{
	fStopOnLoadEnabled = fTeam->StopOnImageLoad();
	fStopOnImageLoad->SetValue(
		fStopOnLoadEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	fUseCustomImages = fTeam->StopImageNameListEnabled();
	fStopImageConstraints->Menu()
		->ItemAt(fUseCustomImages ? 1 : 0)->SetMarked(true);

	fStopImageNames->MakeEmpty();
	const BStringList& imageNames = fTeam->StopImageNames();
	for (int32 i = 0; i < imageNames.CountStrings(); i++) {
		BStringItem* item = new(std::nothrow) BStringItem(
			imageNames.StringAt(i));
		if (item == NULL)
			return;
		item->SetEnabled(fUseCustomImages);
		ObjectDeleter<BStringItem> itemDeleter(item);
		if (!fStopImageNames->AddItem(item))
			return;
		itemDeleter.Detach();
	}

	fStopImageConstraints->SetEnabled(fStopOnLoadEnabled);
	fStopImageNameInput->SetEnabled(fUseCustomImages);
}
