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
	MSG_REMOVE_IMAGE_NAME					= 'arin'
};


BreakConditionConfigWindow::BreakConditionConfigWindow(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Configure break conditions", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fExceptionThrown(NULL),
	fExceptionCaught(NULL),
	fStopOnImageLoad(NULL),
	fStopImageConstraints(NULL),
	fStopImageNames(NULL),
	fStopImageNameInput(NULL),
	fAddImageNameButton(NULL),
	fRemoveImageNameButton(NULL),
	fCloseButton(NULL),
	fTarget(target)
{
}


BreakConditionConfigWindow::~BreakConditionConfigWindow()
{
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
			for (int32 i = 0; i < fStopImageNames->CountItems(); i++)
				fStopImageNames->ItemAt(i)->SetEnabled(false);
			fStopImageNameInput->SetEnabled(false);
			fAddImageNameButton->SetEnabled(false);
			fRemoveImageNameButton->SetEnabled(false);
			break;
		}

		case MSG_SET_STOP_FOR_CUSTOM_IMAGES:
		{
			for (int32 i = 0; i < fStopImageNames->CountItems(); i++)
				fStopImageNames->ItemAt(i)->SetEnabled(true);
			fStopImageNameInput->SetEnabled(true);
			fAddImageNameButton->SetEnabled(
				fStopImageNameInput->TextView()->TextLength() > 0);
			fRemoveImageNameButton->SetEnabled(
				fStopImageNames->CurrentSelection() >= 0);
			break;
		}

		case MSG_IMAGE_NAME_SELECTION_CHANGED:
		{
			fRemoveImageNameButton->SetEnabled(
				fStopImageNames->CurrentSelection() >= 0);
			break;
		}

		case MSG_STOP_ON_IMAGE_LOAD:
		{
			fListener->SetStopOnImageLoadRequested(
				fStopOnImageLoad->Value() == B_CONTROL_ON);
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
BreakConditionConfigWindow::_Init()
{
	BBox* exceptionSettingsBox = new BBox("exceptionBox");
	exceptionSettingsBox->SetLabel("Exceptions");
	exceptionSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fExceptionThrown = new BCheckBox("exceptionThrown",
				"Stop when an exception is thrown",
				new BMessage(MSG_STOP_ON_THROWN_EXCEPTION_CHANGED)))
			.Add(fExceptionCaught = new BCheckBox("exceptionCaught",
				"Stop when an exception is caught",
				new BMessage(MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED)))
		.End()
		.View());

	fExceptionThrown->SetTarget(this);
	fExceptionCaught->SetTarget(this);

	// TODO: enable once implemented
	fExceptionCaught->SetEnabled(false);


	BBox* imageSettingsBox = new BBox("imageBox");
	imageSettingsBox->SetLabel("Images");
	BMenu* stopImageMenu = new BMenu("stopImageTypesMenu");

	stopImageMenu->AddItem(new BMenuItem("All",
		new BMessage(MSG_SET_STOP_FOR_ALL_IMAGES)));
	stopImageMenu->AddItem(new BMenuItem("Custom",
		new BMessage(MSG_SET_STOP_FOR_CUSTOM_IMAGES)));

	BListView* fStopImageNames = new BListView("customImageList",
		B_MULTIPLE_SELECTION_LIST);
	fStopImageNames->SetSelectionMessage(
		new BMessage(MSG_IMAGE_NAME_SELECTION_CHANGED));

	imageSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fStopOnImageLoad = new BCheckBox("stopOnImage",
				"Stop when an image is loaded",
				new BMessage(MSG_STOP_ON_IMAGE_LOAD)))
			.Add(fStopImageConstraints = new BMenuField(
				"stopTypes", "Types:", stopImageMenu))
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
			.End()
		.End()
		.View());

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float minListHeight = 5 * (fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading);
	fStopImageNames->SetExplicitMinSize(BSize(B_SIZE_UNSET, minListHeight));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(exceptionSettingsBox)
		.Add(imageSettingsBox)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCloseButton = new BButton("Close", new BMessage(
					B_QUIT_REQUESTED)))
		.End();


	fCloseButton->SetTarget(this);
	stopImageMenu->SetTargetForItems(this);
	stopImageMenu->SetLabelFromMarked(true);
	stopImageMenu->ItemAt(0L)->SetMarked(true);

	// check if the exception breakpoints are already installed
	AutoLocker< ::Team> teamLocker(fTeam);
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
