/*
 * Copyright 2008-2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 *		Leo Rouleau
 */


#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include <RosterPrivate.h>


#include <iostream>

#include <Drivers.h>
#include <StorageDefs.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "DevicesView.h"
#include "DriverUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DevicesView"


DevicesView::DevicesView(OrderByType OrderBy)
	:
	BView("DevicesView", B_WILL_DRAW | B_FRAME_EVENTS),
	fOrderBy(OrderBy),
	fHasShownDisableAlert(false),
	fRebootNeeded(false)
{
	CreateLayout();
	RescanDevices();
	RebuildDevicesOutline();
}


DevicesView::~DevicesView()
{
	DeleteDevices();
}


void
DevicesView::CreateLayout()
{
	BMenuBar* menuBar = new BMenuBar("menu");
	BMenu* menu = new BMenu(B_TRANSLATE("Devices"));
	BMenuItem* item;
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh devices"),
		new BMessage(kMsgRefresh), 'R'));
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Report compatibility"),
		new BMessage(kMsgReportCompatibility)));
	item->SetEnabled(false);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Generate system information"),
		new BMessage(kMsgGenerateSysInfo)));
	item->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu->SetTargetForItems(this);
	item->SetTarget(be_app);
	menuBar->AddItem(menu);

	fDevicesOutline = new BOutlineListView("devices_list");
	fDevicesOutline->SetTarget(this);
	fDevicesOutline->SetSelectionMessage(new BMessage(kMsgSelectionChanged));

	BScrollView *scrollView = new BScrollView("devicesScrollView",
		fDevicesOutline, B_WILL_DRAW | B_FRAME_EVENTS, true, true);
	// Horizontal scrollbar doesn't behave properly like the vertical
	// scrollbar... If you make the view bigger (exposing a larger percentage
	// of the view), it does not adjust the width of the scroll 'dragger'
	// why? Bug? In scrollview or in outlinelistview?

	BPopUpMenu* orderByPopupMenu = new BPopUpMenu("orderByMenu");
	BMenuItem* byBus = new BMenuItem(B_TRANSLATE("Bus"),
		new BMessage(kMsgOrderBus));
	BMenuItem* byCategory = new BMenuItem(B_TRANSLATE("Category"),
		new BMessage(kMsgOrderCategory));
	BMenuItem* byConnection = new BMenuItem(B_TRANSLATE("Connection"),
		new BMessage(kMsgOrderConnection));
	orderByPopupMenu->AddItem(byBus);
	orderByPopupMenu->AddItem(byCategory);
	orderByPopupMenu->AddItem(byConnection);

	item = orderByPopupMenu->ItemAt((int32)fOrderBy);
	if (item != NULL)
		item->SetMarked(true);
	fOrderByMenu = new BMenuField(B_TRANSLATE("Order by:"), orderByPopupMenu);
	fAttributesView = new PropertyList("attributesView");
	fBlockButton
		= new BButton("blockButton", B_TRANSLATE("Disable driver"), new BMessage(kMsgToggleDriver));
	fBlockButton->SetEnabled(false);
	fRebootNotice = new BStringView("rebootNotice", "");

	fActionMenuBar = new BMenuBar("Action Menu");
	BMenu* rebootMenu = new BMenu(B_TRANSLATE("Reboot needed"));
	rebootMenu->AddItem(new BMenuItem(B_TRANSLATE("Restart computer"), new BMessage(kMsgReboot)));
	rebootMenu->SetTargetForItems(this);
	fActionMenuBar->AddItem(rebootMenu);
	fActionMenuBar->Hide();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(menuBar, 1.0f)
			.Add(fActionMenuBar, 0.0f)
			.End()
		.AddSplit(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGroup(B_VERTICAL)
				.Add(fOrderByMenu, 1)
				.Add(scrollView, 2)
				.End()
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 2.0f)
				.Add(fAttributesView, 2)
				.AddGroup(B_HORIZONTAL)
					.Add(fBlockButton)
					.Add(fRebootNotice)
					.AddGlue()
					.End()
				.End();
}


void
DevicesView::RescanDevices()
{
	// Empty the outline and delete the devices in the list, incl. categories
	fDevicesOutline->MakeEmpty();
	DeleteDevices();
	DeleteCategoryMap();

	// Fill the devices list
	status_t error;
	device_node_cookie rootCookie;
	if ((error = init_dm_wrapper()) < 0) {
		std::cerr << "Error initializing device manager: " << strerror(error)
			<< std::endl;
		return;
	}

	get_root(&rootCookie);
	AddDeviceAndChildren(&rootCookie, NULL);

	uninit_dm_wrapper();

	CreateCategoryMap();
}


void
DevicesView::DeleteDevices()
{
	while (fDevices.size() > 0) {
		delete fDevices.back();
		fDevices.pop_back();
	}

	CategoryMapIterator iter;
	for (iter = fCategoryMap.begin(); iter != fCategoryMap.end(); iter++)
		delete iter->second;
	fCategoryMap.clear();
}


void
DevicesView::CreateCategoryMap()
{
	CategoryMapIterator iter;
	for (unsigned int i = 0; i < fDevices.size(); i++) {
		Category category = fDevices[i]->GetCategory();
		if (category < 0 || category >= kCategoryStringLength) {
			std::cerr << "CreateCategoryMap: device " << fDevices[i]->GetName()
				<< " returned an unknown category index (" << category << "). "
				<< "Skipping device." << std::endl;
			continue;
		}

		const char* categoryName = kCategoryString[category];

		iter = fCategoryMap.find(category);
		if (iter == fCategoryMap.end()) {
			// This category has not yet been added, add it.
			fCategoryMap[category] = new Device(NULL, BUS_NONE, CAT_NONE, categoryName);
		}
	}
}


void
DevicesView::DeleteCategoryMap()
{
	CategoryMapIterator iter;
	for (iter = fCategoryMap.begin(); iter != fCategoryMap.end(); iter++) {
		delete iter->second;
	}
	fCategoryMap.clear();
}


int
DevicesView::SortItemsCompare(const BListItem *item1, const BListItem *item2)
{
	const BStringItem* stringItem1 = dynamic_cast<const BStringItem*>(item1);
	const BStringItem* stringItem2 = dynamic_cast<const BStringItem*>(item2);
	if (!(stringItem1 && stringItem2)) {
		// is this check necessary?
		std::cerr << "Could not cast BListItem to BStringItem, file a bug\n";
		return 0;
	}
	return Compare(stringItem1->Text(), stringItem2->Text());
}


void
DevicesView::RebuildDevicesOutline()
{
	// Rearranges existing Devices into the proper hierarchy
	fDevicesOutline->MakeEmpty();

	if (fOrderBy == ORDER_BY_BUS) {
		// add all bus controllers to the outline
		for (unsigned int i = 0; i < fDevices.size(); i++)
			if (fDevices[i]->GetCategory() == CAT_BUS)
				fDevicesOutline->AddItem(fDevices[i]);

		// attach devices to their bus
		for (unsigned int i = 0; i < fDevices.size(); i++) {
			if (fDevices[i]->GetCategory() != CAT_BUS) {
				Device* busParent = fDevices[i]->GetPhysicalParent();

				while (busParent != NULL && busParent->GetCategory() != CAT_BUS) {
					busParent = busParent->GetPhysicalParent();
				}

				if (busParent != NULL)
					fDevicesOutline->AddUnder(fDevices[i], busParent);
				else
					fDevicesOutline->AddItem(fDevices[i]);
			}
		}
		fDevicesOutline->SortItemsUnder(NULL, true, SortItemsCompare);
	} else if (fOrderBy == ORDER_BY_CATEGORY) {
		// Add all categories to the outline
		CategoryMapIterator iter;
		for (iter = fCategoryMap.begin(); iter != fCategoryMap.end(); iter++) {
			fDevicesOutline->AddItem(iter->second);
		}

		// Add all devices under the categories
		for (unsigned int i = 0; i < fDevices.size(); i++) {
			Category category = fDevices[i]->GetCategory();

			iter = fCategoryMap.find(category);
			if (iter == fCategoryMap.end()) {
				std::cerr
					<< "Tried to add device without category, file a bug\n";
				continue;
			} else {
				fDevicesOutline->AddUnder(fDevices[i], iter->second);
			}
		}
		fDevicesOutline->SortItemsUnder(NULL, true, SortItemsCompare);
	} else if (fOrderBy == ORDER_BY_CONNECTION) {
		for (unsigned int i = 0; i < fDevices.size(); i++) {
			if (fDevices[i]->GetPhysicalParent() == NULL) {
				// process each parent device and its children
				fDevicesOutline->AddItem(fDevices[i]);
				AddChildrenToOutlineByConnection(fDevices[i]);
			}
		}
	}
}


void
DevicesView::AddChildrenToOutlineByConnection(Device* parent)
{
	for (unsigned int i = 0; i < fDevices.size(); i++) {
		if (fDevices[i]->GetPhysicalParent() == parent) {
			fDevicesOutline->AddUnder(fDevices[i], parent);
			AddChildrenToOutlineByConnection(fDevices[i]);
		}
	}
}


void
DevicesView::AddDeviceAndChildren(device_node_cookie *node, Device* parent)
{
	Attributes attributes;
	Device* newDevice = NULL;

	// Copy all its attributes,
	// necessary because we can only request them once from the device manager
	char data[256];
	struct device_attr_info attr;
	attr.cookie = 0;
	attr.node_cookie = *node;
	attr.value.raw.data = data;
	attr.value.raw.length = sizeof(data);

	while (dm_get_next_attr(&attr) == B_OK) {
		BString attrString;
		switch (attr.type) {
			case B_STRING_TYPE:
				attrString << attr.value.string;
				break;
			case B_UINT8_TYPE:
				attrString << attr.value.ui8;
				break;
			case B_UINT16_TYPE:
				attrString << attr.value.ui16;
				break;
			case B_UINT32_TYPE:
				attrString << attr.value.ui32;
				break;
			case B_UINT64_TYPE:
				attrString << attr.value.ui64;
				break;
			default:
				attrString << "Raw data";
		}
		attributes.push_back(Attribute(attr.name, attrString));
	}

	// Determine what type of device it is and create it
	for (unsigned int i = 0; i < attributes.size(); i++) {
		// Devices Root
		if (attributes[i].fName == B_DEVICE_PRETTY_NAME
			&& attributes[i].fValue == "Devices Root") {
			newDevice = new Device(parent, BUS_NONE,
				CAT_COMPUTER, B_TRANSLATE("Computer"));
			break;
		}

		// ACPI Controller
		if (attributes[i].fName == B_DEVICE_PRETTY_NAME
			&& attributes[i].fValue == "ACPI") {
			newDevice = new Device(parent, BUS_ACPI,
				CAT_BUS, B_TRANSLATE("ACPI bus"));
			break;
		}

		// PCI bus
		if (attributes[i].fName == B_DEVICE_PRETTY_NAME
			&& attributes[i].fValue == "PCI") {
			newDevice = new Device(parent, BUS_PCI,
				CAT_BUS, B_TRANSLATE("PCI bus"));
			break;
		}

		// ISA bus
		if (attributes[i].fName == B_DEVICE_BUS
			&& attributes[i].fValue == "isa") {
			newDevice = new Device(parent, BUS_ISA,
				CAT_BUS, B_TRANSLATE("ISA bus"));
			break;
		}

		// USB bus
		if (attributes[i].fName == B_DEVICE_PRETTY_NAME
			&& attributes[i].fValue == "USB") {
			newDevice = new Device(parent, BUS_USB,
				CAT_BUS, B_TRANSLATE("USB bus"));
			break;
		}

		// PCI device
		if (attributes[i].fName == B_DEVICE_BUS
			&& attributes[i].fValue == "pci") {
			newDevice = new DevicePCI(parent);
			break;
		}

		// ACPI device
		if (attributes[i].fName == B_DEVICE_BUS
			&& attributes[i].fValue == "acpi") {
			newDevice = new DeviceACPI(parent);
			break;
		}

		// USB device
		if (attributes[i].fName == B_DEVICE_BUS
			&& attributes[i].fValue == "usb") {
			newDevice = new DeviceUSB(parent);
			break;
		}

		// ATA / SCSI / IDE controller
		if (attributes[i].fName == "controller_name") {
			newDevice = new Device(parent, BUS_PCI,
				CAT_MASS, attributes[i].fValue);
			break;
		}

		// SCSI device node
		if (attributes[i].fName == B_DEVICE_BUS
			&& attributes[i].fValue == "scsi") {
			newDevice = new DeviceSCSI(parent);
			break;
		}

		// Last resort, lets look for a pretty name
		if (attributes[i].fName == B_DEVICE_PRETTY_NAME) {
			newDevice = new Device(parent, BUS_NONE,
				CAT_NONE, attributes[i].fValue);
			break;
		}
	}

	// A completely unknown device
	if (newDevice == NULL) {
		newDevice = new Device(parent, BUS_NONE,
			CAT_NONE, B_TRANSLATE("Unknown device"));
	}

	struct device_attr_info driverAttrInfo;
	driverAttrInfo.node_cookie = *node;
	driverAttrInfo.cookie = 0;
	dm_get_driver_path(&driverAttrInfo);

	bool hasPublishedPath = false;

	// Add its attributes to the device, initialize it and add to the list.
	for (unsigned int i = 0; i < attributes.size(); i++) {
		if (attributes[i].fName == B_DEVICE_PUBLISHED_PATH) {
			newDevice->SetAttribute(B_TRANSLATE_CONTEXT("Device paths", "Device"),
				attributes[i].fValue);
			hasPublishedPath = true;
			continue;
		}

		newDevice->SetAttribute(attributes[i].fName, attributes[i].fValue);
	}

	if (driverAttrInfo.value.string[0] != '\0')
		newDevice->SetAttribute(B_TRANSLATE_CONTEXT("Driver used", "Device"),
			driverAttrInfo.value.string);
	else
		newDevice->SetAttribute(B_TRANSLATE_CONTEXT("Driver used", "Device"),
			B_TRANSLATE_CONTEXT("unknown", "Device"));
	if (!hasPublishedPath)
		newDevice->SetAttribute(B_TRANSLATE_CONTEXT("Device paths", "Device"),
			B_TRANSLATE_CONTEXT("none", "Device"));

	newDevice->InitFromAttributes();
	fDevices.push_back(newDevice);

	// Process children
	status_t err;
	device_node_cookie child = *node;

	if (get_child(&child) != B_OK)
		return;

	do {
		AddDeviceAndChildren(&child, newDevice);
	} while ((err = get_next_child(&child)) == B_OK);
}


void
DevicesView::_ShowInfoAlert(const BString& message)
{
	BAlert* alert = new BAlert("infoAlert", message, B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}


void
DevicesView::_ShowDisableDriverAlert(const BPath& settingsPath, const BString& relativePath)
{
	if (fHasShownDisableAlert)
		return;

	fHasShownDisableAlert = true;

	BString alertText;
	alertText << B_TRANSLATE("The driver has been disabled. A reboot is required "
							 "for the change to take effect.\n\n"
							 "After rebooting, this device may no longer appear "
							 "in the device list as the system will not be able to identify "
							 "it without its driver.\n\n"
							 "To re-enable the driver, find the device in the "
							 "Devices application and click \"Enable driver\". If the "
							 "device is no longer visible, remove the corresponding "
							 "blocked entry from the packages settings file:\n")
			  << settingsPath.Path() << "\n\n"
			  << B_TRANSLATE("Blocked entry:\n")
			  << relativePath;

	BAlert* alert = new BAlert(B_TRANSLATE("Driver disabled"), alertText, B_TRANSLATE("OK"), NULL,
		NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}


void
DevicesView::_ToggleDriverState(bool disable)
{
	int32 selected = fDevicesOutline->CurrentSelection(0);
	if (selected < 0)
		return;

	Device* device = (Device*)fDevicesOutline->ItemAt(selected);
	BString driver = device->GetDriverUsed();

	if (disable && DriverUtils::IsCriticalDriver(device))
		return;

	BString packageName;
	if (!DriverUtils::IsPackagedDriver(driver, &packageName))
		return;

	BPath settingsPath;
	BString relativePath;
	status_t status = DriverUtils::GetDriverPackageSettings(driver, settingsPath, relativePath);
	if (status != B_OK) {
		if (disable) {
			BString errorMsg = B_TRANSLATE("The driver path is not in a package volume: ");
			errorMsg << driver;
			_ShowInfoAlert(errorMsg);
		}
		return;
	}

	if (DriverUtils::UpdatePackageBlockedEntry(settingsPath.Path(), packageName.String(),
			relativePath.String(), disable)
		!= B_OK) {
		_ShowInfoAlert(B_TRANSLATE("Failed to write to the settings file."));
		return;
	}

	_UpdateBlockButton(device);
	fRebootNeeded = true;
	fActionMenuBar->Show();

	if (disable)
		_ShowDisableDriverAlert(settingsPath, relativePath);
}


void
DevicesView::_UpdateBlockButton(Device* device)
{
	fBlockButton->SetTarget(this);

	if (device == NULL) {
		fBlockButton->SetEnabled(false);
		fBlockButton->SetLabel(B_TRANSLATE("Disable driver"));
		fRebootNotice->SetText("");
		return;
	}

	BString driver = device->GetDriverUsed();
	bool hasDriver = !driver.IsEmpty() && driver != B_TRANSLATE_CONTEXT("unknown", "Device")
		&& driver != B_TRANSLATE_CONTEXT("none", "Device");

	if (!hasDriver) {
		fBlockButton->SetEnabled(false);
		fBlockButton->SetLabel(B_TRANSLATE("Disable driver"));
		fRebootNotice->SetText("");
	} else if (DriverUtils::IsCriticalDriver(device)) {
		fBlockButton->SetEnabled(false);
		fBlockButton->SetLabel(B_TRANSLATE("Disable driver"));
		fRebootNotice->SetHighUIColor(B_PANEL_TEXT_COLOR);
		fRebootNotice->SetText(B_TRANSLATE("Disabling critical system drivers is not allowed."));
	} else if (!DriverUtils::IsPackagedDriver(driver)) {
		fBlockButton->SetEnabled(false);
		fBlockButton->SetLabel(B_TRANSLATE("Disable driver"));
		fRebootNotice->SetHighUIColor(B_PANEL_TEXT_COLOR);
		fRebootNotice->SetText(B_TRANSLATE("Disabling non-packaged drivers is not supported."));
	} else {
		fBlockButton->SetEnabled(true);
		fRebootNotice->SetText("");
		if (DriverUtils::IsDriverEnabled(driver))
			fBlockButton->SetLabel(B_TRANSLATE("Disable driver"));
		else
			fBlockButton->SetLabel(B_TRANSLATE("Enable driver"));
	}
}


void
DevicesView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgReboot:
		{
			BRoster roster;
			BRoster::Private rosterPrivate(roster);
			status_t error = rosterPrivate.ShutDown(true, false, false);
			if (error != B_OK) {
				BString errorMsg;
				errorMsg << B_TRANSLATE("ShutDown failed with error: ") << strerror(error);
				BAlert* errorAlert = new BAlert(B_TRANSLATE("Error"), errorMsg.String(),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				errorAlert->Go();
			}
			break;
		}
		case kMsgSelectionChanged:
		{
			int32 selected = fDevicesOutline->CurrentSelection(0);
			Device* device = (selected >= 0) ? (Device*)fDevicesOutline->ItemAt(selected) : NULL;
			if (device != NULL) {
				fAttributesView->AddAttributes(device->GetAllAttributes());
				fAttributesView->Invalidate();
			}
			_UpdateBlockButton(device);
			break;
		}

		case kMsgOrderBus:
		{
			fOrderBy = ORDER_BY_BUS;
			RescanDevices();
			RebuildDevicesOutline();
			break;
		}

		case kMsgOrderCategory:
		{
			fOrderBy = ORDER_BY_CATEGORY;
			RescanDevices();
			RebuildDevicesOutline();
			break;
		}

		case kMsgOrderConnection:
		{
			fOrderBy = ORDER_BY_CONNECTION;
			RescanDevices();
			RebuildDevicesOutline();
			break;
		}

		case kMsgRefresh:
		{
			fAttributesView->Clear();
			RescanDevices();
			RebuildDevicesOutline();
			break;
		}

		case kMsgReportCompatibility:
		{
			// To be implemented...
			break;
		}

		case kMsgGenerateSysInfo:
		{
			// To be implemented...
			break;
		}

		case kMsgToggleDriver:
		{
			int32 selected = fDevicesOutline->CurrentSelection(0);
			if (selected >= 0) {
				Device* device = (Device*)fDevicesOutline->ItemAt(selected);
				BString driver = device->GetDriverUsed();
				_ToggleDriverState(DriverUtils::IsDriverEnabled(driver));
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}
