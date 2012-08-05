/*
 * Copyright 2010-2011, Hamish Morrison, hamish@lavabit.com 
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "SettingsWindow.h"
#include "Settings.h"

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <StringView.h>
#include <String.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Screen.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"


static const uint32 kMsgDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgSwapEnabledUpdate = 'swen';
static const uint32 kMsgVolumeSelected = 'vlsl';
static const int64 kMegaByte = 1024 * 1024;


class SizeSlider : public BSlider {
	public:
		SizeSlider(const char* name, const char* label,
			BMessage* message, int32 min, int32 max, uint32 flags);
		virtual ~SizeSlider();

		virtual const char* UpdateText() const;

	private:
		mutable BString	fText;
};


SizeSlider::SizeSlider(const char* name, const char* label,
	BMessage* message, int32 min, int32 max, uint32 flags)
	: BSlider(name, label, message, min, max, B_HORIZONTAL, B_BLOCK_THUMB, flags)
{
	rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &color);
}


SizeSlider::~SizeSlider()
{
}


const char*
byte_string(int64 size)
{
	double value = 1. * size;
	static char string[256];

	if (value < 1024)
		snprintf(string, sizeof(string), B_TRANSLATE("%Ld B"), size);
	else {
		static const char *units[] = {
			B_TRANSLATE_MARK("KB"),
			B_TRANSLATE_MARK("MB"),
			B_TRANSLATE_MARK("GB"),
			NULL
		};
		int32 i = -1;

		do {
			value /= 1024.0;
			i++;
		} while (value >= 1024 && units[i + 1]);

		off_t rounded = off_t(value * 100LL);
		snprintf(string, sizeof(string), "%g %s", rounded / 100.0,
			B_TRANSLATE_NOCOLLECT(units[i]));
	}

	return string;
}


const char*
SizeSlider::UpdateText() const
{
	fText = byte_string(Value() * kMegaByte);
	return fText.String();
}


class VolumeMenuItem : public BMenuItem {
	public:
		VolumeMenuItem(const char* label, BMessage* message, BVolume* volume);
	    BVolume* Volume() { return fVolume; }

	private:
		BVolume* fVolume;
};


VolumeMenuItem::VolumeMenuItem(const char* label, BMessage* message,
	BVolume* volume)
	: BMenuItem(label, message)
{
	fVolume = volume;
}


SettingsWindow::SettingsWindow()
	:
	BWindow(BRect(0, 0, 269, 172), B_TRANSLATE_SYSTEM_NAME("VirtualMemory"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
		| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fLocked(false)
	
{
	BView* view = new BGroupView();

	fSwapEnabledCheckBox = new BCheckBox("enable swap",
		B_TRANSLATE("Enable virtual memory"),
		new BMessage(kMsgSwapEnabledUpdate));

	BBox* box = new BBox("box", B_FOLLOW_LEFT_RIGHT);
	box->SetLabel(fSwapEnabledCheckBox);

	system_info info;
	get_system_info(&info);

	BString string = B_TRANSLATE("Physical memory: ");
	string << byte_string((off_t)info.max_pages * B_PAGE_SIZE);
	BStringView* memoryView = new BStringView("physical memory", string.String());

	string = B_TRANSLATE("Current swap file size: ");
	string << byte_string(fSettings.SwapSize());
	BStringView* swapfileView = new BStringView("current swap size", string.String());

	BPopUpMenu* menu = new BPopUpMenu("invalid");

	// collect volumes
	// TODO: listen to volume changes!
	// TODO: accept dropped volumes

	BVolumeRoster volumeRoster;
	BVolume* volume = new BVolume();
	char name[B_FILE_NAME_LENGTH];
	while (volumeRoster.GetNextVolume(volume) == B_OK) {	
		if (!volume->IsPersistent() || volume->GetName(name) != B_OK || !name[0])
			continue;
		VolumeMenuItem* item = new VolumeMenuItem(name, 
			new BMessage(kMsgVolumeSelected), volume);
		menu->AddItem(item);
		volume = new BVolume();
	}

	fVolumeMenuField = new BMenuField("volumes", B_TRANSLATE("Use volume:"), menu);
	
	fSizeSlider = new SizeSlider("size slider", 
		B_TRANSLATE("Requested swap file size:"), new BMessage(kMsgSliderUpdate),
		1, 1, B_WILL_DRAW | B_FRAME_EVENTS);
	fSizeSlider->SetViewColor(255, 0, 255);

	fWarningStringView = new BStringView("", "");
	fWarningStringView->SetAlignment(B_ALIGN_CENTER);

	view->SetLayout(new BGroupLayout(B_HORIZONTAL));
	view->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.AddGroup(B_HORIZONTAL)
			.Add(memoryView)
			.AddGlue()
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(swapfileView)
			.AddGlue()
		.End()
#ifdef SWAP_VOLUME_IMPLEMENTED
		.AddGroup(B_HORIZONTAL)
			.Add(fVolumeMenuField)
			.AddGlue()
		.End()
#else
		.AddGlue()
#endif
		.Add(fSizeSlider)
		.Add(fWarningStringView)
		.SetInsets(10, 10, 10, 10)
	);
	box->AddChild(view);

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(box)
		.AddGroup(B_HORIZONTAL, 10)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	BScreen screen;
	BRect screenFrame = screen.Frame();
	if (!screenFrame.Contains(fSettings.WindowPosition()))
		CenterOnScreen();
	else
		MoveTo(fSettings.WindowPosition());

#ifdef SWAP_VOLUME_IMPLEMENTED
	// Validate the volume specified in settings file
	status_t result = fSettings.SwapVolume().InitCheck();

	if (result != B_OK) {
		BAlert* alert = new BAlert("VirtualMemory", B_TRANSLATE(
			"The swap volume specified in the settings file is invalid.\n"
			"You can keep the current setting or switch to the "
			"default swap volume."),
			B_TRANSLATE("Keep"), B_TRANSLATE("Switch"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		int32 choice = alert->Go();
		if (choice == 1) {
			BVolumeRoster volumeRoster;
			BVolume bootVolume;
			volumeRoster.GetBootVolume(&bootVolume);
			fSettings.SetSwapVolume(bootVolume);
		}
	}
#endif

	_Update();
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRevert:
			fSettings.RevertSwapChanges();
			_Update();
			break;
		case kMsgDefaults:
			_SetSwapDefaults();
			_Update();
			break;
		case kMsgSliderUpdate:
			fSettings.SetSwapSize((off_t)fSizeSlider->Value() * kMegaByte);
			_Update();
			break;
		case kMsgVolumeSelected:
			fSettings.SetSwapVolume(*((VolumeMenuItem*)fVolumeMenuField->Menu()
				->FindMarked())->Volume());
			_Update();
			break;
		case kMsgSwapEnabledUpdate:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) != B_OK)
				break;

			if (value == 0) {
				// print out warning, give the user the time to think about it :)
				// ToDo: maybe we want to remove this possibility in the GUI
				// as Be did, but I thought a proper warning could be helpful
				// (for those that want to change that anyway)
				BAlert* alert = new BAlert("VirtualMemory",
					B_TRANSLATE(
					"Disabling virtual memory will have unwanted effects on "
					"system stability once the memory is used up.\n"
					"Virtual memory does not affect system performance "
					"until this point is reached.\n\n"
					"Are you really sure you want to turn it off?"),
					B_TRANSLATE("Turn off"), B_TRANSLATE("Keep enabled"), NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetShortcut(1, B_ESCAPE);
				int32 choice = alert->Go();
				if (choice == 1) {
					fSwapEnabledCheckBox->SetValue(1);
					break;
				}
			}

			fSettings.SetSwapEnabled(value != 0);
			_Update();
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
SettingsWindow::QuitRequested()
{
	fSettings.SetWindowPosition(Frame().LeftTop());
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
SettingsWindow::_Update()
{
	if ((fSwapEnabledCheckBox->Value() != 0) != fSettings.SwapEnabled())
		fSwapEnabledCheckBox->SetValue(fSettings.SwapEnabled());

#ifdef SWAP_VOLUME_IMPLEMENTED	
	if (fVolumeMenuField->IsEnabled() != fSettings.SwapEnabled())
		fVolumeMenuField->SetEnabled(fSettings.SwapEnabled());
	VolumeMenuItem* selectedVolumeItem =
		(VolumeMenuItem*)fVolumeMenuField->Menu()->FindMarked();
	if (selectedVolumeItem == NULL) {
		VolumeMenuItem* currentVolumeItem;
		int32 items = fVolumeMenuField->Menu()->CountItems();
		for (int32 index = 0; index < items; ++index) {
			currentVolumeItem = ((VolumeMenuItem*)fVolumeMenuField->Menu()->ItemAt(index));
			if (*(currentVolumeItem->fVolume) == fSettings.SwapVolume()) {
				currentVolumeItem->SetMarked(true);
				break;
			}
		}
	} else if (*selectedVolumeItem->fVolume != fSettings.SwapVolume()) {
		VolumeMenuItem* currentVolumeItem;
		int32 items = fVolumeMenuField->Menu()->CountItems();
		for (int32 index = 0; index < items; ++index) {
			currentVolumeItem = ((VolumeMenuItem*)fVolumeMenuField->Menu()->ItemAt(index));
			if (*(currentVolumeItem->fVolume) == fSettings.SwapVolume()) {
				currentVolumeItem->SetMarked(true);
				break;
			}
		}
	}
#endif

	fWarningStringView->SetText("");
	fLocked = false;
	
	if (fSettings.IsRevertible())
			fWarningStringView->SetText(
				B_TRANSLATE("Changes will take effect on restart!"));
	if (fRevertButton->IsEnabled() != fSettings.IsRevertible())
		fRevertButton->SetEnabled(fSettings.IsRevertible());
	
	off_t minSize, maxSize;
	if (_GetSwapFileLimits(minSize, maxSize) == B_OK) {
		// round to nearest MB -- slider steps in whole MBs
		(minSize >>= 20) <<= 20;
		(maxSize >>= 20) <<= 20;
		BString minLabel, maxLabel;
		minLabel << byte_string(minSize);
		maxLabel << byte_string(maxSize);
		if (minLabel != fSizeSlider->MinLimitLabel()
			|| maxLabel != fSizeSlider->MaxLimitLabel()) {
			fSizeSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
			fSizeSlider->SetLimits(minSize / kMegaByte, maxSize / kMegaByte);
		}
	} else if (fSettings.SwapEnabled()) {
		fWarningStringView->SetText(
			B_TRANSLATE("Insufficient space for a swap file."));
		fLocked = true;
	}
	if (fSizeSlider->Value() != fSettings.SwapSize() / kMegaByte)
		fSizeSlider->SetValue(fSettings.SwapSize() / kMegaByte);
	if (fSizeSlider->IsEnabled() != fSettings.SwapEnabled() || fLocked)
	{
		fSizeSlider->SetEnabled(fSettings.SwapEnabled() && !fLocked);
		fSettings.SetSwapSize((off_t)fSizeSlider->Value() * kMegaByte);
	}
}


status_t
SettingsWindow::_GetSwapFileLimits(off_t& minSize, off_t& maxSize)
{
	minSize = kMegaByte;

	// maximum size is the free space on the current volume
	// (minus some safety offset, depending on the disk size)
	off_t freeSpace = fSettings.SwapVolume().FreeBytes();
	off_t safetyFreeSpace = fSettings.SwapVolume().Capacity() / 100;
	if (safetyFreeSpace > 1024 * kMegaByte)
		safetyFreeSpace = 1024 * kMegaByte;

	// check if there already is a page file on this disk and
	// adjust the free space accordingly
	BPath path;
	if (find_directory(B_COMMON_VAR_DIRECTORY, &path, false,
		&fSettings.SwapVolume()) == B_OK) {
		path.Append("swap");
		BEntry swap(path.Path());

		off_t size;
		if (swap.GetSize(&size) == B_OK) {
			// If swap file exists, forget about safety space;
			// disk may have filled after creation of swap file.
			safetyFreeSpace = 0;
			freeSpace += size;
		}
	}

	maxSize = freeSpace - safetyFreeSpace;
	if (maxSize < minSize) {
		maxSize = 0;
		minSize = 0;
		return B_ERROR;
	}

	return B_OK;
}


void
SettingsWindow::_SetSwapDefaults()
{
	fSettings.SetSwapEnabled(true);

	BVolumeRoster volumeRoster;
	BVolume temporaryVolume;
	volumeRoster.GetBootVolume(&temporaryVolume);
	fSettings.SetSwapVolume(temporaryVolume);

	system_info info;
	get_system_info(&info);
	
	off_t defaultSize = (off_t)info.max_pages * B_PAGE_SIZE;
	off_t minSize, maxSize;
	_GetSwapFileLimits(minSize, maxSize);
	
	if (defaultSize > maxSize / 2)
		defaultSize = maxSize / 2;
		
	fSettings.SetSwapSize(defaultSize);
}

