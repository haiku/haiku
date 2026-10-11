/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <set>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <StorageDefs.h>
#include <String.h>
#include <SupportDefs.h>
#include <TypeConstants.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <fs_attr.h>
#include <fs_info.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"
#include "TestApp.h"


// test dirs/files/types
static const char* testMountPoint = "/tmp/volumetest-mountpoint";


static bool
icon_equal(const BBitmap* icon1, const BBitmap* icon2)
{
	return icon1->Bounds() == icon2->Bounds() && icon1->BitsLength() == icon2->BitsLength()
		&& memcmp(icon1->Bits(), icon2->Bits(), icon1->BitsLength()) == 0;
}

static void
CheckVolume(BVolume& volume, dev_t device, status_t error)
{
	CPPUNIT_ASSERT(volume.InitCheck() == error);
	if (error == B_OK) {
		fs_info info;
		CPPUNIT_ASSERT(fs_stat_dev(device, &info) == 0);
		// device
		CPPUNIT_ASSERT(volume.Device() == device);
		// root dir
		BDirectory rootDir;
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetRootDirectory(&rootDir));
		node_ref rootNode;
		rootNode.device = device;
		rootNode.node = info.root;
		BDirectory actualRootDir(&rootNode);
		CPPUNIT_ASSERT(rootDir == actualRootDir);
		// capacity, free bytes
		CPPUNIT_ASSERT(volume.Capacity() == info.total_blocks * info.block_size);
		CPPUNIT_ASSERT(volume.FreeBytes() == info.free_blocks * info.block_size);
		// name
		char name[B_FILE_NAME_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetName(name));
		CPPUNIT_ASSERT(BString(name) == info.volume_name);
		// icons
		// mini
		BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
		BBitmap miniIcon2(BRect(0, 0, 15, 15), B_CMAP8);
		status_t iconError = get_device_icon(info.device_name, miniIcon2.Bits(), B_MINI_ICON);
		CPPUNIT_ASSERT(volume.GetIcon(&miniIcon, B_MINI_ICON) == iconError);
		if (iconError == B_OK)
			CPPUNIT_ASSERT(icon_equal(&miniIcon, &miniIcon2));
		// large
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
		BBitmap largeIcon2(BRect(0, 0, 31, 31), B_CMAP8);
		iconError = get_device_icon(info.device_name, largeIcon2.Bits(), B_LARGE_ICON);
		CPPUNIT_ASSERT(volume.GetIcon(&largeIcon, B_LARGE_ICON) == iconError);
		if (iconError == B_OK)
			CPPUNIT_ASSERT(icon_equal(&largeIcon, &largeIcon2));
		// flags
		CPPUNIT_ASSERT(volume.IsRemovable() == bool(info.flags & B_FS_IS_REMOVABLE));
		CPPUNIT_ASSERT(volume.IsReadOnly() == bool(info.flags & B_FS_IS_READONLY));
		CPPUNIT_ASSERT(volume.IsPersistent() == bool(info.flags & B_FS_IS_PERSISTENT));
		CPPUNIT_ASSERT(volume.IsShared() == bool(info.flags & B_FS_IS_SHARED));
		CPPUNIT_ASSERT(volume.KnowsMime() == bool(info.flags & B_FS_HAS_MIME));
		CPPUNIT_ASSERT(volume.KnowsAttr() == bool(info.flags & B_FS_HAS_ATTR));
		CPPUNIT_ASSERT(volume.KnowsQuery() == bool(info.flags & B_FS_HAS_QUERY));
	} else {
		CPPUNIT_ASSERT(volume.Device() == -1);
		// root dir
		BDirectory rootDir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetRootDirectory(&rootDir));
		// capacity, free bytes
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.Capacity());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.FreeBytes());
		// name
		char name[B_FILE_NAME_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetName(name));
		// icons
		// mini
		BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(&miniIcon, B_MINI_ICON));
		// large
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(&largeIcon, B_LARGE_ICON));
		// flags
		CPPUNIT_ASSERT(volume.IsRemovable() == false);
		CPPUNIT_ASSERT(volume.IsReadOnly() == false);
		CPPUNIT_ASSERT(volume.IsPersistent() == false);
		CPPUNIT_ASSERT(volume.IsShared() == false);
		CPPUNIT_ASSERT(volume.KnowsMime() == false);
		CPPUNIT_ASSERT(volume.KnowsAttr() == false);
		CPPUNIT_ASSERT(volume.KnowsQuery() == false);
	}
}

static void
AssertNotBootVolume(const BVolume& volume)
{
	CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
	dev_t bootDevice = dev_for_path("/boot");
	CPPUNIT_ASSERT(bootDevice >= 0);
	CPPUNIT_ASSERT(volume.Device() != bootDevice);
}

static void
GetAllDevices(std::set<dev_t>& devices)
{
	int32 cookie = 0;
	dev_t device;
	while ((device = next_dev(&cookie)) >= 0) {
		devices.insert(device);
	}
}

static void
CheckWatchingMessage(bool mounted, dev_t expectedDevice, BTestHandler& handler,
	node_ref nodeRef = node_ref())
{
	snooze(100000);
	// get the message
	BMessageQueue& queue = handler.Queue();
	BMessage* _message = queue.NextMessage();
	CPPUNIT_ASSERT(_message);
	BMessage message(*_message);
	delete _message;
	// check the message
	if (mounted) {
		// volume mounted
		int32 opcode;
		dev_t device;
		dev_t parentDevice;
		ino_t directory;
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("opcode", &opcode));
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("new device", &device));
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("device", &parentDevice));
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt64("directory", &directory));
		CPPUNIT_ASSERT_EQUAL(B_DEVICE_MOUNTED, opcode);
		CPPUNIT_ASSERT(device == expectedDevice);
		CPPUNIT_ASSERT(parentDevice == nodeRef.device);
		CPPUNIT_ASSERT(directory == nodeRef.node);
	} else {
		// volume unmounted
		int32 opcode;
		dev_t device;
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("opcode", &opcode));
		CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("device", &device));
		CPPUNIT_ASSERT_EQUAL(B_DEVICE_UNMOUNTED, opcode);
		CPPUNIT_ASSERT(device == expectedDevice);
	}
}

class BTestApp;

class VolumeTest : public BasicTest {
	CPPUNIT_TEST_SUITE(VolumeTest);
	CPPUNIT_TEST(DefaultVolume_Constructor_ReturnsNoInit);
	CPPUNIT_TEST(ExistingDevice_Constructor_ReturnsOk);
	CPPUNIT_TEST(InvalidDevice_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(NonExistingDevice_Constructor_ReturnsEntryNotFound);

	CPPUNIT_TEST(ExistingDevice_SetTo_ReturnsOk);
	CPPUNIT_TEST(InvalidDevice_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(NonExistingDevice_SetTo_ReturnsEntryNotFound);
	CPPUNIT_TEST(InitializedVolume_Unset_ReturnsNoInit);

	CPPUNIT_TEST(InitializedVolume_CopyConstructor_ReturnsEqual);
	CPPUNIT_TEST(InitializedVolume_Assignment_ReturnsEqual);

	CPPUNIT_TEST(InitializedVolumes_Comparison_ReturnsExpected);

	CPPUNIT_TEST(InitializedVolume_SetName_ReturnsOk);
	CPPUNIT_TEST(InitializedVolume_SetName_ReturnsBadValue);
	CPPUNIT_TEST(UninitializedVolume_SetName_ReturnsBadValue);

	CPPUNIT_TEST(InitializedVolume_GetName_ReturnsBadValue);

	CPPUNIT_TEST(InitializedVolume_GetRootDirectory_ReturnsBadValue);

	CPPUNIT_TEST(InitializedVolume_GetIcon_ReturnsBadValue);
	CPPUNIT_TEST(InitializedVolume_GetIcon_ReturnsBadValueForIncompatibleFormat);

	CPPUNIT_TEST(VolumeRoster_GetBootVolume_ReturnsOk);
	CPPUNIT_TEST(VolumeRoster_GetNextVolume_ReturnsBadValue);
	CPPUNIT_TEST(VolumeRoster_GetNextVolumeAndRewind_ReturnsExpected);

	CPPUNIT_TEST(VolumeRoster_MountUnmountWatching_ReturnsExpected);
	CPPUNIT_TEST(VolumeRoster_StartWatching_ReturnsErrorForBadMessenger);
	CPPUNIT_TEST_SUITE_END();

	BTestApp* fApplication;
	std::string fImageFile;

public:
	void tearDown()
	{
		// delete the application
		if (fApplication) {
			fApplication->Terminate();
			delete fApplication;
			fApplication = NULL;
		}
		// unmount and delete image
		DeleteVolume(fImageFile, testMountPoint);

		BasicTest::tearDown();
	}

	void setUp()
	{
		BasicTest::setUp();
		// create and mount image
		fImageFile = CreateVolume(testMountPoint, 2);
		// create app
		fApplication = new BTestApp("application/x-vnd.obos.volume-test");
		if (fApplication->Init() != B_OK) {
			fprintf(stderr, "Failed to initialize application.\n");
			delete fApplication;
			fApplication = NULL;
		}
	}

	void DefaultVolume_Constructor_ReturnsNoInit()
	{
		BVolume volume;
		CheckVolume(volume, -1, B_NO_INIT);
	}

	void ExistingDevice_Constructor_ReturnsOk()
	{
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			BVolume volume(device);
			CheckVolume(volume, device, device >= 0 ? B_OK : B_BAD_VALUE);
		}
	}

	void InvalidDevice_Constructor_ReturnsBadValue()
	{
		BVolume volume(-2);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.InitCheck());
	}

	void NonExistingDevice_Constructor_ReturnsEntryNotFound()
	{
		dev_t device = 213;
		fs_info info;
		while (fs_stat_dev(device, &info) == 0)
			device++;
		BVolume volume(device);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, volume.InitCheck());
	}

	void ExistingDevice_SetTo_ReturnsOk()
	{
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		BVolume volume1;
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
			// reinit already initialized volume
			CPPUNIT_ASSERT(volume1.SetTo(device) == initError);
			CheckVolume(volume1, device, initError);
			// init fresh volume
			BVolume volume2;
			CPPUNIT_ASSERT(volume2.SetTo(device) == initError);
			CheckVolume(volume2, device, initError);
		}
	}

	void InvalidDevice_SetTo_ReturnsBadValue()
	{
		BVolume volume;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.SetTo(-2));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.InitCheck());
	}

	void NonExistingDevice_SetTo_ReturnsEntryNotFound()
	{
		dev_t device = 213;
		fs_info info;
		while (fs_stat_dev(device, &info) == 0)
			device++;
		BVolume volume;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, volume.SetTo(device));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, volume.InitCheck());
	}

	void InitializedVolume_Unset_ReturnsNoInit()
	{
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			if (device < 0)
				continue;
			// init volume
			BVolume volume(device);
			CheckVolume(volume, device, B_OK);
			// unset
			volume.Unset();
			CheckVolume(volume, device, B_NO_INIT);
		}
	}

	void InitializedVolume_CopyConstructor_ReturnsEqual()
	{
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
			BVolume volume3(device);
			CheckVolume(volume3, device, initError);
			// copy constructor
			BVolume volume2(volume3);
			CheckVolume(volume2, device, initError);
		}
	}

	void InitializedVolume_Assignment_ReturnsEqual()
	{
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		BVolume volume1;
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
			BVolume volume3(device);
			CheckVolume(volume3, device, initError);
			// assignment operation
			CPPUNIT_ASSERT(&(volume1 = volume3) == &volume1);
			CheckVolume(volume1, device, initError);
		}
	}

	void InitializedVolumes_Comparison_ReturnsExpected()
	{
		// volumes for testing
		const char* volumes[] = {"/boot", "/", "/dev", "/pipe", "/unknown", testMountPoint};
		int32 volumeCount = sizeof(volumes) / sizeof(const char*);
		for (int32 i = 0; i < volumeCount; i++) {
			const char* volumeRootDir = volumes[i];
			dev_t device = dev_for_path(volumeRootDir);
			status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
			BVolume volume(device);
			CheckVolume(volume, device, initError);
			for (int32 k = 0; k < volumeCount; k++) {
				const char* volumeRootDir2 = volumes[k];
				dev_t device2 = dev_for_path(volumeRootDir2);
				status_t initError2 = (device2 >= 0 ? B_OK : B_BAD_VALUE);
				BVolume volume2(device2);
				CheckVolume(volume2, device2, initError2);
				bool equal = (i == k || (initError == initError2 && initError2 != B_OK));
				CPPUNIT_ASSERT((volume == volume2) == equal);
				CPPUNIT_ASSERT((volume != volume2) == !equal);
			}
		}
	}

	void InitializedVolume_SetName_ReturnsOk()
	{
		dev_t device = dev_for_path(testMountPoint);
		BVolume volume(device);
		CheckVolume(volume, device, B_OK);
		AssertNotBootVolume(volume);

		const char* newName = "a new name";
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.SetName(newName));
		char name[B_FILE_NAME_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetName(name));
		CPPUNIT_ASSERT(BString(newName) == name);
		CheckVolume(volume, device, B_OK);

		const char* newName2 = "another name";
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.SetName(newName2));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetName(name));
		CPPUNIT_ASSERT(BString(newName2) == name);
		CheckVolume(volume, device, B_OK);
	}

	void InitializedVolume_SetName_ReturnsBadValue()
	{
#ifndef TEST_R5
		dev_t device = dev_for_path(testMountPoint);
		BVolume volume(device);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.SetName(NULL));
#endif
	}

	void UninitializedVolume_SetName_ReturnsBadValue()
	{
		BVolume volume;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.SetName("some name"));
	}

	void InitializedVolume_GetName_ReturnsBadValue()
	{
#ifndef TEST_R5
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetName(NULL));
#endif
	}

	void InitializedVolume_GetRootDirectory_ReturnsBadValue()
	{
#ifndef TEST_R5
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetRootDirectory(NULL));
#endif
	}

	void InitializedVolume_GetIcon_ReturnsBadValue()
	{
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(NULL, B_LARGE_ICON));
	}

	void InitializedVolume_GetIcon_ReturnsBadValueForIncompatibleFormat()
	{
#ifndef TEST_R5
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		// mini
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(&largeIcon, B_MINI_ICON));
		// large
		BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.GetIcon(&miniIcon, B_LARGE_ICON));
#endif
	}

	void VolumeRoster_GetBootVolume_ReturnsOk()
	{
		BVolumeRoster roster;
		BVolume volume;
		CPPUNIT_ASSERT_EQUAL(B_OK, roster.GetBootVolume(&volume));
		dev_t device = dev_for_path("/boot");
		CPPUNIT_ASSERT(device >= 0);
		CheckVolume(volume, device, B_OK);
	}

	void VolumeRoster_GetNextVolume_ReturnsBadValue()
	{
#ifndef TEST_R5
		BVolumeRoster roster;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, roster.GetNextVolume(NULL));
#endif
	}

	void VolumeRoster_GetNextVolumeAndRewind_ReturnsExpected()
	{
		BVolumeRoster roster;
		BVolume volume;
		std::set<dev_t> allDevices;
		GetAllDevices(allDevices);
		int32 allDevicesCount = allDevices.size();

		// iterate through the first i devices and rewind, for all i
		for (int32 i = 0; i <= allDevicesCount; i++) {
			// iterate through the first i devices
			std::set<dev_t> devices = allDevices;
			volume.Unset();
			int32 checkCount = i;
			while (--checkCount >= 0 && roster.GetNextVolume(&volume) == B_OK) {
				dev_t device = volume.Device();
				CPPUNIT_ASSERT(device >= 0);
				CheckVolume(volume, device, B_OK);
				CPPUNIT_ASSERT(devices.find(device) != devices.end());
				devices.erase(device);
			}
			// rewind and iterate through all devices
			devices = allDevices;
			roster.Rewind();
			volume.Unset();
			status_t error;
			while ((error = roster.GetNextVolume(&volume)) == B_OK) {
				dev_t device = volume.Device();
				CPPUNIT_ASSERT(device >= 0);
				CheckVolume(volume, device, B_OK);
				CPPUNIT_ASSERT(devices.find(device) != devices.end());
				devices.erase(device);
			}
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, error);
			CPPUNIT_ASSERT(devices.empty());
			roster.Rewind();
		}
	}

	void VolumeRoster_MountUnmountWatching_ReturnsExpected()
	{
		BVolumeRoster roster;
		BMessenger target(&fApplication->Handler());
		CPPUNIT_ASSERT_EQUAL(B_OK, roster.StartWatching(target));
		dev_t device = dev_for_path(testMountPoint);
		CPPUNIT_ASSERT(device >= 0);

		// unmount volume
		DeleteVolume(fImageFile, testMountPoint, false);
		CheckWatchingMessage(false, device, fApplication->Handler());

		// get the node_ref of the mount point
		node_ref nodeRef;
		CPPUNIT_ASSERT_EQUAL(B_OK, BDirectory(testMountPoint).GetNodeRef(&nodeRef));

		// mount volume
		fImageFile = CreateVolume(testMountPoint, 2, false);
		device = dev_for_path(testMountPoint);
		CPPUNIT_ASSERT(device >= 0);
		CheckWatchingMessage(true, device, fApplication->Handler(), nodeRef);

		// start watching with another target
		BTestHandler* handler2 = fApplication->CreateTestHandler();
		BMessenger target2(handler2);
		CPPUNIT_ASSERT_EQUAL(B_OK, roster.StartWatching(target2));
		CPPUNIT_ASSERT(roster.Messenger() == target2);

		// unmount volume
		DeleteVolume(fImageFile, testMountPoint, false);
		CPPUNIT_ASSERT(roster.Messenger() == target2);
		CheckWatchingMessage(false, device, *handler2);

		// mount volume
		fImageFile = CreateVolume(testMountPoint, 2, false);
		CPPUNIT_ASSERT(roster.Messenger() == target2);
		device = dev_for_path(testMountPoint);
		CPPUNIT_ASSERT(device >= 0);
		CheckWatchingMessage(true, device, *handler2, nodeRef);

		// stop watching
		roster.StopWatching();
		CPPUNIT_ASSERT(!roster.Messenger().IsValid());

		// unmount, mount volume: no messages should arrive anymore
		DeleteVolume(fImageFile, testMountPoint, false);
		fImageFile = CreateVolume(testMountPoint, 2, false);
		snooze(100000);
		CPPUNIT_ASSERT(fApplication->Handler().Queue().IsEmpty());
		CPPUNIT_ASSERT(handler2->Queue().IsEmpty());
	}

	void VolumeRoster_StartWatching_ReturnsErrorForBadMessenger()
	{
		BVolumeRoster roster;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, roster.StartWatching(BMessenger()));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VolumeTest, getTestSuiteName());
