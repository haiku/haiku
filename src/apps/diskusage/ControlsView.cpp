/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "ControlsView.h"

#include <Bitmap.h>
#include <Box.h>
#include <TabView.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <SupportDefs.h>
#include <View.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include <LayoutBuilder.h>

#include "DiskUsage.h"
#include "VolumeView.h"


class VolumeTab: public BTab {
public:
								VolumeTab(BVolume* volume);
	virtual						~VolumeTab();

			BVolume*			Volume() const
									{ return fVolume; }
			float				IconWidth() const;

	virtual	void				DrawLabel(BView* owner, BRect frame);

private:
			BBitmap*			fIcon;
			BVolume*			fVolume;
};


VolumeTab::VolumeTab(BVolume* volume)
	:
	BTab(),
	fIcon(new BBitmap(BRect(0, 0, 15, 15), B_RGBA32)),
	fVolume(volume)
{
	if (fVolume->GetIcon(fIcon, B_MINI_ICON) < B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


float
VolumeTab::IconWidth() const
{
	if (fIcon != NULL)
		// add a small margin
		return fIcon->Bounds().Width() + kSmallHMargin;
	else
		return 0.0f;
}


void
VolumeTab::DrawLabel(BView* owner, BRect frame)
{
	owner->SetDrawingMode(B_OP_OVER);
	if (fIcon != NULL) {
		owner->MovePenTo(frame.left + kSmallHMargin,
			(frame.top + frame.bottom - fIcon->Bounds().Height()) / 2.0);
		owner->DrawBitmap(fIcon);
	}
	owner->SetDrawingMode(B_OP_COPY);
	font_height fh;
	owner->GetFontHeight(&fh);

	BString label = Label();

	owner->TruncateString(&label, B_TRUNCATE_END,
		frame.Width() - IconWidth() - kSmallHMargin);

	owner->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	owner->DrawString(label,
		BPoint(frame.left + IconWidth() + kSmallHMargin,
			(frame.top + frame.bottom - fh.ascent - fh.descent) / 2.0
				+ fh.ascent));
}


VolumeTab::~VolumeTab()
{
	delete fIcon;
	delete fVolume;
}


// #pragma mark -


class ControlsView::VolumeTabView: public BTabView {
public:
								VolumeTabView();
	virtual						~VolumeTabView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual BRect				TabFrame(int32 index) const;

			BVolume*			FindDeviceFor(dev_t device,
									bool invoke = false);

private:
			void				_AddVolume(dev_t device);
			void				_RemoveVolume(dev_t device);

			BVolumeRoster*		fVolumeRoster;
};


ControlsView::VolumeTabView::VolumeTabView()
	:
	BTabView("volume_tabs", B_WIDTH_FROM_LABEL)
{
}


ControlsView::VolumeTabView::~VolumeTabView()
{
	fVolumeRoster->StopWatching();
	delete fVolumeRoster;
}


BRect
ControlsView::VolumeTabView::TabFrame(int32 index) const
{
	float height = BTabView::TabFrame(index).Height();
	float x = 0.0f;
	float width = 0.0f;
	float minStringWidth = StringWidth("Haiku");

	int	countTabs = CountTabs();

	// calculate the total width if no truncation is made at all
	float averageWidth = Frame().Width() / countTabs;
	
	// margins are the deltas with the average widths	
	float* margins = new float[countTabs];
	for (int32 i = 0; i < countTabs; i++) {
		float tabLabelWidth = StringWidth(TabAt(i)->Label());
		if (tabLabelWidth < minStringWidth)
			tabLabelWidth = minStringWidth;
		float tabWidth = tabLabelWidth + 3.0f * kSmallHMargin
				+ ((VolumeTab*)TabAt(i))->IconWidth();
	
		margins[i] = tabWidth - averageWidth;
		width += tabWidth;
	}

	// determine how much we should shave to show all tabs (truncating)
	float toShave = width - Frame().Width();

	if (toShave > 0.0f) {
		// the thinest a tab can be to hold the minimum string
		float minimumMargin = minStringWidth + 3.0f * kSmallHMargin
			- averageWidth;

		float averageToShave;
		float oldToShave;
		/* 
			we might have to do multiple passes because of the minimum
			tab width we are imposing.
			we could also fail to totally fit all tabs.
			TODO: allow paging.
		*/

		do {
			averageToShave = toShave / countTabs;
			oldToShave = toShave;
			for (int32 i = 0; i < countTabs; i++) {
				float iconWidth = ((VolumeTab*)TabAt(i))->IconWidth();
				float newMargin = margins[i] - averageToShave;

				toShave -= averageToShave;
				if (newMargin < minimumMargin + iconWidth) {
					toShave += minimumMargin - newMargin + iconWidth;
					newMargin = minimumMargin + iconWidth;
				}
				margins[i] = newMargin;
			}
		} while (toShave > 0 && oldToShave != toShave);
	}

	for (int i = 0; i < index; i++)
		x += averageWidth + margins[i];

	float margin = margins[index];
	delete[] margins;

	return BRect(x, 0.0f, x + averageWidth + margin, height);
}


void
ControlsView::VolumeTabView::AttachedToWindow()
{
	// Populate the menu with the persistent volumes.
	fVolumeRoster = new BVolumeRoster();

	BVolume tempVolume;
	while (fVolumeRoster->GetNextVolume(&tempVolume) == B_OK) {
		if (tempVolume.IsPersistent()) {
			BVolume* volume = new BVolume(tempVolume);
			VolumeTab *item = new VolumeTab(volume);
			char name[B_PATH_NAME_LENGTH];
			volume->GetName(name);			
			AddTab(new VolumeView(name, volume), item);
		}
	}

	// Begin watching mount and unmount events.
	fVolumeRoster->StartWatching(BMessenger(this));
}


void
ControlsView::VolumeTabView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			switch (message->FindInt32("opcode")) {
				case B_DEVICE_MOUNTED:
					_AddVolume(message->FindInt32("new device"));
					break;

				case B_DEVICE_UNMOUNTED:
					_RemoveVolume(message->FindInt32("device"));
					break;
			}
			break;

		case kBtnRescan:
			ViewForTab(Selection())->MessageReceived(message);
			break;

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			entry_ref ref;

			for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
				BEntry entry(&ref, true);
				BPath path;
				entry.GetPath(&path);
				dev_t device = dev_for_path(path.Path());

				for (int j = 0; VolumeTab* item = (VolumeTab*)TabAt(j); j++) {
					if (item->Volume()->Device() == device) {
						Select(j);
						((VolumeView*)(item->View()))->SetPath(path);
						break;
					}
				}
			}
			break;
		}

		default:
			BTabView::MessageReceived(message);
			break;
	}
}


BVolume*
ControlsView::VolumeTabView::FindDeviceFor(dev_t device, bool invoke)
{
	BVolume* volume = NULL;

	// Iterate through items looking for a BVolume representing this device.
	for (int i = 0; VolumeTab* item = (VolumeTab*)TabAt(i); i++) {
		if (item->Volume()->Device() == device) {
			volume = item->Volume();
			if (invoke)
				Select(i);
			break;
		}
	}

	return volume;
}


void
ControlsView::VolumeTabView::_AddVolume(dev_t device)
{
	// Make sure the volume is not already in the menu.
	for (int i = 0; VolumeTab* item = (VolumeTab*)TabAt(i); i++) {
		if (item->Volume()->Device() == device)
			return;
	}

	BVolume* volume = new BVolume(device);

	VolumeTab* item = new VolumeTab(volume);
	char name[B_PATH_NAME_LENGTH];
	volume->GetName(name);

	AddTab(new VolumeView(name, volume), item);
	Invalidate();
}


void
ControlsView::VolumeTabView::_RemoveVolume(dev_t device)
{
	for (int i = 0; VolumeTab* item = (VolumeTab*)TabAt(i); i++) {
		if (item->Volume()->Device() == device) {
			if (i == 0)
				Select(1);
			else
				Select(i - 1);
			RemoveTab(i);
			delete item;
			return;
		}
	}
}


// #pragma mark -


ControlsView::ControlsView()
	:
	BView(NULL, B_WILL_DRAW)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	fVolumeTabView = new VolumeTabView();
	AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.Add(fVolumeTabView)
	);
}


void
ControlsView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			fVolumeTabView->MessageReceived(msg);
			break;

		case kBtnRescan:
			fVolumeTabView->MessageReceived(msg);
			break;

		default:
			BView::MessageReceived(msg);
	}
}


ControlsView::~ControlsView()
{
}


void
ControlsView::ShowInfo(const FileInfo* info)
{
	((VolumeView*)fVolumeTabView->ViewForTab(
		fVolumeTabView->Selection()))->ShowInfo(info);
}


void
ControlsView::SetRescanEnabled(bool enabled)
{
	((VolumeView*)fVolumeTabView->ViewForTab(
		fVolumeTabView->Selection()))->SetRescanEnabled(enabled);
}


BVolume*
ControlsView::FindDeviceFor(dev_t device, bool invoke)
{
	return fVolumeTabView->FindDeviceFor(device, invoke);
}
