/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsWindow.h"
#include "Settings.h"

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
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


static const uint32 kMsgDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgSwapEnabledUpdate = 'swen';


class SizeSlider : public BSlider {
	public:
		SizeSlider(BRect rect, const char* name, const char* label,
			BMessage* message, int32 min, int32 max, uint32 resizingMode);
		virtual ~SizeSlider();

		virtual char* UpdateText() const;

	private:
		mutable BString	fText;
};


static const int64 kMegaByte = 1048576;


const char *
byte_string(int64 size)
{
	double value = 1. * size;
	static char string[64];

	if (value < 1024)
		snprintf(string, sizeof(string), "%Ld B", size);
	else {
		char *units[] = {"K", "M", "G", NULL};
		int32 i = -1;

		do {
			value /= 1024.0;
			i++;
		} while (value >= 1024 && units[i + 1]);

		off_t rounded = off_t(value * 100LL);
		sprintf(string, "%g %sB", rounded / 100.0, units[i]);
	}

	return string;
}


//	#pragma mark -


SizeSlider::SizeSlider(BRect rect, const char* name, const char* label,
	BMessage* message, int32 min, int32 max, uint32 resizingMode)
	: BSlider(rect, name, label, message, min, max, B_BLOCK_THUMB, resizingMode)
{
	rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &color);
}


SizeSlider::~SizeSlider()
{
}


char *
SizeSlider::UpdateText() const
{
	fText = byte_string(Value() * kMegaByte);

	return const_cast<char*>(fText.String());
}


//	#pragma mark -


SettingsWindow::SettingsWindow()
	: BWindow(BRect(0, 0, 269, 172), "VirtualMemory", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect rect = Bounds();
	BView* view = new BView(rect, "background", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);

	float lineHeight = ceil(fontHeight.ascent + fontHeight.descent + fontHeight.ascent);

	fSwapEnabledCheckBox = new BCheckBox(rect, "enable swap", "Enable Virtual Memory",
		new BMessage(kMsgSwapEnabledUpdate));
	fSwapEnabledCheckBox->SetValue(fSettings.SwapEnabled());
	fSwapEnabledCheckBox->ResizeToPreferred();

	rect.InsetBy(10, 10);
	BBox* box = new BBox(rect, "box", B_FOLLOW_LEFT_RIGHT);
	box->SetLabel(fSwapEnabledCheckBox);
	view->AddChild(box);

	system_info info;
	get_system_info(&info);

	rect.right -= 20;
	rect.top = lineHeight;
	BString string = "Physical Memory: ";
	string << byte_string((off_t)info.max_pages * B_PAGE_SIZE);
	BStringView* stringView = new BStringView(rect, "physical memory", string.String(),
		B_FOLLOW_ALL);
	stringView->ResizeToPreferred();
	box->AddChild(stringView);

	rect.OffsetBy(0, lineHeight);
	string = "Current Swap File Size: ";
	string << byte_string(fSettings.SwapSize());
	stringView = new BStringView(rect, "current swap size", string.String(),
		B_FOLLOW_ALL);
	stringView->ResizeToPreferred();
	box->AddChild(stringView);

	BPopUpMenu* menu = new BPopUpMenu("volumes");

	// collect volumes
	// ToDo: listen to volume changes!
	// ToDo: accept dropped volumes

	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (!volume.IsPersistent() || volume.GetName(name) != B_OK || !name[0])
			continue;

		BMenuItem* item = new BMenuItem(name, NULL);
		menu->AddItem(item);

		if (volume.Device() == fSettings.SwapVolume().Device())
			item->SetMarked(true);
	}

	rect.OffsetBy(0, lineHeight);
	BMenuField* field = new BMenuField(rect, "devices", "Use Volume:", menu);
	field->SetDivider(field->StringWidth(field->Label()) + 8);
	field->ResizeToPreferred();
	field->SetEnabled(false);
	box->AddChild(field);

	off_t minSize, maxSize;
	_GetSwapFileLimits(minSize, maxSize);

	rect.OffsetBy(0, lineHeight + 8);
	fSizeSlider = new SizeSlider(rect, "size slider", "Requested Swap File Size:",
		new BMessage(kMsgSliderUpdate), minSize / kMegaByte, maxSize / kMegaByte,
		B_FOLLOW_LEFT_RIGHT);
	fSizeSlider->SetLimitLabels("999 MB", "999 MB");
	fSizeSlider->ResizeToPreferred();
	fSizeSlider->SetViewColor(255, 0, 255);
	box->AddChild(fSizeSlider);

	rect.OffsetBy(0, fSizeSlider->Frame().Height() + 5);
	rect.bottom = rect.top + stringView->Frame().Height();
	fWarningStringView = new BStringView(rect, "", "", B_FOLLOW_ALL);
	fWarningStringView->SetAlignment(B_ALIGN_CENTER);
	box->AddChild(fWarningStringView);

	box->ResizeTo(box->Frame().Width(), fWarningStringView->Frame().bottom + 10);

	// Add "Defaults" and "Revert" buttons

	rect.top = box->Frame().bottom + 10;
	BButton* button = new BButton(rect, "defaults", "Defaults", new BMessage(kMsgDefaults));
	button->ResizeToPreferred();
	view->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 10, 0);
	fRevertButton = new BButton(rect, "revert", "Revert", new BMessage(kMsgRevert));
	button->ResizeToPreferred();
	view->AddChild(fRevertButton);

	view->ResizeTo(view->Frame().Width(), button->Frame().bottom + 10);
	ResizeTo(view->Bounds().Width(), view->Bounds().Height());
	AddChild(view);
		// add view after resizing the window, so that the view's resizing
		// mode is not used (we already layed out the views for the new height)

	// if the strings don't fit on screen, we enlarge the window now - the
	// views are already added to the window and will resize automatically
	if (Bounds().Width() < view->StringWidth(fSizeSlider->Label()) * 2)
		ResizeTo(view->StringWidth(fSizeSlider->Label()) * 2, Bounds().Height());

	_Update();

	BScreen screen;
	BRect screenFrame = screen.Frame();

	if (!screenFrame.Contains(fSettings.WindowPosition())) {
		// move on screen, centered
		MoveTo((screenFrame.Width() - Bounds().Width()) / 2,
			(screenFrame.Height() - Bounds().Height()) / 2);
	} else
		MoveTo(fSettings.WindowPosition());
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::_Update()
{
	if ((fSwapEnabledCheckBox->Value() != 0) != fSettings.SwapEnabled())
		fSwapEnabledCheckBox->SetValue(fSettings.SwapEnabled());

	off_t minSize, maxSize;
	if (_GetSwapFileLimits(minSize, maxSize) == B_OK) {
		BString minLabel, maxLabel;
		minLabel << byte_string(minSize);
		maxLabel << byte_string(maxSize);
		if (minLabel != fSizeSlider->MinLimitLabel()
			|| maxLabel != fSizeSlider->MaxLimitLabel()) {
			fSizeSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
#ifdef __HAIKU__
			fSizeSlider->SetLimits(minSize / kMegaByte, maxSize / kMegaByte);
#endif
		}

		if (fSizeSlider->Value() != fSettings.SwapSize() / kMegaByte)
			fSizeSlider->SetValue(fSettings.SwapSize() / kMegaByte);

		fSizeSlider->SetEnabled(true);
	} else {
		fSizeSlider->SetValue(minSize);
		fSizeSlider->SetEnabled(false);
	}

	// ToDo: set volume

	bool changed = fSettings.SwapChanged();
	if (fRevertButton->IsEnabled() != changed) {
		fRevertButton->SetEnabled(changed);
		if (changed)
			fWarningStringView->SetText("Changes will take effect on restart!");
		else
			fWarningStringView->SetText("");
	}
}


status_t
SettingsWindow::_GetSwapFileLimits(off_t& minSize, off_t& maxSize)
{
	// minimum size is the installed memory
	system_info info;
	get_system_info(&info);
	minSize = (off_t)info.max_pages * B_PAGE_SIZE;

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
		if (swap.GetSize(&size) == B_OK)
			freeSpace += size;
	}

	maxSize = freeSpace - safetyFreeSpace;

	if (maxSize < minSize) {
		maxSize = minSize;
		return B_ERROR;
	}

	return B_OK;
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
			fSettings.SetSwapDefaults();
			_Update();
			break;
		case kMsgSliderUpdate:
			fSettings.SetSwapSize((off_t)fSizeSlider->Value() * kMegaByte);
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
				//	as Be did, but I thought a proper warning could be helpful
				//	(for those that want to change that anyway)
				int32 choice = (new BAlert("VirtualMemory",
					"Disabling virtual memory will have unwanted effects on "
					"system stability once the memory is used up.\n"
					"Virtual memory does not affect system performance "
					"until this point is reached.\n\n"
					"Are you really sure you want to turn it off?",
					"Turn Off", "Keep Enabled", NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
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

