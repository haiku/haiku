/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */


#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <ScrollView.h>
#include <String.h>

#include <iostream>

#include "DevicesView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DevicesView"

DevicesView::DevicesView()
	:
	BView("DevicesView", B_WILL_DRAW | B_FRAME_EVENTS)
{
	CreateLayout();
	RescanDevices();
	RebuildDevicesOutline();
}


void
DevicesView::CreateLayout()
{
	BMenuBar* menuBar = new BMenuBar("menu");
	BMenu* menu = new BMenu(B_TRANSLATE("Devices"));
	BMenuItem* item;
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh devices"),
		new BMessage(kMsgRefresh), 'R'));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Report compatibility"),
		new BMessage(kMsgReportCompatibility)));
	item->SetEnabled(false);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE(
		"Generate system information"), new BMessage(kMsgGenerateSysInfo)));
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
	BMenuItem* byCategory = new BMenuItem(B_TRANSLATE("Category"),
		new BMessage(kMsgOrderCategory));
	BMenuItem* byConnection = new BMenuItem(B_TRANSLATE("Connection"),
		new BMessage(kMsgOrderConnection));
	byCategory->SetMarked(true);
	fOrderBy = byCategory->IsMarked() ? ORDER_BY_CATEGORY :
		ORDER_BY_CONNECTION;
	orderByPopupMenu->AddItem(byCategory);
	orderByPopupMenu->AddItem(byConnection);
	fOrderByMenu = new BMenuField(B_TRANSLATE("Order by:"), orderByPopupMenu);

	fTabView = new BTabView("fTabView", B_WIDTH_FROM_LABEL);

	fBasicTab = new BTab();
	fBasicView = new PropertyListPlain("basicView");
	fTabView->AddTab(fBasicView, fBasicTab);
	fBasicTab->SetLabel(B_TRANSLATE("Basic information"));

	fDeviceTypeTab = new BTab();
	fBusView = new PropertyListPlain("busView");
	fTabView->AddTab(fBusView, fDeviceTypeTab);
	fDeviceTypeTab->SetLabel(B_TRANSLATE("Bus"));

	fDetailedTab = new BTab();
	fAttributesView = new PropertyList("attributesView");
	fTabView->AddTab(fAttributesView, fDetailedTab);
	fDetailedTab->SetLabel(B_TRANSLATE("Detailed"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddSplit(B_HORIZONTAL)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddGroup(B_VERTICAL)
				.Add(fOrderByMenu, 1)
				.Add(scrollView, 2)
				.End()
			.Add(fTabView, 2);
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
}


void
DevicesView::CreateCategoryMap()
{
	CategoryMapIterator iter;
	for (unsigned int i = 0; i < fDevices.size(); i++) {
		Category category = fDevices[i]->GetCategory();
		const char* categoryName = kCategoryString[category];

		iter = fCategoryMap.find(category);
		if (iter == fCategoryMap.end()) {
			// This category has not yet been added, add it.
			fCategoryMap[category] = new Device(NULL, BUS_NONE, CAT_NONE,
				categoryName);
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
DevicesView::SortItemsCompare(const BListItem *item1,
	const BListItem *item2)
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

	if (fOrderBy == ORDER_BY_CONNECTION) {
		for (unsigned int i = 0; i < fDevices.size(); i++) {
			if (fDevices[i]->GetPhysicalParent() == NULL) {
				// process each parent device and its children
				fDevicesOutline->AddItem(fDevices[i]);
				AddChildrenToOutlineByConnection(fDevices[i]);
			}
		}
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
	}
	// TODO: Implement BY_BUS
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

		// ATA / SCSI / IDE controller
		if (attributes[i].fName == "controller_name") {
			newDevice = new Device(parent, BUS_PCI,
				CAT_MASS, attributes[i].fValue);
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

	// Add its attributes to the device, initialize it and add to the list.
	for (unsigned int i = 0; i < attributes.size(); i++) {
		newDevice->SetAttribute(attributes[i].fName, attributes[i].fValue);
	}
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


DevicesView::~DevicesView()
{
	DeleteDevices();
}


void
DevicesView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSelectionChanged:
		{
			int32 selected = fDevicesOutline->CurrentSelection(0);
			if (selected >= 0) {
				Device* device = (Device*)fDevicesOutline->ItemAt(selected);
				fBasicView->AddAttributes(device->GetBasicAttributes());
				fBusView->AddAttributes(device->GetBusAttributes());
				fAttributesView->AddAttributes(device->GetAllAttributes());
				fDeviceTypeTab->SetLabel(device->GetBusTabName());
				// hmm the label doesn't automatically refresh
				fTabView->Invalidate();
			}
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
			RescanDevices();
			RebuildDevicesOutline();
			break;
		}

		case kMsgReportCompatibility:
		{
			break;
		}

		case kMsgGenerateSysInfo:
		{
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}
