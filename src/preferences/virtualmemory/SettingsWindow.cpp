/*
 * Copyright 2010-2011, Hamish Morrison, hamish@lavabit.com 
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "SettingsWindow.h"

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StringForSize.h>
#include <StringView.h>
#include <String.h>
#include <Slider.h>
#include <system_info.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Settings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"


static const uint32 kMsgDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgSwapEnabledUpdate = 'swen';
static const uint32 kMsgVolumeSelected = 'vlsl';
static const off_t kMegaByte = 1024 * 1024;
static dev_t bootDev = -1;


SizeSlider::SizeSlider(const char* name, const char* label,
	BMessage* message, int32 min, int32 max, uint32 flags)
	:
	BSlider(name, label, message, min, max, B_HORIZONTAL,
		B_BLOCK_THUMB, flags)
{
	rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &color);
}


const char*
SizeSlider::UpdateText() const
{
	return string_for_size(Value() * kMegaByte, fText, sizeof(fText));
}


VolumeMenuItem::VolumeMenuItem(BVolume volume, BMessage* message)
	:
	BMenuItem("", message),
	fVolume(volume)
{
	GenerateLabel();
}


void
VolumeMenuItem::MessageReceived(BMessage* message)
{
	if (message->what == B_NODE_MONITOR) {
		int32 code;
		if (message->FindInt32("opcode", &code) == B_OK)
			if (code == B_ENTRY_MOVED)
				GenerateLabel();
	}
}


void
VolumeMenuItem::GenerateLabel()
{
	char name[B_FILE_NAME_LENGTH + 1];
	fVolume.GetName(name);

	BDirectory dir;
	if (fVolume.GetRootDirectory(&dir) == B_OK) {
		BEntry entry;
		if (dir.GetEntry(&entry) == B_OK) {
			BPath path;
			if (entry.GetPath(&path) == B_OK) {
				BString label;
				label << name << " (" << path.Path() << ")";
				SetLabel(label);
				return;
			}
		}
	}

	SetLabel(name);
}


SettingsWindow::SettingsWindow()
	:
	BWindow(BRect(0, 0, 269, 172), B_TRANSLATE_SYSTEM_NAME("VirtualMemory"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
		| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)

{
	bootDev = dev_for_path("/boot");
	BAlignment align(B_ALIGN_LEFT, B_ALIGN_MIDDLE);

	if (fSettings.ReadWindowSettings() == B_OK)
		MoveTo(fSettings.WindowPosition());
	else
		CenterOnScreen();

	status_t result = fSettings.ReadSwapSettings();
	if (result == kErrorSettingsNotFound)
		fSettings.DefaultSwapSettings(false);
	else if (result == kErrorSettingsInvalid) {
		int32 choice = (new BAlert(B_TRANSLATE_SYSTEM_NAME("VirtualMemory"),
			B_TRANSLATE("The settings specified in the settings file "
			"are invalid. You can load the defaults or quit."),
			B_TRANSLATE("Load defaults"), B_TRANSLATE("Quit")))->Go();
		if (choice == 1) {
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		fSettings.DefaultSwapSettings(false);
	} else if (result == kErrorVolumeNotFound) {
		int32 choice = (new BAlert(B_TRANSLATE_SYSTEM_NAME("VirtualMemory"),
			B_TRANSLATE("The volume specified in the settings file "
			"could not be found. You can use the boot volume or quit."),
			B_TRANSLATE("Use boot volume"), B_TRANSLATE("Quit")))->Go();
		if (choice == 1) {
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		fSettings.SetSwapVolume(bootDev, false);
	}

	fSwapEnabledCheckBox = new BCheckBox("enable swap",
		B_TRANSLATE("Enable virtual memory"),
		new BMessage(kMsgSwapEnabledUpdate));
	fSwapEnabledCheckBox->SetExplicitAlignment(align);

	char sizeStr[16];
	system_info info;
	get_system_info(&info);
	BString string = B_TRANSLATE("Physical memory: ");
	string << string_for_size(info.max_pages * B_PAGE_SIZE,	sizeStr,
		sizeof(sizeStr));
	BStringView* memoryView = new BStringView("physical memory",
		string.String());
	memoryView->SetExplicitAlignment(align);

	system_memory_info memInfo = {};
	__get_system_info_etc(B_MEMORY_INFO, &memInfo, sizeof(memInfo));
	string = B_TRANSLATE("Current swap file size: ");
	string << string_for_size(memInfo.max_swap_space, sizeStr,
		sizeof(sizeStr));
	BStringView* swapFileView = new BStringView("current swap size",
		string.String());
	swapFileView->SetExplicitAlignment(align);

	BPopUpMenu* menu = new BPopUpMenu("volume menu");
	fVolumeMenuField = new BMenuField("volume menu field",
		B_TRANSLATE("Use volume:"), menu);
	fVolumeMenuField->SetExplicitAlignment(align);

	BVolumeRoster roster;
	BVolume vol;
	while (roster.GetNextVolume(&vol) == B_OK) {
		if (!vol.IsPersistent() || vol.IsReadOnly() || vol.IsRemovable()
			|| vol.IsShared())
			continue;
		_AddVolumeMenuItem(vol.Device());
	}

	watch_node(NULL, B_WATCH_MOUNT, this, this);

	fSizeSlider = new SizeSlider("size slider",
		B_TRANSLATE("Requested swap file size:"),
		new BMessage(kMsgSliderUpdate),	0, 0, B_WILL_DRAW | B_FRAME_EVENTS);
	fSizeSlider->SetViewColor(255, 0, 255);
	fSizeSlider->SetExplicitAlignment(align);

	fWarningStringView = new BStringView("warning",
		B_TRANSLATE("Changes will take effect upon reboot."));

	BBox* box = new BBox("box");
	box->SetLabel(fSwapEnabledCheckBox);

	box->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(memoryView)
		.Add(swapFileView)
		.Add(fVolumeMenuField)
		.Add(fSizeSlider)
		.Add(fWarningStringView)
		.SetInsets(10)
		.View());

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(box)
		.AddGroup(B_HORIZONTAL, 10)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
		.End()
		.SetInsets(10);

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
		BAlert* alert = new BAlert(B_TRANSLATE_SYSTEM_NAME("VirtualMemory"),
			B_TRANSLATE("The swap volume specified in the settings file is ",
			"invalid.\n You can keep the current setting or switch to the "
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


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;
			dev_t device;
			if (opcode == B_DEVICE_MOUNTED
				&& message->FindInt32("new device", &device) == B_OK) {
				BVolume vol(device);
				if (!vol.IsPersistent() || vol.IsReadOnly()
					|| vol.IsRemovable() || vol.IsShared()) {
					break;
				}
				_AddVolumeMenuItem(device);
			} else if (opcode == B_DEVICE_UNMOUNTED
				&& message->FindInt32("device", &device) == B_OK) {
				_RemoveVolumeMenuItem(device);
			}
			_Update();
			break;
		}
		case kMsgRevert:
			fSettings.RevertSwapSettings();
			_Update();
			break;
		case kMsgDefaults:
			fSettings.DefaultSwapSettings();
			_Update();
			break;
		case kMsgSliderUpdate:
			fSettings.SetSwapSize((off_t)fSizeSlider->Value() * kMegaByte);
			_Update();
			break;
		case kMsgVolumeSelected:
			fSettings.SetSwapVolume(((VolumeMenuItem*)fVolumeMenuField
				->Menu()->FindMarked())->Volume().Device());
			_Update();
			break;
		case kMsgSwapEnabledUpdate:
		{
			if (fSwapEnabledCheckBox->Value() == 0) {
				// print out warning, give the user the
				// time to think about it :)
				// ToDo: maybe we want to remove this possibility in the GUI
				// as Be did, but I thought a proper warning could be helpful
				// (for those that want to change that anyway)
				BAlert* alert = new BAlert(
					B_TRANSLATE_SYSTEM_NAME("VirtualMemory"), B_TRANSLATE(
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

			fSettings.SetSwapEnabled(fSwapEnabledCheckBox->Value());
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
	fSettings.WriteWindowSettings();
	fSettings.WriteSwapSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


status_t
SettingsWindow::_AddVolumeMenuItem(dev_t device)
{
	if (_FindVolumeMenuItem(device) != NULL)
		return B_ERROR;

	VolumeMenuItem* item = new VolumeMenuItem(device,
		new BMessage(kMsgVolumeSelected));

	fs_info info;
	if (fs_stat_dev(device, &info) == 0) {
		node_ref node;
		node.device = info.dev;
		node.node = info.root;
		AddHandler(item);
		watch_node(&node, B_WATCH_NAME, item);
	}

	fVolumeMenuField->Menu()->AddItem(item);
	return B_OK;
}


status_t
SettingsWindow::_RemoveVolumeMenuItem(dev_t device)
{
	VolumeMenuItem* item = _FindVolumeMenuItem(device);
	if (item != NULL) {
		fVolumeMenuField->Menu()->RemoveItem(item);
		delete item;
		return B_OK;
	} else
		return B_ERROR;
}


VolumeMenuItem*
SettingsWindow::_FindVolumeMenuItem(dev_t device)
{
	VolumeMenuItem* item = NULL;
	int32 count = fVolumeMenuField->Menu()->CountItems();
	for (int i = 0; i < count; i++) {
		item = (VolumeMenuItem*)fVolumeMenuField->Menu()->ItemAt(i);
		if (item->Volume().Device() == device)
			return item;
	}

	return NULL;
}


void
SettingsWindow::_Update()
{
	fSwapEnabledCheckBox->SetValue(fSettings.SwapEnabled());

	VolumeMenuItem* item = _FindVolumeMenuItem(fSettings.SwapVolume());
	if (item != NULL) {
		fSizeSlider->SetEnabled(true);
		item->SetMarked(true);
		BEntry swapFile;
		if (bootDev == item->Volume().Device())
			swapFile.SetTo("/var/swap");
		else {
			BDirectory root;
			item->Volume().GetRootDirectory(&root);
			swapFile.SetTo(&root, "swap");
		}

		off_t swapFileSize = 0;
		swapFile.GetSize(&swapFileSize);

		char sizeStr[16];

		off_t freeSpace = item->Volume().FreeBytes() + swapFileSize;
		off_t safeSpace = freeSpace - (off_t)(0.15 * freeSpace);
		(safeSpace >>= 20) <<= 20;
		off_t minSize = B_PAGE_SIZE + kMegaByte;
		(minSize >>= 20) <<= 20;
		BString minLabel, maxLabel;
		minLabel << string_for_size(minSize, sizeStr, sizeof(sizeStr));
		maxLabel << string_for_size(safeSpace, sizeStr, sizeof(sizeStr));

		fSizeSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
		fSizeSlider->SetLimits(minSize / kMegaByte, safeSpace / kMegaByte);
		fSizeSlider->SetValue(fSettings.SwapSize() / kMegaByte);
	} else
		fSizeSlider->SetEnabled(false);

	bool revertable = fSettings.IsRevertable();
	if (revertable)
		fWarningStringView->Show();
	else
		fWarningStringView->Hide();
	fRevertButton->SetEnabled(revertable);
	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
}
