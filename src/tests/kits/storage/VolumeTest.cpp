// VolumeTest.cpp

#include <stdio.h>
#include <string>
#include <unistd.h>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <TypeConstants.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestApp.h>
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "VolumeTest.h"
#include "../app/bmessenger/Helpers.h"

// test dirs/files/types
static const char *testDir			= "/tmp/testDir";
static const char *testFile1		= "/tmp/testDir/file1";
static const char *testMountPoint	= "/tmp/testDir/mount_point";

// icon_equal
static
bool
icon_equal(const BBitmap *icon1, const BBitmap *icon2)
{
	return (icon1->Bounds() == icon2->Bounds()
			&& icon1->BitsLength() == icon2->BitsLength()
			&& memcmp(icon1->Bits(), icon2->Bits(), icon1->BitsLength()) == 0);
}

// Suite
CppUnit::Test*
VolumeTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<VolumeTest> TC;
		
	suite->addTest( new TC("BVolume::Init Test1",
						   &VolumeTest::InitTest1) );
	suite->addTest( new TC("BVolume::Init Test2",
						   &VolumeTest::InitTest2) );
	suite->addTest( new TC("BVolume::Assignment Test",
						   &VolumeTest::AssignmentTest) );
	suite->addTest( new TC("BVolume::Comparisson Test",
						   &VolumeTest::ComparissonTest) );
	suite->addTest( new TC("BVolume::SetName Test",
						   &VolumeTest::SetNameTest) );
	suite->addTest( new TC("BVolume::BadValues Test",
						   &VolumeTest::BadValuesTest) );
	suite->addTest( new TC("BVolumeRoster::Iteration Test",
						   &VolumeTest::IterationTest) );
	suite->addTest( new TC("BVolumeRoster::Watching Test",
						   &VolumeTest::WatchingTest) );

	return suite;
}		

// setUp
void
VolumeTest::setUp()
{
	BasicTest::setUp();
	// create test dir and files
	execCommand(
		string("mkdir ") + testDir
	);
	// create and mount image
	createVolume(testFile1, testMountPoint, 1);
	// create app
	fApplication = new BTestApp("application/x-vnd.obos.volume-test");
	if (fApplication->Init() != B_OK) {
		fprintf(stderr, "Failed to initialize application.\n");
		delete fApplication;
		fApplication = NULL;
	}
}
	
// tearDown
void
VolumeTest::tearDown()
{
	// delete the application
	if (fApplication) {
		fApplication->Terminate();
		delete fApplication;
		fApplication = NULL;
	}
	// unmount and delete image
	deleteVolume(testFile1, testMountPoint);
	// delete the test dir
	execCommand(string("rm -rf ") + testDir);

	BasicTest::tearDown();
}

// CheckVolume
static
void
CheckVolume(BVolume &volume, dev_t device, status_t error)
{
	CHK(volume.InitCheck() == error);
	if (error == B_OK) {
		fs_info info;
		CHK(fs_stat_dev(device, &info) == 0);
		// device
		CHK(volume.Device() == device);
		// root dir
		BDirectory rootDir;
		CHK(volume.GetRootDirectory(&rootDir) == B_OK);
		node_ref rootNode;
		rootNode.device = device;
		rootNode.node = info.root;
		BDirectory actualRootDir(&rootNode);
		CHK(rootDir == actualRootDir);
		// capacity, free bytes
		CHK(volume.Capacity() == info.total_blocks * info.block_size);
		CHK(volume.FreeBytes() == info.free_blocks * info.block_size);
		// name
		char name[B_FILE_NAME_LENGTH];
		CHK(volume.GetName(name) == B_OK);
		CHK(BString(name) == info.volume_name);
		// icons
		// mini
		BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
		BBitmap miniIcon2(BRect(0, 0, 15, 15), B_CMAP8);
		status_t iconError = get_device_icon(info.device_name,
											 miniIcon2.Bits(), B_MINI_ICON);
		CHK(volume.GetIcon(&miniIcon, B_MINI_ICON) == iconError);
		if (iconError == B_OK)
			CHK(icon_equal(&miniIcon, &miniIcon2));
		// large
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
		BBitmap largeIcon2(BRect(0, 0, 31, 31), B_CMAP8);
		iconError = get_device_icon(info.device_name, largeIcon2.Bits(),
									B_LARGE_ICON);
		CHK(volume.GetIcon(&largeIcon, B_LARGE_ICON) == iconError);
		if (iconError == B_OK)
			CHK(icon_equal(&largeIcon, &largeIcon2));
		// flags
		CHK(volume.IsRemovable() == bool(info.flags & B_FS_IS_REMOVABLE));
		CHK(volume.IsReadOnly() == bool(info.flags & B_FS_IS_READONLY));
		CHK(volume.IsPersistent() == bool(info.flags & B_FS_IS_PERSISTENT));
		CHK(volume.IsShared() == bool(info.flags & B_FS_IS_SHARED));
		CHK(volume.KnowsMime() == bool(info.flags & B_FS_HAS_MIME));
		CHK(volume.KnowsAttr() == bool(info.flags & B_FS_HAS_ATTR));
		CHK(volume.KnowsQuery() == bool(info.flags & B_FS_HAS_QUERY));
	} else {
		CHK(volume.Device() == -1);
		// root dir
		BDirectory rootDir;
		CHK(volume.GetRootDirectory(&rootDir) == B_BAD_VALUE);
		// capacity, free bytes
		CHK(volume.Capacity() == B_BAD_VALUE);
		CHK(volume.FreeBytes() == B_BAD_VALUE);
		// name
		char name[B_FILE_NAME_LENGTH];
		CHK(volume.GetName(name) == B_BAD_VALUE);
		// icons
		// mini
		BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(volume.GetIcon(&miniIcon, B_MINI_ICON) == B_BAD_VALUE);
		// large
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(volume.GetIcon(&largeIcon, B_LARGE_ICON) == B_BAD_VALUE);
		// flags
		CHK(volume.IsRemovable() == false);
		CHK(volume.IsReadOnly() == false);
		CHK(volume.IsPersistent() == false);
		CHK(volume.IsShared() == false);
		CHK(volume.KnowsMime() == false);
		CHK(volume.KnowsAttr() == false);
		CHK(volume.KnowsQuery() == false);
	}
}

// AssertNotBootVolume
static
void
AssertNotBootVolume(const BVolume &volume)
{
	CHK(volume.InitCheck() == B_OK);
	dev_t bootDevice = dev_for_path("/boot");
	CHK(bootDevice >= 0);
	CHK(volume.Device() != bootDevice);
}

// InitTest1
void
VolumeTest::InitTest1()
{
	// 1. BVolume(void)
	{
		BVolume volume;
		CheckVolume(volume, -1, B_NO_INIT);
	}
	// 2. BVolume(dev_t dev)
	// volumes for testing
	const char *volumes[] = {
		"/boot",
		"/",
		"/dev",
		"/pipe",
		"/unknown",
		testMountPoint
	};
	int32 volumeCount = sizeof(volumes) / sizeof(const char*);
	for (int32 i = 0; i < volumeCount; i++) {
		NextSubTest();
		const char *volumeRootDir = volumes[i];
		dev_t device = dev_for_path(volumeRootDir);
		BVolume volume(device);
		CheckVolume(volume, device, (device >= 0 ? B_OK : B_BAD_VALUE));
	}
	// invalid device ID
	NextSubTest();
	{
		BVolume volume(-2);
		CHK(volume.InitCheck() == B_BAD_VALUE);
	}
	// invalid device ID
	NextSubTest();
	{
		dev_t device = 213;
		fs_info info;
		while (fs_stat_dev(device, &info) == 0)
			device++;
		BVolume volume(device);
		CHK(volume.InitCheck() == B_ENTRY_NOT_FOUND);
	}
}

// InitTest2
void
VolumeTest::InitTest2()
{
	// volumes for testing
	const char *volumes[] = {
		"/boot",
		"/",
		"/dev",
		"/pipe",
		"/unknown",
		testMountPoint
	};
	int32 volumeCount = sizeof(volumes) / sizeof(const char*);
	BVolume volume1;
	for (int32 i = 0; i < volumeCount; i++) {
		NextSubTest();
		const char *volumeRootDir = volumes[i];
		dev_t device = dev_for_path(volumeRootDir);
		status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
		// reinit already initialized volume
		CHK(volume1.SetTo(device) == initError);
		CheckVolume(volume1, device, initError);
		// init fresh volume
		BVolume volume2;
		CHK(volume2.SetTo(device) == initError);
		CheckVolume(volume2, device, initError);
		// uninit volume
		volume2.Unset();
		CheckVolume(volume2, device, B_NO_INIT);
	}
	// invalid device ID
	NextSubTest();
	{
		BVolume volume;
		CHK(volume.SetTo(-2) == B_BAD_VALUE);
		CHK(volume.InitCheck() == B_BAD_VALUE);
	}
	// invalid device ID
	NextSubTest();
	{
		dev_t device = 213;
		fs_info info;
		while (fs_stat_dev(device, &info) == 0)
			device++;
		BVolume volume;
		CHK(volume.SetTo(device) == B_ENTRY_NOT_FOUND);
		CHK(volume.InitCheck() == B_ENTRY_NOT_FOUND);
	}
}

// AssignmentTest
void
VolumeTest::AssignmentTest()
{
	// volumes for testing
	const char *volumes[] = {
		"/boot",
		"/",
		"/dev",
		"/pipe",
		"/unknown",
		testMountPoint
	};
	int32 volumeCount = sizeof(volumes) / sizeof(const char*);
	BVolume volume1;
	for (int32 i = 0; i < volumeCount; i++) {
		NextSubTest();
		const char *volumeRootDir = volumes[i];
		dev_t device = dev_for_path(volumeRootDir);
		status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
		BVolume volume3(device);
		CheckVolume(volume3, device, initError);
		// assignment operation
		CHK(&(volume1 = volume3) == &volume1);
		CheckVolume(volume1, device, initError);
		// copy constructor
		BVolume volume2(volume3);
		CheckVolume(volume2, device, initError);
	}
}

// ComparissonTest
void
VolumeTest::ComparissonTest()
{
	// volumes for testing
	const char *volumes[] = {
		"/boot",
		"/",
		"/dev",
		"/pipe",
		"/unknown",
		testMountPoint
	};
	int32 volumeCount = sizeof(volumes) / sizeof(const char*);
	for (int32 i = 0; i < volumeCount; i++) {
		NextSubTest();
		const char *volumeRootDir = volumes[i];
		dev_t device = dev_for_path(volumeRootDir);
		status_t initError = (device >= 0 ? B_OK : B_BAD_VALUE);
		BVolume volume(device);
		CheckVolume(volume, device, initError);
		for (int32 k = 0; k < volumeCount; k++) {
			const char *volumeRootDir2 = volumes[k];
			dev_t device2 = dev_for_path(volumeRootDir2);
			status_t initError2 = (device2 >= 0 ? B_OK : B_BAD_VALUE);
			BVolume volume2(device2);
			CheckVolume(volume2, device2, initError2);
			bool equal = (i == k
						  || initError == initError2 && initError2 != B_OK);
			CHK((volume == volume2) == equal);
			CHK((volume != volume2) == !equal);
		}
	}
}

// SetNameTest
void
VolumeTest::SetNameTest()
{
	// status_t SetName(const char* name);
	dev_t device = dev_for_path(testMountPoint);
	BVolume volume(device);
	CheckVolume(volume, device, B_OK);
	AssertNotBootVolume(volume);
	// set a new name
	NextSubTest();
	const char *newName = "a new name";
	CHK(volume.SetName(newName) == B_OK);
	char name[B_FILE_NAME_LENGTH];
	CHK(volume.GetName(name) == B_OK);
	CHK(string(newName) == name);
	CheckVolume(volume, device, B_OK);
	// set another name
	NextSubTest();
	const char *newName2 = "another name";
	CHK(volume.SetName(newName2) == B_OK);
	CHK(volume.GetName(name) == B_OK);
	CHK(string(newName2) == name);
	CheckVolume(volume, device, B_OK);
	// GetName() with NULL buffer
// R5: crashes when passing a NULL pointer
#ifndef TEST_R5
	NextSubTest();
	CHK(volume.GetName(NULL) == B_BAD_VALUE);
#endif
	// bad name
// R5: crashes when passing a NULL pointer
#ifndef TEST_R5
	NextSubTest();
	CHK(volume.SetName(NULL) == B_BAD_VALUE);
#endif
	// uninitialized volume
	NextSubTest();
	volume.Unset();
	CHK(volume.SetName(newName) == B_BAD_VALUE);
}

// BadValuesTest
void
VolumeTest::BadValuesTest()
{
	BVolume volume(dev_for_path("/boot"));
	CHK(volume.InitCheck() == B_OK);
	// NULL arguments
// R5: crashes, when passing a NULL BDirectory.
#ifndef TEST_R5
	NextSubTest();
	CHK(volume.GetRootDirectory(NULL) == B_BAD_VALUE);
#endif
	NextSubTest();
	CHK(volume.GetIcon(NULL, B_MINI_ICON) == B_BAD_VALUE);
	CHK(volume.GetIcon(NULL, B_LARGE_ICON) == B_BAD_VALUE);
	// incompatible icon formats
// R5: returns B_OK
#ifndef TEST_R5
	NextSubTest();
	// mini
	BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
	CHK(volume.GetIcon(&largeIcon, B_MINI_ICON) == B_BAD_VALUE);
	// large
	BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
	CHK(volume.GetIcon(&miniIcon, B_LARGE_ICON) == B_BAD_VALUE);
#endif
}


// BVolumeRoster tests

//  GetAllDevices
static
void
GetAllDevices(set<dev_t> &devices)
{
//printf("GetAllDevices()\n");
	int32 cookie = 0;
	dev_t device;
	while ((device = next_dev(&cookie)) >= 0)
{
//printf("  device: %ld\n", device);
//BVolume dVolume(device);
//char name[B_FILE_NAME_LENGTH];
//dVolume.GetName(name);
//BDirectory rootDir;
//dVolume.GetRootDirectory(&rootDir);
//BEntry rootEntry;
//rootDir.GetEntry(&rootEntry);
//BPath rootPath;
//rootEntry.GetPath(&rootPath);
//printf("  name: `%s', root: `%s'\n", name, rootPath.Path());
		devices.insert(device);
}
//printf("GetAllDevices() done\n");
}

// IterationTest
void
VolumeTest::IterationTest()
{
	// status_t GetBootVolume(BVolume *volume)
	NextSubTest();
	BVolumeRoster roster;
	BVolume volume;
	CHK(roster.GetBootVolume(&volume) == B_OK);
	dev_t device = dev_for_path("/boot");
	CHK(device >= 0);
	CheckVolume(volume, device, B_OK);

	// status_t GetNextVolume(BVolume *volume)
	// void Rewind()
	set<dev_t> allDevices;
	GetAllDevices(allDevices);
	int32 allDevicesCount = allDevices.size();
	for (int32 i = 0; i <= allDevicesCount; i++) {
		NextSubTest();
		// iterate through the first i devices
		set<dev_t> devices(allDevices);
		volume.Unset();
		int32 checkCount = i;
		while (--checkCount >= 0 && roster.GetNextVolume(&volume) == B_OK) {
			device = volume.Device();
			CHK(device >= 0);
			CheckVolume(volume, device, B_OK);
			CHK(devices.find(device) != devices.end());
			devices.erase(device);
		}
		// rewind and iterate through all devices
		devices = allDevices;
		roster.Rewind();
		volume.Unset();
		status_t error;
		while ((error = roster.GetNextVolume(&volume)) == B_OK) {
			device = volume.Device();
			CHK(device >= 0);
			CheckVolume(volume, device, B_OK);
			CHK(devices.find(device) != devices.end());
			devices.erase(device);
		}
		CHK(error == B_BAD_VALUE);
		CHK(devices.empty());
		roster.Rewind();
	}

	// bad argument
// R5: crashes when passing a NULL BVolume
#ifndef TEST_R5
	NextSubTest();
	CHK(roster.GetNextVolume(NULL) == B_BAD_VALUE);
#endif
}

// CheckWatchingMessage
static
void
CheckWatchingMessage(bool mounted, dev_t expectedDevice, BTestHandler &handler,
					 node_ref nodeRef = node_ref())
{
	snooze(100000);
	// get the message
	BMessageQueue &queue = handler.Queue();
	BMessage *_message = queue.NextMessage();
	CHK(_message);
	BMessage message(*_message);
	delete _message;
	// check the message
	if (mounted) {
		// volume mounted
		int32 opcode;
		dev_t device;
		dev_t parentDevice;
		ino_t directory;
		CHK(message.FindInt32("opcode", &opcode) == B_OK);
		CHK(message.FindInt32("new device", &device) == B_OK);
		CHK(message.FindInt32("device", &parentDevice) == B_OK);
		CHK(message.FindInt64("directory", &directory) == B_OK);
		CHK(opcode == B_DEVICE_MOUNTED);
		CHK(device == expectedDevice);
		CHK(parentDevice == nodeRef.device);
		CHK(directory == nodeRef.node);
	} else {
		// volume unmounted
		int32 opcode;
		dev_t device;
		CHK(message.FindInt32("opcode", &opcode) == B_OK);
		CHK(message.FindInt32("device", &device) == B_OK);
		CHK(opcode == B_DEVICE_UNMOUNTED);
		CHK(device == expectedDevice);
	}
}

// WatchingTest
void
VolumeTest::WatchingTest()
{
	// status_t StartWatching(BMessenger msngr=be_app_messenger);
	// void StopWatching(void);
	// BMessenger Messenger(void) const;

	// start watching
	NextSubTest();
	BVolumeRoster roster;
	CHK(!roster.Messenger().IsValid());
	BMessenger target(&fApplication->Handler());
	CHK(roster.StartWatching(target) == B_OK);
	CHK(roster.Messenger() == target);
	dev_t device = dev_for_path(testMountPoint);
	CHK(device >= 0);
	// unmount volume
	NextSubTest();
	deleteVolume(testFile1, testMountPoint, false);
	CHK(roster.Messenger() == target);
	CheckWatchingMessage(false, device, fApplication->Handler());
	// get the node_ref of the mount point
	node_ref nodeRef;
	CHK(BDirectory(testMountPoint).GetNodeRef(&nodeRef) == B_OK);
	// mount volume
	NextSubTest();
	createVolume(testFile1, testMountPoint, 1, false);
	CHK(roster.Messenger() == target);
	device = dev_for_path(testMountPoint);
	CHK(device >= 0);
	CheckWatchingMessage(true, device, fApplication->Handler(), nodeRef);
	// start watching with another target
	BTestHandler *handler2 = fApplication->CreateTestHandler();
	BMessenger target2(handler2);
	CHK(roster.StartWatching(target2) == B_OK);
	CHK(roster.Messenger() == target2);
	// unmount volume
	NextSubTest();
	deleteVolume(testFile1, testMountPoint, false);
	CHK(roster.Messenger() == target2);
	CheckWatchingMessage(false, device, *handler2);
	// mount volume
	NextSubTest();
	createVolume(testFile1, testMountPoint, 1, false);
	CHK(roster.Messenger() == target2);
	device = dev_for_path(testMountPoint);
	CHK(device >= 0);
	CheckWatchingMessage(true, device, *handler2, nodeRef);
	// stop watching
	NextSubTest();
	roster.StopWatching();
	CHK(!roster.Messenger().IsValid());
	// unmount, mount volume
	NextSubTest();
	deleteVolume(testFile1, testMountPoint, false);
	createVolume(testFile1, testMountPoint, 1, false);
	snooze(100000);
	CHK(fApplication->Handler().Queue().IsEmpty());
	CHK(handler2->Queue().IsEmpty());

	// try start watching with a bad messenger
	NextSubTest();
	CHK(roster.StartWatching(BMessenger()) == B_ERROR);
}

