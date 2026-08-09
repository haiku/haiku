/*
 * Copyright 2026, Leo Rouleau, leorouleau5070@gmail.com
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <OutlineListView.h>
#include <StringItem.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "Device.h"
#include "DevicesView.h"

#include <TestSuiteAddon.h>


class DeviceTreeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DeviceTreeTest);
	CPPUNIT_TEST(TestOrderByCategory);
	CPPUNIT_TEST(TestOrderByBus);
	CPPUNIT_TEST(TestOrderByConnection);
	CPPUNIT_TEST(TestSortingAlphabetical);
	CPPUNIT_TEST_SUITE_END();

	BApplication* fApp;

public:
	void setUp()
	{
		if (be_app == NULL)
			fApp = new BApplication("application/x-vnd.Haiku-DevicesTreeTest");
		else
			fApp = NULL;
	}

	void tearDown()
	{
		if (fApp != NULL) {
			delete fApp;
			fApp = NULL;
			be_app = NULL;
		}
	}

	void TestOrderByCategory()
	{
		BOutlineListView outline("test");
		Devices devices;
		CategoryMap categoryMap;

		Device* netDevice1 = new Device(NULL, BUS_PCI, CAT_NETWORK, "Ethernet Controller");
		Device* netDevice2 = new Device(NULL, BUS_PCI, CAT_NETWORK, "Wireless Adapter");
		Device* displayDevice = new Device(NULL, BUS_PCI, CAT_DISPLAY, "Graphics Card");

		devices.push_back(netDevice1);
		devices.push_back(netDevice2);
		devices.push_back(displayDevice);

		DevicesView::CreateCategoryMap(devices, categoryMap);
		DevicesView::RebuildDevicesOutline(&outline, devices, categoryMap, ORDER_BY_CATEGORY);

		// Top-level items in the outline must only be the 2 Category headers (Display and Network)
		CPPUNIT_ASSERT_EQUAL(2, (int)outline.CountItemsUnder(NULL, true));

		// Under the Display category, there should be 1 device
		Device* displayCatItem = categoryMap[CAT_DISPLAY];
		CPPUNIT_ASSERT(displayCatItem != NULL);
		CPPUNIT_ASSERT_EQUAL(1, (int)outline.CountItemsUnder(displayCatItem, true));
		CPPUNIT_ASSERT_EQUAL((BListItem*)displayDevice, outline.ItemUnderAt(displayCatItem, true, 0));

		// Under the network category, there should be 2 devices
		Device* netCatItem = categoryMap[CAT_NETWORK];
		CPPUNIT_ASSERT(netCatItem != NULL);
		CPPUNIT_ASSERT_EQUAL(2, (int)outline.CountItemsUnder(netCatItem, true));

		// Ethernet Controller should come before Wireless Adapter because of alphabetical sorting
		CPPUNIT_ASSERT_EQUAL((BListItem*)netDevice1, outline.ItemUnderAt(netCatItem, true, 0));
		CPPUNIT_ASSERT_EQUAL((BListItem*)netDevice2, outline.ItemUnderAt(netCatItem, true, 1));

		for (unsigned int i = 0; i < devices.size(); i++) delete devices[i];
		DevicesView::DeleteCategoryMap(categoryMap);
	}

	void TestOrderByBus()
	{
		BOutlineListView outline("test");
		Devices devices;
		CategoryMap categoryMap;

		Device* root = new Device(NULL, BUS_NONE, CAT_COMPUTER, "Computer");
		Device* pciBus = new Device(root, BUS_PCI, CAT_BUS, "PCI bus");
		Device* netCard = new Device(pciBus, BUS_PCI, CAT_NETWORK, "Ethernet Controller");
		Device* orphanDev = new Device(NULL, BUS_NONE, CAT_GENERIC, "Orphan Device");

		devices.push_back(root);
		devices.push_back(pciBus);
		devices.push_back(netCard);
		devices.push_back(orphanDev);

		DevicesView::RebuildDevicesOutline(&outline, devices, categoryMap, ORDER_BY_BUS);

		// pciBus must be at the root
		CPPUNIT_ASSERT_EQUAL((BListItem*)NULL, outline.Superitem(pciBus));

		// netCard must be placed under pciBus
		CPPUNIT_ASSERT_EQUAL((BListItem*)pciBus, outline.Superitem(netCard));

		// orphanDev remains at root level
		CPPUNIT_ASSERT_EQUAL((BListItem*)NULL, outline.Superitem(orphanDev));

		for (unsigned int i = 0; i < devices.size(); i++) delete devices[i];
	}

	void TestOrderByConnection()
	{
		BOutlineListView outline("test");
		Devices devices;
		CategoryMap categoryMap;

		Device* root = new Device(NULL, BUS_NONE, CAT_COMPUTER, "Computer");
		Device* pciBus = new Device(root, BUS_PCI, CAT_BUS, "PCI bus");
		Device* storage = new Device(pciBus, BUS_PCI, CAT_MASS, "NVMe Storage");
		Device* usbBus = new Device(root, BUS_USB, CAT_BUS, "USB bus");
		Device* mouse = new Device(usbBus, BUS_USB, CAT_INPUT, "USB Mouse");

		devices.push_back(root);
		devices.push_back(pciBus);
		devices.push_back(storage);
		devices.push_back(usbBus);
		devices.push_back(mouse);

		DevicesView::RebuildDevicesOutline(&outline, devices, categoryMap, ORDER_BY_CONNECTION);

		// Only root should be at top level
		CPPUNIT_ASSERT_EQUAL(1, (int)outline.CountItemsUnder(NULL, true));
		CPPUNIT_ASSERT_EQUAL((BListItem*)root, outline.ItemUnderAt(NULL, true, 0));

		// There should be two children under root
		CPPUNIT_ASSERT_EQUAL(2, (int)outline.CountItemsUnder(root, true));
		CPPUNIT_ASSERT_EQUAL((BListItem*)root, outline.Superitem(pciBus));
		CPPUNIT_ASSERT_EQUAL((BListItem*)root, outline.Superitem(usbBus));

		// There should be storage under PCI bus
		CPPUNIT_ASSERT_EQUAL(1, (int)outline.CountItemsUnder(pciBus, true));
		CPPUNIT_ASSERT_EQUAL((BListItem*)pciBus, outline.Superitem(storage));

		// There should be mouse under usbBus
		CPPUNIT_ASSERT_EQUAL(1, (int)outline.CountItemsUnder(usbBus, true));
		CPPUNIT_ASSERT_EQUAL((BListItem*)usbBus, outline.Superitem(mouse));

		for (unsigned int i = 0; i < devices.size(); i++) delete devices[i];
	}

	void TestSortingAlphabetical()
	{
		Device* devA = new Device(NULL, BUS_PCI, CAT_NETWORK, "A Device");
		Device* devB = new Device(NULL, BUS_PCI, CAT_NETWORK, "B Device");
		Device* devZ = new Device(NULL, BUS_PCI, CAT_NETWORK, "Z Device");

		CPPUNIT_ASSERT(DevicesView::SortItemsCompare(devA, devZ) < 0);
		CPPUNIT_ASSERT(DevicesView::SortItemsCompare(devZ, devA) > 0);

		BOutlineListView outline("test");
		Devices devices;
		CategoryMap categoryMap;

		devices.push_back(devZ);
		devices.push_back(devA);
		devices.push_back(devB);

		DevicesView::CreateCategoryMap(devices, categoryMap);
		DevicesView::RebuildDevicesOutline(&outline, devices, categoryMap, ORDER_BY_CATEGORY);

		Device* netCategory = categoryMap[CAT_NETWORK];
		CPPUNIT_ASSERT(netCategory != NULL);
		CPPUNIT_ASSERT_EQUAL(3, (int)outline.CountItemsUnder(netCategory, true));

		// Verify items under the category are alphabetical
		CPPUNIT_ASSERT_EQUAL((BListItem*)devA, outline.ItemUnderAt(netCategory, true, 0));
		CPPUNIT_ASSERT_EQUAL((BListItem*)devB, outline.ItemUnderAt(netCategory, true, 1));
		CPPUNIT_ASSERT_EQUAL((BListItem*)devZ, outline.ItemUnderAt(netCategory, true, 2));

		for (unsigned int i = 0; i < devices.size(); i++) delete devices[i];
		DevicesView::DeleteCategoryMap(categoryMap);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DeviceTreeTest, getTestSuiteName());
