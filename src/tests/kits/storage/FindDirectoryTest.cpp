// FindDirectoryTest.cpp

#include <errno.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "FindDirectoryTest.h"

#include <FindDirectory.h>
#include <fs_info.h>
#include <Entry.h>
#include <Path.h>
#include <Volume.h>



const directory_which directories[] = {
	B_DESKTOP_DIRECTORY,
	B_TRASH_DIRECTORY,
	// BeOS directories.  These are mostly accessed read-only.
	B_BEOS_DIRECTORY,
	B_BEOS_SYSTEM_DIRECTORY,
	B_BEOS_ADDONS_DIRECTORY,
	B_BEOS_BOOT_DIRECTORY,
	B_BEOS_FONTS_DIRECTORY,
	B_BEOS_LIB_DIRECTORY,
 	B_BEOS_SERVERS_DIRECTORY,
	B_BEOS_APPS_DIRECTORY,
	B_BEOS_BIN_DIRECTORY,
	B_BEOS_ETC_DIRECTORY,
	B_BEOS_DOCUMENTATION_DIRECTORY,
	B_BEOS_PREFERENCES_DIRECTORY,
	B_BEOS_TRANSLATORS_DIRECTORY,
	B_BEOS_MEDIA_NODES_DIRECTORY,
	B_BEOS_SOUNDS_DIRECTORY,
	// Common directories, shared among all users.
	B_COMMON_DIRECTORY,
	B_COMMON_SYSTEM_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_COMMON_BOOT_DIRECTORY,
	B_COMMON_FONTS_DIRECTORY,
	B_COMMON_LIB_DIRECTORY,
	B_COMMON_SERVERS_DIRECTORY,
	B_COMMON_BIN_DIRECTORY,
	B_COMMON_ETC_DIRECTORY,
	B_COMMON_DOCUMENTATION_DIRECTORY,
	B_COMMON_SETTINGS_DIRECTORY,
	B_COMMON_DEVELOP_DIRECTORY,
	B_COMMON_LOG_DIRECTORY,
	B_COMMON_SPOOL_DIRECTORY,
	B_COMMON_TEMP_DIRECTORY,
	B_COMMON_VAR_DIRECTORY,
	B_COMMON_TRANSLATORS_DIRECTORY,
	B_COMMON_MEDIA_NODES_DIRECTORY,
	B_COMMON_SOUNDS_DIRECTORY,
	// User directories.  These are interpreted in the context
	// of the user making the find_directory call.
	B_USER_DIRECTORY,
	B_USER_CONFIG_DIRECTORY,
	B_USER_ADDONS_DIRECTORY,
	B_USER_BOOT_DIRECTORY,
	B_USER_FONTS_DIRECTORY,
	B_USER_LIB_DIRECTORY,
	B_USER_SETTINGS_DIRECTORY,
	B_USER_DESKBAR_DIRECTORY,
	B_USER_PRINTERS_DIRECTORY,
	B_USER_TRANSLATORS_DIRECTORY,
	B_USER_MEDIA_NODES_DIRECTORY,
	B_USER_SOUNDS_DIRECTORY,
	// Global directories.
	B_APPS_DIRECTORY,
	B_PREFERENCES_DIRECTORY,
	B_UTILITIES_DIRECTORY
};

const int32 directoryCount = sizeof(directories) / sizeof(directory_which);

const char *testFile		= "/tmp/testFile";
const char *testMountPoint	= "/non-existing-mount-point";


// Suite
CppUnit::Test*
FindDirectoryTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<FindDirectoryTest> TC;
		
	suite->addTest( new TC("find_directory() Test",
						   &FindDirectoryTest::Test) );
		
	return suite;
}		

// setUp
void
FindDirectoryTest::setUp()
{
	BasicTest::setUp();
	createVolume(testFile, testMountPoint, 1);
}
	
// tearDown
void
FindDirectoryTest::tearDown()
{
	deleteVolume(testFile, testMountPoint);
	BasicTest::tearDown();
}

// print_directories
/*static
void
print_directories(dev_t device)
{
	printf("device id: %ld\n", device);
	BVolume volume;
	status_t error = volume.SetTo(device);
	if (error != B_OK)
		printf("Failed to init volume\n");
	for (int32 i = 0; error == B_OK && i < directoryCount; i++) {
		BPath path;
		error = find_directory(directories[i], &path, false, &volume);
		if (error == B_OK)
			printf("%4d: `%s'\n", directories[i], path.Path());
		else
			printf("Failed to find directory: %s\n", strerror(error));
	}
}*/

// test_find_directory
static
status_t
test_find_directory(directory_which dir, BPath &path, dev_t device)
{
	status_t error = B_BAD_VALUE;
	switch (dir) {
		// volume relative dirs
		case B_DESKTOP_DIRECTORY:
		{
			if (device < 0)
				device = dev_for_path("/boot");
			fs_info info;
			if (fs_stat_dev(device, &info) == 0) {
				if (!strcmp(info.fsh_name, "bfs")) {
					entry_ref ref(device, info.root, "home");
					BPath homePath(&ref);
					error = homePath.InitCheck();
					if (error == B_OK)
						path.SetTo(homePath.Path(), "Desktop");
				} else
					error = B_ENTRY_NOT_FOUND;
			} else
				error = errno;
			break;
		}
		case B_TRASH_DIRECTORY:
		{
			if (device < 0)
				device = dev_for_path("/boot");
			fs_info info;
			if (fs_stat_dev(device, &info) == 0) {
				if (!strcmp(info.fsh_name, "bfs")) {
					entry_ref ref(device, info.root, "home");
					BPath homePath(&ref);
					error = homePath.InitCheck();
					if (error == B_OK)
						path.SetTo(homePath.Path(), "Desktop/Trash");
				} else if (!strcmp(info.fsh_name, "dos")) {
					entry_ref ref(device, info.root, "RECYCLED");
					BPath recycledPath(&ref);
					error = recycledPath.InitCheck();
					if (error == B_OK)
						path.SetTo(recycledPath.Path(), "_BEOS_");
				} else
					error = B_ENTRY_NOT_FOUND;
			} else
				error = errno;
			break;
		}
		// BeOS directories.  These are mostly accessed read-only.
		case B_BEOS_DIRECTORY:
			error = path.SetTo("/boot/beos");
			break;
		case B_BEOS_SYSTEM_DIRECTORY:
			error = path.SetTo("/boot/beos/system");
			break;
		case B_BEOS_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons");
			break;
		case B_BEOS_BOOT_DIRECTORY:
			error = path.SetTo("/boot/beos/system/boot");
			break;
		case B_BEOS_FONTS_DIRECTORY:
			error = path.SetTo("/boot/beos/etc/fonts");
			break;
		case B_BEOS_LIB_DIRECTORY:
			error = path.SetTo("/boot/beos/system/lib");
			break;
 		case B_BEOS_SERVERS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/servers");
			break;
		case B_BEOS_APPS_DIRECTORY:
			error = path.SetTo("/boot/beos/apps");
			break;
		case B_BEOS_BIN_DIRECTORY:
			error = path.SetTo("/boot/beos/bin");
			break;
		case B_BEOS_ETC_DIRECTORY:
			error = path.SetTo("/boot/beos/etc");
			break;
		case B_BEOS_DOCUMENTATION_DIRECTORY:
			error = path.SetTo("/boot/beos/documentation");
			break;
		case B_BEOS_PREFERENCES_DIRECTORY:
			error = path.SetTo("/boot/beos/preferences");
			break;
		case B_BEOS_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons/Translators");
			break;
		case B_BEOS_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons/media");
			break;
		case B_BEOS_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/beos/etc/sounds");
			break;
		// Common directories, shared among all users.
		case B_COMMON_DIRECTORY:
			error = path.SetTo("/boot/home");
			break;
		case B_COMMON_SYSTEM_DIRECTORY:
			error = path.SetTo("/boot/home/config");
			break;
		case B_COMMON_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons");
			break;
		case B_COMMON_BOOT_DIRECTORY:
			error = path.SetTo("/boot/home/config/boot");
			break;
		case B_COMMON_FONTS_DIRECTORY:
			error = path.SetTo("/boot/home/config/fonts");
			break;
		case B_COMMON_LIB_DIRECTORY:
			error = path.SetTo("/boot/home/config/lib");
			break;
		case B_COMMON_SERVERS_DIRECTORY:
			error = path.SetTo("/boot/home/config/servers");
			break;
		case B_COMMON_BIN_DIRECTORY:
			error = path.SetTo("/boot/home/config/bin");
			break;
		case B_COMMON_ETC_DIRECTORY:
			error = path.SetTo("/boot/home/config/etc");
			break;
		case B_COMMON_DOCUMENTATION_DIRECTORY:
			error = path.SetTo("/boot/home/config/documentation");
			break;
		case B_COMMON_SETTINGS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings");
			break;
		case B_COMMON_DEVELOP_DIRECTORY:
			error = path.SetTo("/boot/develop");
			break;
		case B_COMMON_LOG_DIRECTORY:
			error = path.SetTo("/boot/var/log");
			break;
		case B_COMMON_SPOOL_DIRECTORY:
			error = path.SetTo("/boot/var/spool");
			break;
		case B_COMMON_TEMP_DIRECTORY:
			error = path.SetTo("/boot/var/tmp");
			break;
		case B_COMMON_VAR_DIRECTORY:
			error = path.SetTo("/boot/var");
			break;
		case B_COMMON_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/Translators");
			break;
		case B_COMMON_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/media");
			break;
		case B_COMMON_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/home/config/sounds");
			break;
		// User directories.  These are interpreted in the context
		// of the user making the find_directory call.
		case B_USER_DIRECTORY:
			error = path.SetTo("/boot/home");
			break;
		case B_USER_CONFIG_DIRECTORY:
			error = path.SetTo("/boot/home/config");
			break;
		case B_USER_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons");
			break;
		case B_USER_BOOT_DIRECTORY:
			error = path.SetTo("/boot/home/config/boot");
			break;
		case B_USER_FONTS_DIRECTORY:
			error = path.SetTo("/boot/home/config/fonts");
			break;
		case B_USER_LIB_DIRECTORY:
			error = path.SetTo("/boot/home/config/lib");
			break;
		case B_USER_SETTINGS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings");
			break;
		case B_USER_DESKBAR_DIRECTORY:
			error = path.SetTo("/boot/home/config/be");
			break;
		case B_USER_PRINTERS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings/printers");
			break;
		case B_USER_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/Translators");
			break;
		case B_USER_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/media");
			break;
		case B_USER_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/home/config/sounds");
			break;
		// Global directories.
		case B_APPS_DIRECTORY:
			error = path.SetTo("/boot/apps");
			break;
		case B_PREFERENCES_DIRECTORY:
			error = path.SetTo("/boot/preferences");
			break;
		case B_UTILITIES_DIRECTORY:
			error = path.SetTo("/boot/utilities");
			break;
	}
	return error;
}

// TestDirectories
static
void
TestDirectories(dev_t device)
{
	BVolume volume;
	if (device >= 0)
		CPPUNIT_ASSERT( volume.SetTo(device) == B_OK );
	for (int32 i = 0; i < directoryCount; i++) {
		BPath path;
		BPath path2;
		char path3[B_PATH_NAME_LENGTH + 1];
		status_t result = test_find_directory(directories[i], path, device);
		status_t result2 = find_directory(directories[i], &path2, false,
										  &volume);
		status_t result3 = find_directory(directories[i], device, false,
										  path3, B_PATH_NAME_LENGTH + 1);
		CPPUNIT_ASSERT( result == result2 && result == result3 );
		if (result == B_OK)
			CPPUNIT_ASSERT( path == path2 && path == path3 );
	}
}

// Test
void
FindDirectoryTest::Test()
{
	// /boot
	NextSubTest();
	dev_t device = dev_for_path("/boot");
	CPPUNIT_ASSERT( device > 0 );
	TestDirectories(device);
	// /dev
	NextSubTest();
	device = dev_for_path("/dev");
	CPPUNIT_ASSERT( device > 0 );
	TestDirectories(device);
	// /
	NextSubTest();
	device = dev_for_path("/");
	CPPUNIT_ASSERT( device > 0 );
	TestDirectories(device);
	// test image
	NextSubTest();
	device = dev_for_path(testMountPoint);
	CPPUNIT_ASSERT( device > 0 );
	TestDirectories(device);
	// invalid device ID
	NextSubTest();
	TestDirectories(-1);
	// NULL BVolume
	NextSubTest();
	for (int32 i = 0; i < directoryCount; i++) {
		BPath path;
		BPath path2;
		status_t result = test_find_directory(directories[i], path, -1);
		status_t result2 = find_directory(directories[i], &path2, false, NULL);
		CPPUNIT_ASSERT( result == result2 );
		if (result == B_OK)
			CPPUNIT_ASSERT( path == path2 );
	}
	// no such volume
	NextSubTest();
	device = 213;
	fs_info info;
	while (fs_stat_dev(device, &info) == 0)
		device++;
	for (int32 i = 0; i < directoryCount; i++) {
		BPath path;
		char path3[B_PATH_NAME_LENGTH + 1];
		status_t result = test_find_directory(directories[i], path, device);
		status_t result3 = find_directory(directories[i], device, false,
										  path3, B_PATH_NAME_LENGTH + 1);
		// Our test_find_directory() returns rather strange errors instead
		// of B_ENTRY_NOT_FOUND.
		CPPUNIT_ASSERT( result == B_OK && result3 == B_OK
						|| result != B_OK && result3 != B_OK );
		if (result == B_OK)
			CPPUNIT_ASSERT( path == path3 );
	}
	// bad args
	// R5: crashes
	NextSubTest();
	device = dev_for_path("/boot");
	CPPUNIT_ASSERT( device > 0 );
#if !TEST_R5
	CPPUNIT_ASSERT( find_directory(B_BEOS_DIRECTORY, NULL, false, NULL)
					== B_BAD_VALUE );
	CPPUNIT_ASSERT( find_directory(B_BEOS_DIRECTORY, device, false, NULL, 50)
					== B_BAD_VALUE );
#endif
	// small buffer
	NextSubTest();
	char path3[7];
	CPPUNIT_ASSERT( find_directory(B_BEOS_DIRECTORY, device, false, path3, 7)
					== E2BIG );
}





