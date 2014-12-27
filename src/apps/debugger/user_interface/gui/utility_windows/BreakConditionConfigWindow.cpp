/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "BreakConditionConfigWindow.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuField.h>
#include <ScrollView.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "MessageCodes.h"
#include "UserInterface.h"
#include "Team.h"


enum {
	MSG_STOP_ON_THROWN_EXCEPTION_CHANGED	= 'stec',
	MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED	= 'scec',
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


BreakConditionConfigWindow::BreakConditionConfigWindow(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Configure break conditions", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fExceptionSettingsBox(NULL),
	fImageSettingsBox(NULL),
	fExceptionThrown(NULL),
	fExceptionCaught(NULL),
	fStopOnImageLoad(NULL),
	fStopImageConstraints(NULL),
	fStopImageNames(NULL),
	fStopImageNameInput(NULL),
	fAddImageNameButton(NULL),
	fRemoveImageNameButton(NULL),
	fCustomImageGroup(NULL),
	fStopOnLoadEnabled(false),
	fUseCustomImages(false),
	fCloseButton(NULL),
	fTarget(target)
{
	fTeam->AddListener(this);
}


BreakConditionConfigWindow::~BreakConditionConfigWindow()
{
	fTeam->RemoveListener(this);
	BMessenger(fTarget).SendMessage(MSG_BREAK_CONDITION_CONFIG_WINDOW_CLOSED);
}


BreakConditionConfigWindow*
BreakConditionConfigWindow::Create(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
{
	BreakConditionConfigWindow* self = new BreakConditionConfigWindow(
		team, listener, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}

void
BreakConditionConfigWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_STOP_ON_THROWN_EXCEPTION_CHANGED:
		{
			_UpdateThrownBreakpoints(fExceptionThrown->Value()
				== B_CONTROL_ON);
			break;
		}

		case MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED:
		{
			break;
		}

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
			BWindow::MessageReceived(message);
			break;
	}

}


void
BreakConditionConfigWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
BreakConditionConfigWindow::StopOnImageLoadSettingsChanged(
	const Team::ImageLoadEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_SETTINGS_CHANGED);
	message.AddBool("enabled", event.StopOnImageLoad());
	message.AddBool("useNameList", event.StopImageNameListEnabled());
	PostMessage(&message);
}


void
BreakConditionConfigWindow::StopOnImageLoadNameAdded(
	const Team::ImageLoadNameEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_NAME_ADDED);
	message.AddString("name", event.ImageName());
	PostMessage(&message);
}


void
BreakConditionConfigWindow::StopOnImageLoadNameRemoved(
	const Team::ImageLoadNameEvent& event)
{
	BMessage message(MSG_STOP_IMAGE_NAME_REMOVED);
	message.AddString("name", event.ImageName());
	PostMessage(&message);
}


void
BreakConditionConfigWindow::_Init()
{
	fExceptionSettingsBox = new BBox("exceptionBox");
	fExceptionSettingsBox->SetLabel("Exceptions");
	fExceptionSettingsBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fExceptionThrown = new BCheckBox("exceptionThrown",
			"Stop when an exception is thrown",
			new BMessage(MSG_STOP_ON_THROWN_EXCEPTION_CHANGED)))
		.Add(fExceptionCaught = new BCheckBox("exceptionCaught",
			"Stop when an exception is caught",
			new BMessage(MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED)))
		.View());

	fExceptionThrown->SetTarget(this);
	fExceptionCaught->SetTarget(this);

	// TODO: enable once implemented
	fExceptionCaught->SetEnabled(false);


	fImageSettingsBox = new BBox("imageBox");
	fImageSettingsBox->SetLabel("Images");
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
			.AddGlue()
			.Add(fAddImageNameButton = new BButton("Add",
				new BMessage(MSG_ADD_IMAGE_NAME)))
			.Add(fRemoveImageNameButton = new BButton("Remove",
				new BMessage(MSG_REMOVE_IMAGE_NAME)))
		.End();

	fImageSettingsBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fStopOnImageLoad = new BCheckBox("stopOnImage",
			"Stop when an image is loaded",
			new BMessage(MSG_STOP_ON_IMAGE_LOAD)))
		.Add(fStopImageConstraints = new BMenuField(
			"stopTypes", "Types:", stopImageMenu))
		.Add(fCustomImageGroup)
		.View());

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float minListHeight = 5 * (fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading);
	fStopImageNames->SetExplicitMinSize(BSize(B_SIZE_UNSET, minListHeight));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fExceptionSettingsBox)
		.Add(fImageSettingsBox)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCloseButton = new BButton("Close", new BMessage(
					B_QUIT_REQUESTED)))
		.End();


	fCloseButton->SetTarget(this);
	fAddImageNameButton->SetEnabled(false);
	fRemoveImageNameButton->SetEnabled(false);
	stopImageMenu->SetTargetForItems(this);
	stopImageMenu->SetLabelFromMarked(true);
	fStopImageNameInput->SetModificationMessage(
		new BMessage(MSG_IMAGE_NAME_INPUT_CHANGED));

	fCustomImageGroup->Hide();

	AutoLocker< ::Team> teamLocker(fTeam);
	_UpdateStopImageState();
	_UpdateExceptionState();

}


void
BreakConditionConfigWindow::_UpdateThrownBreakpoints(bool enable)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		it.HasNext();) {
		Image* image = it.Next();

		ImageDebugInfo* info = image->GetImageDebugInfo();
		target_addr_t address;
		if (_FindExceptionFunction(info, address) != B_OK)
			continue;

		if (enable)
			fListener->SetBreakpointRequested(address, true, true);
		else
			fListener->ClearBreakpointRequested(address);
	}
}


status_t
BreakConditionConfigWindow::_FindExceptionFunction(ImageDebugInfo* info,
	target_addr_t& _foundAddress) const
{
	if (info != NULL) {
		FunctionInstance* instance = info->FunctionByName(
			"__cxa_allocate_exception");
		if (instance == NULL)
			instance = info->FunctionByName("__throw(void)");

		if (instance != NULL) {
			_foundAddress = instance->Address();
			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}


void
BreakConditionConfigWindow::_UpdateExceptionState()
{
	// check if the exception breakpoints are already installed
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		it.HasNext();) {
		Image* image = it.Next();

		ImageDebugInfo* info = image->GetImageDebugInfo();
		target_addr_t address;
		if (_FindExceptionFunction(info, address) != B_OK)
			continue;

		if (fTeam->BreakpointAtAddress(address) != NULL) {
			fExceptionThrown->SetValue(B_CONTROL_ON);
			break;
		}
	}
}


void
BreakConditionConfigWindow::_UpdateStopImageState()
{
	bool previousStop = fStopOnLoadEnabled;
	bool previousCustomImages = fUseCustomImages && fStopOnLoadEnabled;

	fStopOnLoadEnabled = fTeam->StopOnImageLoad();
	fStopOnImageLoad->SetValue(
		fStopOnLoadEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	fUseCustomImages = fTeam->StopImageNameListEnabled();
	fStopImageConstraints->Menu()
		->ItemAt(fTeam->StopImageNameListEnabled() ? 1 : 0)->SetMarked(true);

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

	_UpdateStopImageButtons(previousStop, previousCustomImages);
}


void
BreakConditionConfigWindow::_UpdateStopImageButtons(bool previousStop,
	bool previousCustomImages)
{
	fStopImageConstraints->SetEnabled(fStopOnLoadEnabled);
	bool showCustomGroup = fUseCustomImages && fStopOnLoadEnabled;
	if (!previousCustomImages && showCustomGroup)
		fCustomImageGroup->Show();
	else if (previousCustomImages && !showCustomGroup)
		fCustomImageGroup->Hide();
}
