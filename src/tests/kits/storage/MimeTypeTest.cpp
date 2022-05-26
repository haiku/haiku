// MimeTypeTest.cpp

#include <ctype.h>			// For tolower()
#include <fcntl.h>			// open()
#include <map>
#include <queue>
#include <stdio.h>
#include <string.h>			// For memcmp()
#include <string>
#include <unistd.h>
#include <vector>


#include <fs_attr.h>		// For struct attr_info
#include <fs_info.h>
#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <Drivers.h>		// B_GET_ICON, device_icon
#include <Message.h>
#include <Mime.h>
#if !TEST_R5
	#include <mime/database_support.h>
#endif
#include <Path.h>			// Only needed for entry_ref dumps
#include <StorageKit.h>
#include <String.h>
#include <storage_support.h>	// for split_path()

#include "TestShell.h"
#include "TestApp.h"
#include "TestUtils.h"

#include "MimeTypeTest.h"

// MIME database directories
static const char *testDir				= "/tmp/mimeTestDir";
static const char *R5DatabaseDir		= "/boot/home/config/settings/beos_mime";
#if TEST_R5
static std::string mimeDatabaseDir		= R5DatabaseDir;
#else
static std::string mimeDatabaseDir		= BPrivate::Storage::Mime::kDatabaseDir;
#endif

// MIME Test Types
// testType and testTypeApp are Delete()d after each test.
static const char *testType				= "text/x-vnd.obos-Storage-Kit-Test";
static const char *testType1			= "text/x-vnd.obos-Storage-Kit-Test1";
static const char *testType2			= "text/x-vnd.obos-Storage-Kit-Test2";
static const char *testType3			= "text/x-vnd.obos-Storage-Kit-Test3";
static const char *testType4			= "text/x-vnd.obos-Storage-Kit-Test4";
static const char *testType5			= "text/x-vnd.obos-Storage-Kit-Test5";
static const char *testTypeApp			= "application/StorageKit-Test";
static const char *testTypeApp1			=  "application/"
										   "x-vnd.obos.mime.test.test1";
static const char *testTypeApp2			=  "application/"
										   "x-vnd.obos.mime.test.test2";
static const char *testTypeApp3			=  "application/"
										   "x-vnd.obos.mime.test.test3";
static const char *testTypeInvalid		= "text/Are spaces valid?";
static const char *testTypeSuperValid	= "valid-but-fake-supertype";
static const char *testTypeSuperInvalid	= "?????";

// Real MIME types
static const char *wildcardType			= "application/octet-stream";
static const char *applicationSupertype	= "application";

// Application Paths
static const char *testApp				= "/boot/beos/apps/SoundRecorder";
static const char *testApp2				= "/boot/beos/apps/CDPlayer";
static const char *fakeTestApp			= "/__this_isn't_likely_to_exist__";

// BMessage field names
static const char *applicationsField		= "applications";
static const char *typeField				= "type";
static const char *typesField				= "types";
static const char *fileExtField				= "extensions";
static const char *attrInfoField_Name		= "attr:name";
static const char *attrInfoField_PublicName	= "attr:public_name";
static const char *attrInfoField_Type		= "attr:type";
static const char *attrInfoField_Viewable	= "attr:viewable";
static const char *attrInfoField_Editable	= "attr:editable";

// Descriptions
static const char *testDescr			= "Just a test, nothing more :-)";
static const char *testDescr2			= "Another amazing test string";
static const char *longDescr			=
"This description is longer than B_MIME_TYPE_LENGTH, which is quite useful for certain things... "
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";
// Signatures
static const char *testSig				= "application/x-vnd.obos.mime-type-test";
static const char *testSig2				= "application/x-vnd.obos.mime-type-test-2";
static const char *longSig				= "application/x-vnd.obos.mime-type-test-long."
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";

// Declarations for handy dandy private functions
static bool operator==(BBitmap &bmp1, BBitmap &bmp2);
static bool operator!=(BBitmap &bmp1, BBitmap &bmp2);
static bool operator==(BMessage &msg1, BMessage &msg2);
static bool operator!=(BMessage &msg1, BMessage &msg2);
static void fill_bitmap(BBitmap &bmp, char value);
//static void dump_bitmap(BBitmap &bmp, char *name = "bmp");
#if !TEST_R5
	static status_t reduce_color_depth(BBitmap &src32, BBitmap &dest8);
#endif
//static void dump_ref(entry_ref *ref, char* name = "ref");
static void to_lower(const char *str, std::string &result);
static std::string to_lower(const char *str);
static void remove_type(const char *type, const char *databaseDir = mimeDatabaseDir.c_str());
static bool type_exists(const char *type, const char *databaseDir = mimeDatabaseDir.c_str());
class ContainerAdapter;
class SetAdapter;
class QueueAdapter;
void FillWithMimeTypes(ContainerAdapter &container, BMessage &typeMessage, const char* fieldName);
	// Used to add all the types in a BMessage from GetInstalled*Types() to some
	// sort of container object (via an adapter to support differing interfaces).

// Custom TestSuite class to allow us to make a copy of the MIME database directory
// before running all the BMimeType tests
class MimeTypeTestSuite : public CppUnit::TestSuite {
public:
	MimeTypeTestSuite() : CppUnit::TestSuite(), fMimeDirExisted(false) {}
	virtual void setUp()
	{
		// If we're using a directory other than the R5 MIME database directory, make
		// sure that directory exists. If not, make a complete copy of the R5 database
		// directory.
		if (mimeDatabaseDir != R5DatabaseDir) {
			BEntry dir(mimeDatabaseDir.c_str());
			if (dir.InitCheck() != B_OK || !dir.Exists()) {
				if (BTestShell::GlobalBeVerbose())
					cout << "(Making a copy of your MIME database at '" + mimeDatabaseDir + "')" << endl;
				std::string cmd = std::string("copyattr -d -r -- ") + R5DatabaseDir
					+ " " + mimeDatabaseDir;
				ExecCommand(cmd.c_str());
			} else {
				fMimeDirExisted = true;
				if (BTestShell::GlobalBeVerbose())
					cout << "(Using existing copy of MIME database in '" + mimeDatabaseDir + "')" << endl;
			}
		}
	}

	virtual void tearDown()
	{
		if (mimeDatabaseDir != R5DatabaseDir && !fMimeDirExisted) {
			if (BTestShell::GlobalBeVerbose())
				cout << "(Removing copy of MIME database in '" + mimeDatabaseDir + "')" << endl;
			std::string cmd = std::string("rm -rf ") + mimeDatabaseDir;
			ExecCommand(cmd.c_str());
		}
	}

	virtual void run( CppUnit::TestResult *result )
	{
		setUp();
		CppUnit::TestSuite::run(result);
		tearDown();
	}
private:
	bool fMimeDirExisted;
};

// Suite
CppUnit::Test*
MimeTypeTest::Suite() {
	MimeTypeTestSuite *suite = new MimeTypeTestSuite();
	typedef CppUnit::TestCaller<MimeTypeTest> TC;

	// Tyler
	suite->addTest( new TC("BMimeType::Install/Delete Test",
						   &MimeTypeTest::InstallDeleteTest) );
	suite->addTest( new TC("BMimeType::App Hint Test",
						   &MimeTypeTest::AppHintTest) );
	suite->addTest( new TC("BMimeType::Attribute Info Test",
						   &MimeTypeTest::AttrInfoTest) );
	suite->addTest( new TC("BMimeType::Long Description Test",
						   &MimeTypeTest::LongDescriptionTest) );
	suite->addTest( new TC("BMimeType::Short Description Test",
						   &MimeTypeTest::ShortDescriptionTest) );
	suite->addTest( new TC("BMimeType::File Extensions Test",
						   &MimeTypeTest::FileExtensionsTest) );
	suite->addTest( new TC("BMimeType::Icon Test (Large)",
						   &MimeTypeTest::LargeIconTest) );
	suite->addTest( new TC("BMimeType::Icon Test (Mini)",
						   &MimeTypeTest::MiniIconTest) );
	suite->addTest( new TC("BMimeType::Icon For Type Test (Large)",
						   &MimeTypeTest::LargeIconForTypeTest) );
	suite->addTest( new TC("BMimeType::Icon For Type Test (Mini)",
						   &MimeTypeTest::MiniIconForTypeTest) );
	suite->addTest( new TC("BMimeType::Installed Types Test",
						   &MimeTypeTest::InstalledTypesTest) );
	suite->addTest( new TC("BMimeType::Preferred App Test",
						   &MimeTypeTest::PreferredAppTest) );
	suite->addTest( new TC("BMimeType::Supporting Apps Test",
						   &MimeTypeTest::SupportingAppsTest) );
	suite->addTest( new TC("BMimeType::Supported Types Test",
						   &MimeTypeTest::SupportedTypesTest) );
	suite->addTest( new TC("BMimeType::Wildcard Apps Test",
						   &MimeTypeTest::WildcardAppsTest) );

	// Ingo
	suite->addTest( new TC("BMimeType::Initialization Test",
						   &MimeTypeTest::InitTest) );
	suite->addTest( new TC("BMimeType::MIME String Test",
						   &MimeTypeTest::StringTest) );
	suite->addTest( new TC("BMimeType::MIME Monitoring Test",
						   &MimeTypeTest::MonitoringTest) );
	suite->addTest( new TC("BMimeType::update_mime_info() Test",
						   &MimeTypeTest::UpdateMimeInfoTest) );
	suite->addTest( new TC("BMimeType::create_app_meta_mime() Test",
						   &MimeTypeTest::CreateAppMetaMimeTest) );
	suite->addTest( new TC("BMimeType::get_device_icon() Test",
						   &MimeTypeTest::GetDeviceIconTest) );
	suite->addTest( new TC("BMimeType::Sniffer Rule Test",
						   &MimeTypeTest::SnifferRuleTest) );
	suite->addTest( new TC("BMimeType::Sniffing Test",
						   &MimeTypeTest::SniffingTest) );


	return suite;
}

// Handy comparison operators for BBitmaps. The size and color depth
// are compared first, followed by the bitmap data.
bool
operator==(BBitmap &bmp1, BBitmap &bmp2) {
	if (bmp1.Bounds() == bmp2.Bounds()) {
//		printf("bmp== 1\n");
		if (bmp1.ColorSpace() == bmp2.ColorSpace()) {
//			printf("bmp== 2\n");
			char *data1 = (char*)bmp1.Bits();
			char *data2 = (char*)bmp2.Bits();
			// NOTE! It's possible that padding bits might not get copied verbatim,
			// which could lead to unexpected failures. If things are acting weird,
			// you might try the commented out code below (keeping in mind that it
			// currently only works for 8-bit color depths).
			for (int i = 0; i < bmp1.BitsLength(); data1++, data2++, i++) {
				if (*data1 != *data2) {
//					printf("i == %d\n", i);
					return false;
				}
			}
/*			for (int i = 0; i < bmp1.Bounds().IntegerHeight(); i++) {
				for (int j = 0; j < bmp1.Bounds().IntegerWidth(); j++) {
//					printf("(%d, %d)", data1[(i * bmp1.BytesPerRow()) + j], data2[(i * bmp2.BytesPerRow()) + j]);
					if (data1[(i * bmp1.BytesPerRow()) + j] != data2[(i * bmp2.BytesPerRow()) + j]) {
						printf("fail at (%d, %d)\n", j, i);
						return false;
					}
				}
			}*/
			return true;
		} else
			return false;
	} else
		return false;
}

bool
operator!=(BBitmap &bmp1, BBitmap &bmp2) {
	return !(bmp1 == bmp2);
}

// Handy comparison operators for BMessages. The BMessages are checked field
// by field, each of which is verified to be identical with respect to: type,
// count, and data (all items).
bool
operator==(BMessage &msg1, BMessage &msg2) {
	status_t err = B_OK;

	// For now I'm ignoring the what fields...I shall deal with that later :-)
	if (msg1.what != msg2.what)
		return false;

/*
	printf("----------------------------------------------------------------------\n");
	msg1.PrintToStream();
	msg2.PrintToStream();
	printf("----------------------------------------------------------------------\n");
*/

	// Check the counts of field names
	int count1, count2;
	count1 = msg1.CountNames(B_ANY_TYPE);
	count2 = msg2.CountNames(B_ANY_TYPE);
	if (count1 != count2 && (count1 == 0 || count2 == 0))
		return false;

	// Iterate over all the names in msg1 and check that the field
	// with the same name exists in msg2, is of the same type, and
	// contains identical data.
	for (int i = 0; i < count1 && !err; i++) {
		char *name;
		type_code typeFound1, typeFound2;
		int32 countFound1, countFound2;

		// Check type and count info
		err = msg1.GetInfo(B_ANY_TYPE, i, &name, &typeFound1, &countFound1);
		if (!err)
			err = msg2.GetInfo(name, &typeFound2, &countFound2);
		if (!err)
			err = (typeFound1 == typeFound2 && countFound1 == countFound2 ? B_OK : B_ERROR);
		if (!err) {
			// Check all the data items
			for (int j = 0; j < countFound1; j++) {
				void *data1, *data2;
				ssize_t bytes1, bytes2;

				err = msg1.FindData(name, typeFound1, j, (const void**)&data1, &bytes1);
				if (!err)
					err = msg2.FindData(name, typeFound2, j, (const void**)&data2, &bytes2);
				if (!err)
					err = (bytes1 == bytes2 && memcmp(data1, data2, bytes1) == 0 ? B_OK : B_ERROR);
			}
		}
	}

	return !err;
}

bool
operator!=(BMessage &msg1, BMessage &msg2) {
	return !(msg1 == msg2);
}

// Fills the bitmap data with the given character
void
fill_bitmap(BBitmap &bmp, char value) {
	char *data = (char*)bmp.Bits();
	for (int i = 0; i < bmp.BitsLength(); data++, i++) {
//		printf("(%d -> ", *data);
		*data = value;
//		printf("%d)", *data);
	}
//	printf("\n");
}

// Fills the bitmap data with the given character
void
fill_bitmap32(BBitmap &bmp, char r, char g, char b, char a) {
	if (bmp.ColorSpace() == B_RGB32 || bmp.ColorSpace() == B_RGBA32) {
		char *data = (char*)bmp.Bits();
		for (int i = 0; i+3 < bmp.BitsLength(); data += 4, i+= 4) {
			data[0] = b;
			data[1] = g;
			data[2] = r;
			data[3] = a;
//			printf("(%d,%d,%d,%d)", data[2], data[1], data[0], data[3]);
		}
//		printf("\n");
	}
}

// Dumps the size, colorspace, and first data byte
// of the bitmap to stdout
/*void
dump_bitmap(BBitmap &bmp, char *name = "bmp") {
	printf("%s == (%ldx%ld, ", name, bmp.Bounds().IntegerWidth()+1,
		bmp.Bounds().IntegerHeight()+1);
	switch (bmp.ColorSpace()) {
		case B_CMAP8:
			printf("B_CMAP8");
			break;

		case B_RGB32:
			printf("B_RGB32");
			break;

		case B_RGBA32:
			printf("B_RGBA32");
			break;

		default:
			printf("%x", bmp.ColorSpace());
			break;
	}
	printf(", %d, [", *(char*)bmp.Bits());
	char *data = (char*)bmp.Bits();
	for (int i = 0; i < bmp.BitsLength() && i < 20; data++, i++)
		printf("%d,", *data);
	printf("]\n");
}*/

// Uses BBitmap::SetBits() to convert the B_RGB32 bitmap in src32
// to a B_CMAP8 bitmap in dest8
#if !TEST_R5
status_t
reduce_color_depth(BBitmap &src32, BBitmap &dest8)
{
	status_t err = (src32.ColorSpace() == B_RGB32
				      && dest8.ColorSpace() == B_CMAP8
				        && src32.InitCheck() == B_OK
				          && dest8.InitCheck() == B_OK
					        && src32.Bounds() == dest8.Bounds())
					          ? B_OK
					            : B_BAD_VALUE;
	if (!err) {
		// Set each pixel individually, since SetBits() for B_RGB32 takes
		// 24-bit rgb pixel data...
		char *data = (char*)src32.Bits();
		for (int32 i = 0; i*4+3 < src32.BitsLength(); data += 4, i++) {
			char rgb[3];
			rgb[0] = data[2];	// red
			rgb[1] = data[1];	// green
			rgb[2] = data[0];	// blue
			dest8.SetBits(rgb, 3, i, B_RGB32);
		}
	}
	return err;
}
#endif

// IconHelper and IconForTypeHelper:
// Adapter(?) classes needed to reuse icon tests among {Get,Set}Icon() and {Get,Set}IconForType()
// What originally were meant to encapsulate the variations among calls to the various BMimeType
// icon functions have now exploded into beefy helper classes with a bunch of BBitmap related
// functionality as well. A lot of this stuff doesn't necessarily belong

class IconHelper {
public:
	IconHelper(icon_size which)
		: bmp1(BitmapBounds(which), B_CMAP8),
		  bmp2(BitmapBounds(which), B_CMAP8),
		  bmpTemp(BitmapBounds(which), B_CMAP8),
		  size(which)
	{
		// Initialize our three bitmaps to different "colors"
		fill_bitmap(bmp1, 1);
		fill_bitmap(bmp2, 2);
		fill_bitmap(bmpTemp, 3);
	}

	virtual ~IconHelper() {}

	// Returns the proper bitmap bounds for the given icon size
	BRect BitmapBounds(icon_size isize) {
		return isize == B_LARGE_ICON ? BRect(0,0,31,31) : BRect(0,0,15,15);
	}

	// Returns the proper bitmap bounds for this helper's icon size
	BRect BitmapBounds() {
		return BitmapBounds(size);
	}

	// Used to call the appropriate GetIcon[ForType] function
	virtual status_t GetIcon(BMimeType &mime, BBitmap *icon) {
		return mime.GetIcon(icon, size);
	}

	// Used to call the appropriate SetIcon[ForType] function
	virtual status_t SetIcon(BMimeType &mime, BBitmap *icon) {
		return mime.SetIcon(icon, size);
	}

	// Used to call the appropriate DeleteIcon[ForType] function
	virtual status_t DeleteIcon(BMimeType &mime) {
#if TEST_R5
		return B_BAD_VALUE;
#else
		return mime.DeleteIcon(size);
#endif
	}

	BBitmap* TempBitmap() {
		return &bmpTemp;
	}

	BBitmap* Bitmap1() {
		return &bmp1;
	}

	BBitmap* Bitmap2() {
		return &bmp2;
	}

	icon_size Size() {
		return size;
	}

protected:
	BBitmap bmp1;
	BBitmap bmp2;
	BBitmap bmpTemp;
	icon_size size;
};

class IconForTypeHelper : public IconHelper {
public:
	IconForTypeHelper(const char *fileType, icon_size which)
		: IconHelper(which), fileType(fileType) {}
	virtual ~IconForTypeHelper() {}
	virtual status_t GetIcon(BMimeType &mime, BBitmap *icon) {
		return mime.GetIconForType(fileType.c_str(), icon, size);
	}
	virtual status_t SetIcon(BMimeType &mime, BBitmap *icon) {
		return mime.SetIconForType(fileType.c_str(), icon, size);
	}
	virtual status_t DeleteIcon(BMimeType &mime) {
#if TEST_R5
		return B_BAD_VALUE;
#else
		return mime.DeleteIconForType(fileType.c_str(), size);
#endif
	}
protected:
	std::string fileType;
};

// Adapter classes used by FillWithMimeTypes() to facilitate
// addition of strings to containers with varying interfaces
class ContainerAdapter {
public:
	virtual void Add(std::string value) = 0;
};

class SetAdapter : public ContainerAdapter {
public:
	SetAdapter(std::set<std::string> &set)
		: fSet(set) { }
	virtual void Add(std::string value) {
		fSet.insert(value);
	}
protected:
	std::set<std::string> &fSet;
};

class QueueAdapter : public ContainerAdapter {
public:
	QueueAdapter(std::queue<std::string> &queue)
		: fQueue(queue) { }
	virtual void Add(std::string value) {
		fQueue.push(value);
	}
protected:
	std::queue<std::string> &fQueue;
};


// setUp
void
MimeTypeTest::setUp()
{
	BasicTest::setUp();
	execCommand(string("mkdir ") + testDir);
/*	// Better not to play with fire, so we'll make a copy of the
	// local mime database which we'll use for certain Haiku tests
	execCommand(string("mkdir ") + testDir
				+ " ; copyattr -d -r -- " + mimeDatabaseDir + "/\* " + testDir
				); */
	// Setup our application
	fApplication = new BTestApp(testSig);
	if (fApplication->Init() != B_OK) {
		fprintf(stderr, "Failed to initialize application.\n");
		delete fApplication;
		fApplication = NULL;
	}

}

// tearDown
void
MimeTypeTest::tearDown()
{
	execCommand(string("rm -rf ") + testDir);

	// Uninistall our test type
	//! /todo Uncomment the following uninstall code when all is said and done.
/*	BMimeType mime(testType);
	status_t err = mime.InitCheck();
	if (!err && mime.IsInstalled())
		err = mime.Delete();
	if (err)
		fprintf(stderr, "Failed to unistall test type \"%s\"\n", testType); */
	// Terminate the Application
	if (fApplication) {
		fApplication->Terminate();
		delete fApplication;
		fApplication = NULL;
	}
	// remove the types we've added
	const char * const testTypes[] = {
		testType, testType1, testType2, testType3, testType4, testType5,
		testTypeApp, testTypeApp1, testTypeApp2, testTypeApp3,
	};
	for (uint32 i = 0; i < sizeof(testTypes) / sizeof(const char*); i++) {
		BMimeType type(testTypes[i]);
		type.Delete();
	}
	BasicTest::tearDown();
}

// entry_ref dumping function ; this may be removed at any time

/*void
dump_ref(entry_ref *ref, char* name = "ref") {
	if (ref) {
		BPath path(ref);
		status_t err = path.InitCheck();
		if (!err) {
			printf("%s == '%s'", name, path.Path());
		} else
			printf("%s == ERROR", name);
		printf(" == (%ld, %Ld, '%s')\n", ref->device, ref->directory, ref->name);

	} else
		printf("%s == (NULL)\n", name);
}*/

// App Hint

void
MimeTypeTest::AppHintTest() {
	// init a couple of entry_refs to applications
	BEntry entry(testApp);
	entry_ref appRef;
	CHK(entry.InitCheck() == B_OK);
	CHK(entry.GetRef(&appRef) == B_OK);
	BEntry entry2(testApp2);
	entry_ref appRef2;
	CHK(entry2.InitCheck() == B_OK);
	CHK(entry2.GetRef(&appRef2) == B_OK);
	// Uninitialized
	NextSubTest();
	{
		BMimeType mime;
		entry_ref ref;
		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(mime.GetAppHint(&ref) != B_OK);		// R5 == B_BAD_VALUE
		CHK(mime.SetAppHint(&ref) != B_OK);		// R5 == B_BAD_VALUE
	}
	// NULL params
	NextSubTest();
	{
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
#if TEST_R5
		CHK(!mime.IsInstalled());
		CHK(mime.GetAppHint(NULL) != B_OK);		// R5 == B_BAD_VALUE
		CHK(!mime.IsInstalled());
		CHK(mime.SetAppHint(NULL) != B_OK);		// Installs, R5 == B_ENTRY_NOT_FOUND
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(NULL) != B_OK);		// R5 == B_BAD_VALUE
		CHK(mime.SetAppHint(NULL) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
#else
		CHK(!mime.IsInstalled());
		CHK(mime.GetAppHint(NULL) != B_OK);		// B_BAD_VALUE
		CHK(!mime.IsInstalled());
		CHK(mime.SetAppHint(NULL) != B_OK);		// B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == appRef);
		CHK(mime.SetAppHint(NULL) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) != B_OK);		// B_ENTRY_NOT_FOUND
#endif
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeleteAppHint() != B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == appRef);
		CHK(mime.DeleteAppHint() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) != B_OK);
#endif
	}
	// Non-installed type
	NextSubTest();
	{
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == appRef);
	}
	// Installed Type
	NextSubTest();
	{
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Get() with no apphint installed
		CHK(mime.GetAppHint(&ref) == B_ENTRY_NOT_FOUND);
		// Initial Set()/Get()
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == appRef);
		// Followup Set()/Get()
		CHK(mime.SetAppHint(&appRef2) == B_OK);
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == appRef2);
		CHK(ref != appRef);
	}
	// Installed Type, invalid entry_ref
	NextSubTest();
	{
		entry_ref ref(-1, -1, NULL);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.SetAppHint(&ref) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Installed Type, fake/invalid entry_ref
	NextSubTest();
	{
		entry_ref ref(0, 0, "__this_ought_not_exist__");
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.SetAppHint(&ref) != B_OK);	// R5 == B_ENTRY_NOT_FOUND
	}
	// Installed Type, abstract entry_ref
	NextSubTest();
	{
		entry_ref fakeRef;
		entry_ref ref;
		BEntry entry(fakeTestApp);
		CHK(entry.InitCheck() == B_OK);
		CHK(!entry.Exists());
		CHK(entry.GetRef(&fakeRef) == B_OK);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.SetAppHint(&appRef) == B_OK);
		CHK(mime.SetAppHint(&fakeRef) == B_OK);
		CHK(mime.GetAppHint(&ref) == B_OK);
		CHK(ref == fakeRef);
		CHK(ref != appRef);
	}
}

// Attr Info

void
MimeTypeTest::AttrInfoTest() {
	// Create some messages to sling around
	const int32 WHAT = 233;	// This is the what value that GAI() returns...not sure if it has a name yet
	BMessage msg1(WHAT), msg2(WHAT), msg3(WHAT), msgIncomplete1(WHAT), msgIncomplete2(WHAT);

	CHK(msg1.AddString(attrInfoField_Name, "Color") == B_OK);
	CHK(msg1.AddString(attrInfoField_PublicName, "The Color") == B_OK);
	CHK(msg1.AddInt32(attrInfoField_Type, B_STRING_TYPE) == B_OK);
	CHK(msg1.AddBool(attrInfoField_Viewable, true) == B_OK);
	CHK(msg1.AddBool(attrInfoField_Editable, true) == B_OK);

	CHK(msg1.AddString(attrInfoField_Name, "High Score") == B_OK);
	CHK(msg1.AddString(attrInfoField_PublicName, "The Highest Score Ever") == B_OK);
	CHK(msg1.AddInt32(attrInfoField_Type, B_INT32_TYPE) == B_OK);
	CHK(msg1.AddBool(attrInfoField_Viewable, false) == B_OK);
	CHK(msg1.AddBool(attrInfoField_Editable, false) == B_OK);

	CHK(msg2.AddString(attrInfoField_Name, "Volume") == B_OK);
	CHK(msg2.AddString(attrInfoField_PublicName, "Loudness") == B_OK);
	CHK(msg2.AddInt32(attrInfoField_Type, B_DOUBLE_TYPE) == B_OK);
	CHK(msg2.AddBool(attrInfoField_Viewable, true) == B_OK);
	CHK(msg2.AddBool(attrInfoField_Editable, true) == B_OK);

	CHK(msg3.AddString(attrInfoField_Name, "Volume") == B_OK);
	CHK(msg3.AddString(attrInfoField_PublicName, "Loudness") == B_OK);
	CHK(msg3.AddInt32(attrInfoField_Type, B_DOUBLE_TYPE) == B_OK);
	CHK(msg3.AddBool(attrInfoField_Viewable, true) == B_OK);
	CHK(msg3.AddBool(attrInfoField_Editable, true) == B_OK);

	CHK(msgIncomplete1.AddString(attrInfoField_Name, "Color") == B_OK);
	CHK(msgIncomplete1.AddString(attrInfoField_PublicName, "The Color") == B_OK);
	CHK(msgIncomplete1.AddInt32(attrInfoField_Type, B_STRING_TYPE) == B_OK);
	CHK(msgIncomplete1.AddBool(attrInfoField_Viewable, true) == B_OK);
	CHK(msgIncomplete1.AddBool(attrInfoField_Editable, true) == B_OK);

	CHK(msgIncomplete1.AddString(attrInfoField_Name, "High Score") == B_OK);
//	CHK(msgIncomplete1.AddString(attrInfoField_PublicName, "The Highest Score Ever") == B_OK);
	CHK(msgIncomplete1.AddInt32(attrInfoField_Type, B_INT32_TYPE) == B_OK);
//	CHK(msgIncomplete1.AddBool(attrInfoField_Viewable, false) == B_OK);
	CHK(msgIncomplete1.AddBool(attrInfoField_Editable, false) == B_OK);

	CHK(msgIncomplete2.AddString(attrInfoField_Name, "Color") == B_OK);
//	CHK(msgIncomplete2.AddString(attrInfoField_PublicName, "The Color") == B_OK);
//	CHK(msgIncomplete2.AddInt32(attrInfoField_Type, B_STRING_TYPE) == B_OK);
//	CHK(msgIncomplete2.AddBool(attrInfoField_Viewable, true) == B_OK);
	CHK(msgIncomplete2.AddBool(attrInfoField_Editable, true) == B_OK);

	CHK(msg1 == msg1);
	CHK(msg2 == msg2);
	CHK(msg3 == msg3);
	CHK(msg1 != msg2);
	CHK(msg1 != msg3);
	CHK(msg2 == msg3);

	// Uninitialized
	NextSubTest();
	{
		BMimeType mime;
		BMessage msg;

		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(mime.GetAttrInfo(&msg) != B_OK);		// R5 == B_BAD_VALUE
		CHK(mime.SetAttrInfo(&msg) != B_OK);		// R5 == B_BAD_VALUE
	}

	// NULL params
	NextSubTest();
	{
#if !TEST_R5
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeleteAttrInfo() != B_OK);
		CHK(!mime.IsInstalled());
		msg1.RemoveName(typeField);						// Clear "type" field, since SAI() just adds another
		CHK(mime.SetAttrInfo(&msg1) == B_OK);
		CHK(mime.IsInstalled());
		CHK(msg != msg1);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" field as GAI() does
		CHK(msg == msg1);
		CHK(mime.SetAttrInfo(NULL) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAttrInfo(&msg) != B_OK);
#endif
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeleteAttrInfo() != B_OK);
		CHK(!mime.IsInstalled());
		msg1.RemoveName(typeField);						// Clear "type" field, since SAI() just adds another
		CHK(mime.SetAttrInfo(&msg1) == B_OK);
		CHK(mime.IsInstalled());
		CHK(msg != msg1);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" field as GAI() does
		CHK(msg == msg1);
		CHK(mime.DeleteAttrInfo() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAttrInfo(&msg) != B_OK);
#endif
	}

	// Improperly formatted BMessages
	NextSubTest();
	{
		BMessage msg(WHAT);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Initial Set()/Get()
		msgIncomplete1.RemoveName(typeField);		// Clear "type" fields, since SAI() just adds another
		msgIncomplete2.RemoveName(typeField);
		CHK(msg != msgIncomplete1);
		CHK(msg != msgIncomplete2);
		CHK(mime.SetAttrInfo(&msgIncomplete1) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msgIncomplete1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msgIncomplete2.AddString(typeField, testType) == B_OK);
		CHK(msg == msgIncomplete1);
		CHK(msg != msgIncomplete2);
	}

	// Set() with improperly formatted message
	NextSubTest();
	{
		BMessage msg(WHAT);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Initial Set()/Get()
		msgIncomplete1.RemoveName(typeField);		// Clear "type" fields, since SAI() just adds another
		msgIncomplete2.RemoveName(typeField);
		CHK(msg != msgIncomplete1);
		CHK(msg != msgIncomplete2);
		CHK(mime.SetAttrInfo(&msgIncomplete1) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msgIncomplete1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msgIncomplete2.AddString(typeField, testType) == B_OK);
		CHK(msg == msgIncomplete1);
		CHK(msg != msgIncomplete2);
	}

	// Set() with empty message
	NextSubTest();
	{
		BMimeType mime(testType);
		BMessage msgEmpty(WHAT);
		BMessage msg(WHAT);
		CHK(msg.AddInt32("stuff", 1234) == B_OK);	// Add an extra attribute to give us something to compare with

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Set(empty)
		CHK(msg != msgEmpty);
		CHK(mime.SetAttrInfo(&msgEmpty) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msgEmpty.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg == msgEmpty);
	}

	// Set() with extra attributes in message
	NextSubTest();
	{
		BMimeType mime(testType);
		BMessage msg(WHAT);
		BMessage msgExtraSet(msg1);
		CHK(msgExtraSet.AddString("extra", ".extra") == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 123) == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 456) == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 789) == B_OK);
		BMessage msgExtraGet(msgExtraSet);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Set(extra)/Get(empty)
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msgExtraSet);
		CHK(mime.SetAttrInfo(&msgExtraSet) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msgExtraSet.AddString(typeField, testType) == B_OK);
		CHK(msg == msgExtraSet);
		CHK(msg != msg1);

		// Get(extra)
		NextSubTest();
		CHK(mime.GetAttrInfo(&msgExtraGet) == B_OK);
		CHK(msgExtraGet == msgExtraSet);
		CHK(msgExtraGet != msg1);

		// Get(extra and then some)
		NextSubTest();
		CHK(msgExtraGet.AddInt32("more_extras", 101112) == B_OK);
		msgExtraGet.RemoveName(typeField);		// Clear "type" fields to be fair, since SFE() just adds another
		CHK(mime.GetAttrInfo(&msgExtraGet) == B_OK);	// Reinitializes result (clearing extra fields)
		CHK(msgExtraGet == msgExtraSet);
		CHK(msgExtraGet != msg1);

	}
	// Normal Function (Non-installed type)
	NextSubTest();
	{
		BMimeType mime(testType);
		BMessage msg(WHAT);
		BMessage msg2(WHAT);

		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);

		CHK(!mime.IsInstalled());
		CHK(mime.GetAttrInfo(&msg) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetAttrInfo(&msg) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetAttrInfo(&msg2) == B_OK);
		CHK(msg.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GAI() does
		CHK(msg == msg2);
	}

	// Normal Function
	NextSubTest();
	{
		BMessage msg(WHAT);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Initial Set()/Get()
		msg1.RemoveName(typeField);		// Clear "type" fields, since SAI() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
		CHK(mime.SetAttrInfo(&msg1) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg == msg1);
		CHK(msg != msg2);

		// Followup Set()/Get()
		NextSubTest();
		CHK(msg.MakeEmpty() == B_OK);
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
		CHK(mime.SetAttrInfo(&msg2) == B_OK);
		CHK(mime.GetAttrInfo(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg != msg1);
		CHK(msg == msg2);

		// Clear
		NextSubTest();
		CHK(msg.MakeEmpty() == B_OK);
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
#if !TEST_R5
		CHK(mime.SetAttrInfo(NULL) == B_OK);		// R5 == CRASH! despite what one might think should happen
		CHK(mime.GetAttrInfo(&msg) != B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg != msg1);
		CHK(msg != msg2);
#endif
	}
}

// File Extensions

void
MimeTypeTest::FileExtensionsTest() {
	// Create some messages to sling around
	const int32 WHAT = 234;	// This is the what value that GFE returns...not sure if it has a name yet
	BMessage msg1(WHAT), msg2(WHAT), msg3(WHAT);

	CHK(msg1.AddString(fileExtField, ".data") == B_OK);
	CHK(msg1.AddString(fileExtField, ".txt") == B_OK);
	CHK(msg1.AddString(fileExtField, ".png") == B_OK);
	CHK(msg1.AddString(fileExtField, ".html") == B_OK);

	CHK(msg2.AddString(fileExtField, ".data") == B_OK);
	CHK(msg2.AddString(fileExtField, ".txt") == B_OK);

	CHK(msg3.AddString(fileExtField, ".data") == B_OK);
	CHK(msg3.AddString(fileExtField, ".txt") == B_OK);

	CHK(msg1 == msg1);
	CHK(msg2 == msg2);
	CHK(msg3 == msg3);
	CHK(msg1 != msg2);
	CHK(msg1 != msg3);
	CHK(msg2 == msg3);

	// Uninitialized
	NextSubTest();
	{
		BMessage msg(WHAT);
		BMimeType mime;

		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(mime.GetFileExtensions(&msg) != B_OK);	// R5 == B_BAD_VALUE
		CHK(mime.SetFileExtensions(&msg) != B_OK);	// R5 == B_BAD_VALUE
	}
	// NULL params
	NextSubTest();
	{
#if !TEST_R5
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeleteFileExtensions() != B_OK);
		CHK(!mime.IsInstalled());
		msg1.RemoveName(typeField);						// Clear "type" field, since SAI() just adds another
		CHK(mime.SetFileExtensions(&msg1) == B_OK);
		CHK(mime.IsInstalled());
		CHK(msg != msg1);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" field as GAI() does
		CHK(msg == msg1);
		CHK(mime.SetFileExtensions(NULL) == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetFileExtensions(&msg) != B_OK);
#endif
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeleteFileExtensions() != B_OK);
		CHK(!mime.IsInstalled());
		msg1.RemoveName(typeField);						// Clear "type" field, since SAI() just adds another
		CHK(mime.SetFileExtensions(&msg1) == B_OK);
		CHK(mime.IsInstalled());
		CHK(msg != msg1);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" field as GAI() does
		CHK(msg == msg1);
		CHK(mime.DeleteFileExtensions() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetFileExtensions(&msg) != B_OK);
#endif
	}
	// Set() with empty message
	NextSubTest();
	{
		BMimeType mime(testType);
		BMessage msgEmpty(WHAT);
		BMessage msg(WHAT);
		CHK(msg.AddInt32("stuff", 1234) == B_OK);	// Add an extra attribute to give us something to compare with

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Set(empty)
		CHK(msg != msgEmpty);
		CHK(mime.SetFileExtensions(&msgEmpty) == B_OK);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msgEmpty.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg == msgEmpty);
	}
	// Set() with extra attributes in message
	NextSubTest();
	{
		BMimeType mime(testType);
		BMessage msg(WHAT);
		BMessage msgExtraSet(msg1);
		CHK(msgExtraSet.AddString("extra", ".extra") == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 123) == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 456) == B_OK);
		CHK(msgExtraSet.AddInt32("more_extras", 789) == B_OK);
		BMessage msgExtraGet(msgExtraSet);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Set(extra)/Get(empty)
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msgExtraSet);
		CHK(mime.SetFileExtensions(&msgExtraSet) == B_OK);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msgExtraSet.AddString(typeField, testType) == B_OK);
		CHK(msg == msgExtraSet);
		CHK(msg != msg1);

		// Get(extra)
		NextSubTest();
		CHK(mime.GetFileExtensions(&msgExtraGet) == B_OK);
		CHK(msgExtraGet == msgExtraSet);
		CHK(msgExtraGet != msg1);

		// Get(extra and then some)
		NextSubTest();
		CHK(msgExtraGet.AddInt32("more_extras", 101112) == B_OK);
		msgExtraGet.RemoveName(typeField);		// Clear "type" fields to be fair, since SFE() just adds another
		CHK(mime.GetFileExtensions(&msgExtraGet) == B_OK);	// Reinitializes result (clearing extra fields)
		CHK(msgExtraGet == msgExtraSet);
		CHK(msgExtraGet != msg1);

	}
	// Normal function
	NextSubTest();
	{
		BMessage msg(WHAT);
		BMimeType mime(testType);

		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Initial Set()/Get()
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
		CHK(mime.SetFileExtensions(&msg1) == B_OK);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg == msg1);
		CHK(msg != msg2);

		// Followup Set()/Get()
		NextSubTest();
		CHK(msg.MakeEmpty() == B_OK);
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
		CHK(mime.SetFileExtensions(&msg2) == B_OK);
		CHK(mime.GetFileExtensions(&msg) == B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg != msg1);
		CHK(msg == msg2);

		// Clear
		NextSubTest();
		CHK(msg.MakeEmpty() == B_OK);
		msg1.RemoveName(typeField);		// Clear "type" fields, since SFE() just adds another
		msg2.RemoveName(typeField);
		CHK(msg != msg1);
		CHK(msg != msg2);
#if !TEST_R5
		CHK(mime.SetFileExtensions(NULL) == B_OK);		// R5 == CRASH! despite what the BeBook says
		CHK(mime.GetFileExtensions(&msg) != B_OK);
		CHK(msg1.AddString(typeField, testType) == B_OK);	// Add in "type" fields as GFE() does
		CHK(msg2.AddString(typeField, testType) == B_OK);
		CHK(msg != msg1);
		CHK(msg != msg2);
#endif
	}
}


// Icon Test Helper Function

void
MimeTypeTest::IconTest(IconHelper &helper) {
	BBitmap *bmp = helper.TempBitmap();
	// Unitialized
	NextSubTest();
	{
		BMimeType mime;
		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(helper.GetIcon(mime, bmp) != B_OK);	// R5 == B_BAD_VALUE
		CHK(helper.SetIcon(mime, bmp) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Non-installed type
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		// Test
		CHK(helper.GetIcon(mime, bmp) != B_OK);	// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(mime.IsInstalled());
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
	}
	// NULL params
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		// Uninstalled
		CHK(helper.GetIcon(mime, NULL) != B_OK);	// B_BAD_VALUE
		CHK(!mime.IsInstalled());
		CHK(helper.SetIcon(mime, NULL) != B_OK);	// R5 == Installs, B_ENTRY_NOT_FOUND
													// Haiku == Doesn't install, B_ENTRY_NOT_FOUND
#if TEST_R5
		CHK(mime.IsInstalled());
#else
		CHK(!mime.IsInstalled());
		CHK(mime.Install() == B_OK);
#endif
		CHK(helper.GetIcon(mime, bmp) != B_OK);	// B_ENTRY_NOT_FOUND
		// Installed
		CHK(helper.GetIcon(mime, NULL) != B_OK);	// B_BAD_VALUE
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(helper.SetIcon(mime, NULL) == B_OK);
		CHK(helper.GetIcon(mime, bmp) != B_OK);	// B_ENTRY_NOT_FOUND
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(helper.DeleteIcon(mime) != B_OK);
		CHK(!mime.IsInstalled());
		CHK(helper.SetIcon(mime, helper.Bitmap2()) == B_OK);
		CHK(mime.IsInstalled());
		fill_bitmap(*bmp, 100);
		CHK(*bmp != *helper.Bitmap2());
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap2());
		CHK(helper.DeleteIcon(mime) == B_OK);
		CHK(mime.IsInstalled());
		CHK(helper.GetIcon(mime, bmp) != B_OK);
#endif
	}
	// Invalid Bitmap Size (small -- 10x10)
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Init Test Bitmap
		BBitmap testBmp(BRect(0,0,9,9), B_CMAP8);
		fill_bitmap(testBmp, 3);
		// Test Set()
		CHK(testBmp != *helper.Bitmap1());
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(helper.SetIcon(mime, &testBmp) != B_OK);	// R5 == B_BAD_VALUE
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(*bmp != testBmp);
		// Test Get()
		fill_bitmap(testBmp, 3);
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, &testBmp) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Invalid Bitmap Size (large -- 100x100)
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Init Test Bitmap
		BBitmap testBmp(BRect(0,0,99,99), B_CMAP8);
		// Test Set()
		fill_bitmap(testBmp, 3);
		CHK(testBmp != *helper.Bitmap1());
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(helper.SetIcon(mime, &testBmp) != B_OK);	// R5 == B_BAD_VALUE
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(*bmp != testBmp);
		// Test Get()
		fill_bitmap(testBmp, 3);
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, &testBmp) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Non-B_CMAP8 Color Depth (not really supported under R5)
	NextSubTest();
	{
#if !TEST_R5
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Init Test Bitmap
		BBitmap testBmp(helper.BitmapBounds(), B_RGB32);
		// Test Set()
//		fill_bitmap(testBmp, 4);
		fill_bitmap32(testBmp, 10, 20, 30, 40);				// Fill our 32-bit bitmap
		BBitmap testBmp8(helper.BitmapBounds(), B_CMAP8);	// Create an 8-bit bitmap the same size
//		dump_bitmap(testBmp8);
		reduce_color_depth(testBmp, testBmp8);				// Make it an 8-bit version of the 32-bit bitmap
//		dump_bitmap(testBmp8);
		CHK(testBmp != *helper.Bitmap1());
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		CHK(helper.SetIcon(mime, &testBmp) == B_OK);
			// R5 == B_OK, despite being invalid color depth; however, icon is not actually set,
			// and any subsequent call to GetIcon() will cause the application to crash...
		CHK(helper.GetIcon(mime, bmp) == B_OK);	// R5 == CRASH!, Haiku == Damn right I can handle that shit
		CHK(*bmp != *helper.Bitmap1());
		CHK(*bmp != testBmp);				// Shouldn't match, since SetIcon() reduces to B_CMAP8
		CHK(*bmp == testBmp8);				// *Should* match, since it's the result of a similar reduction
#endif
	}
	// Normal Function
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Set() then Get()
		fill_bitmap(*bmp, 3);
		CHK(*bmp != *helper.Bitmap1());
		CHK(helper.SetIcon(mime, helper.Bitmap1()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		// Set() then Get() again
		fill_bitmap(*bmp, 3);
		CHK(helper.SetIcon(mime, helper.Bitmap2()) == B_OK);
		CHK(helper.GetIcon(mime, bmp) == B_OK);
		CHK(*bmp == *helper.Bitmap2());
		CHK(*bmp != *helper.Bitmap1());
	}
}

// Icon For Type Helper Functions

void
MimeTypeTest::IconForTypeTest(IconForTypeHelper &helper) {
	IconTest(helper);	// First run all the icon tests
		// Then do some IconForType() specific tests

	BBitmap *bmp = helper.TempBitmap();

	// Invalid MIME string
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Set() then Get()
		fill_bitmap(*bmp, 3);
		CHK(*bmp != *helper.Bitmap1());
		CHK(mime.SetIconForType(testTypeInvalid, helper.Bitmap1(), helper.Size()) != B_OK);	// R5 == B_BAD_VALUE
		CHK(mime.GetIconForType(testTypeInvalid, bmp, helper.Size()) != B_OK);				// R5 == B_BAD_VALUE
		CHK(*bmp != *helper.Bitmap1());
	}
	// NULL MIME string (just like calling respective {Get,Set}Icon() function)
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		// Set() then Get()
		fill_bitmap(*bmp, 3);
		CHK(*bmp != *helper.Bitmap1());
		CHK(mime.SetIconForType(NULL, helper.Bitmap1(), helper.Size()) == B_OK);
		CHK(mime.GetIconForType(NULL, bmp, helper.Size()) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		// Verify GetIcon() does the same thing
		fill_bitmap(*bmp, 3);
		CHK(*bmp != *helper.Bitmap1());
		CHK(mime.GetIcon(bmp, helper.Size()) == B_OK);
		CHK(*bmp == *helper.Bitmap1());
		// Delete with dual NULL calls
		CHK(mime.SetIconForType(NULL, NULL, helper.Size()) == B_OK);
		CHK(mime.GetIconForType(NULL, bmp, helper.Size()) != B_OK);	// B_ENTRY_NOT_FOUND
		CHK(mime.GetIcon(bmp, helper.Size()) != B_OK);				// B_ENTRY_NOT_FOUND
	}
}

void
MimeTypeTest::LargeIconTest() {
	IconHelper helper(B_LARGE_ICON);
	IconTest(helper);
}

void
MimeTypeTest::MiniIconTest() {
	IconHelper helper(B_MINI_ICON);
	IconTest(helper);
}

void
MimeTypeTest::LargeIconForTypeTest() {
	IconForTypeHelper helper(testType, B_LARGE_ICON);
	IconForTypeTest(helper);
}

void
MimeTypeTest::MiniIconForTypeTest() {
	IconForTypeHelper helper(testType, B_MINI_ICON);
	IconForTypeTest(helper);
}

bool isMIMESupertype(const char *type) {
	BMimeType sub, super;
	status_t err;
	err = !type || !BMimeType::IsValid(type);

	// See if the type is the same as it's supertype
	if (!err)
		err = sub.SetTo(type);
	if (!err)
		return sub.GetSupertype(&super) == B_BAD_VALUE;
			// This is what R5::GetSupertype() returns when called on a supertype;

	return false;
}

void
MimeTypeTest::VerifyInstalledTypes() {
	// Check GetInstalledTypes(1)
	NextSubTest();
	{
		BMessage msg;

		// Get the list of installed types
		CHK(BMimeType::GetInstalledTypes(&msg) == B_OK);

		// Add all the type strings to a std::set
		std::set<std::string> typeSet;
		SetAdapter typeAdapter(typeSet);
		FillWithMimeTypes(typeAdapter, msg, "types");

		// Manually verify that the set of types returned by GetInstalledTypes()
		// and the types present in the database are exactly the same (ignoring
		// any files with names made of invalid characters, in case some bozo
		// manually added such a file :-)
		BDirectory rootDir(mimeDatabaseDir.c_str());
		BEntry superEntry;
		CHK(rootDir.InitCheck() == B_OK);
		rootDir.Rewind();
		while (true) {
			status_t err = rootDir.GetNextEntry(&superEntry);
			if (err == B_ENTRY_NOT_FOUND)
				break;	// End of directory listing

			CHK(!err);	// Any other error is unacceptable :-)

			// Get the leaf name
			char superLeafMixed[B_PATH_NAME_LENGTH+1];
			CHK(superEntry.GetName(superLeafMixed) == B_OK);
			std::string superLeaf;
			to_lower(superLeafMixed, superLeaf);

			// We're only interested in directories, as they map to
			// supertypes (and since they map thusly, they must also
			// be valid MIME strings)
			if (superEntry.IsDirectory() && BMimeType::IsValid(superLeaf.c_str())) {
				// First, find and remove the supertype from our set
				CHK(typeSet.find(superLeaf.c_str()) != typeSet.end());
				typeSet.erase(superLeaf.c_str());

				// Second, iterate through all the entries in the directory.
				// If the entry designates a valid MIME string, find it
				// in the set and remove it.
				BDirectory superDir(&superEntry);
				BEntry subEntry;
				CHK(superDir.InitCheck() == B_OK);
				superDir.Rewind();
				while (true) {
					status_t err = superDir.GetNextEntry(&subEntry);
					if (err == B_ENTRY_NOT_FOUND)
						break;	// End of directory listing

					CHK(!err);	// Any other error is unacceptable :-)

					// Get the leaf name
					char subLeafMixed[B_PATH_NAME_LENGTH+1];
					CHK(subEntry.GetName(subLeafMixed) == B_OK);
					std::string subLeaf;
					to_lower(subLeafMixed, subLeaf);

					// Verify it's a valid mime string. If so, find and remove from our set
					std::string subType = superLeaf + "/" + subLeaf;
					if (BMimeType::IsValid(subType.c_str())) {
						if (typeSet.find(subType.c_str()) == typeSet.end())
							cout << "Fuckup == '" << subType << "'" << endl;
						CHK(typeSet.find(subType.c_str()) != typeSet.end());
						typeSet.erase(subType.c_str());
					}
				}
			}
		}

		// At this point our set should be empty :-) If it's not, you might check
		// that you haven't added any superfluous files to your MIME database (like
		// a __mime_table backup, for instance).
		CHK(typeSet.size() == 0);
	}
	NextSubTest();
	// Check GetInstalledTypes(2) and GetInstalledSupertypes()
	{
		BMessage msg;

		// Get the list of installed types
		CHK(BMimeType::GetInstalledSupertypes(&msg) == B_OK);
//		msg.PrintToStream();

		// Add all the type strings to a std::set
		std::set<std::string> typeSet;
		SetAdapter typeAdapter(typeSet);
		FillWithMimeTypes(typeAdapter, msg, "super_types");

		// Manually verify that the set of types returned by GetInstalledSupertypes()
		// and the types present in the database are exactly the same (ignoring
		// any files with names made of invalid characters, in case some bozo
		// manually added such a file :-)
		BDirectory rootDir(mimeDatabaseDir.c_str());
		BEntry superEntry;
		CHK(rootDir.InitCheck() == B_OK);
		rootDir.Rewind();
		while (true) {
			status_t err = rootDir.GetNextEntry(&superEntry);
			if (err == B_ENTRY_NOT_FOUND)
				break;	// End of directory listing

			CHK(!err);	// Any other error is unacceptable :-)

			// Get the leaf name
			char superLeafMixed[B_PATH_NAME_LENGTH+1];
			CHK(superEntry.GetName(superLeafMixed) == B_OK);
			std::string superLeaf;
			to_lower(superLeafMixed, superLeaf);

			// We're only interested in directories, as they map to
			// supertypes (and since they map thusly, they must also
			// be valid MIME strings)
			if (superEntry.IsDirectory() && BMimeType::IsValid(superLeaf.c_str())) {
				// First, find and remove the supertype from our set
				CHK(typeSet.find(superLeaf.c_str()) != typeSet.end());
				typeSet.erase(superLeaf.c_str());

				// Second, get the list of corresponding subtypes and add them
				// to a std::set to be used for verification
				BMessage msg;
				CHK(BMimeType::GetInstalledTypes(superLeaf.c_str(), &msg) == B_OK);
//				msg.PrintToStream();

				std::set<std::string> subtypeSet;
				SetAdapter subtypeAdapter(subtypeSet);
				FillWithMimeTypes(subtypeAdapter, msg, "types");

				// Third, iterate through all the entries in the directory.
				// If the entry designates a valid MIME string, find it
				// in the subtype set and remove it.
				BDirectory superDir(&superEntry);
				BEntry subEntry;
				CHK(superDir.InitCheck() == B_OK);
				superDir.Rewind();
				while (true) {
					status_t err = superDir.GetNextEntry(&subEntry);
					if (err == B_ENTRY_NOT_FOUND)
						break;	// End of directory listing

					CHK(!err);	// Any other error is unacceptable :-)

					// Get the leaf name
					char subLeafMixed[B_PATH_NAME_LENGTH+1];
					CHK(subEntry.GetName(subLeafMixed) == B_OK);
					std::string subLeaf;
					to_lower(subLeafMixed, subLeaf);

					// Verify it's a valid mime string. If so, find and remove from our set
					std::string subType = superLeaf + "/" + subLeaf;
					if (BMimeType::IsValid(subType.c_str())) {
						CHK(subtypeSet.find(subType.c_str()) != subtypeSet.end());
						subtypeSet.erase(subType.c_str());
					}
				}

				// At this point our subtype set should be empty :-)
				CHK(subtypeSet.size() == 0);

			}
		}

		// At this point our set should be empty :-)
		CHK(typeSet.size() == 0);
	}

}

void
MimeTypeTest::InstalledTypesTest() {
	// NULL params
	{
		BMessage msg;
		NextSubTest();

#if !TEST_R5
		CHK(BMimeType::GetInstalledTypes(NULL) != B_OK);			// R5 == CRASH!!!, Haiku == B_BAD_VALUE
#endif
		NextSubTest();
#if !TEST_R5
		CHK(BMimeType::GetInstalledTypes("text", NULL) != B_OK);	// R5 == CRASH!!!, Haiku == B_BAD_VALUE
#endif
		NextSubTest();
		CHK(BMimeType::GetInstalledTypes(NULL, &msg) == B_OK);		// Same as GetInstalledTypes(&msg)
// 		msg.PrintToStream();
		NextSubTest();
#if !TEST_R5
		CHK(BMimeType::GetInstalledTypes(NULL, NULL) != B_OK);		// R5 == CRASH!!!, Haiku == B_BAD_VALUE
#endif
		NextSubTest();
#if !TEST_R5
		CHK(BMimeType::GetInstalledSupertypes(NULL) != B_OK);		// R5 == CRASH!!!, Haiku == B_BAD_VALUE
#endif
	}
	// Invalid supertype param to GetInstalledTypes(char *super, BMessage*)
	{
		BMessage msg;
		NextSubTest();
		CHK(!BMimeType::IsValid(testTypeSuperInvalid));
		CHK(BMimeType::GetInstalledTypes(testTypeSuperInvalid, &msg) != B_OK);			// R5 == B_BAD_VALUE
		NextSubTest();
		CHK(BMimeType::IsValid(testTypeSuperValid));
		CHK(BMimeType::GetInstalledTypes(testTypeSuperValid, &msg) != B_OK);	// R5 == B_ENTRY_NOT_FOUND
	}
	// Normal Function -- GetInstalledTypes(BMessage*)
	// This test gets the list of installed types, then iterates through
	// the actual database directory listings and verifies they're identical.
	{
		VerifyInstalledTypes();
		BMimeType mime(testTypeApp1);
		CHK(mime.InitCheck() == B_OK);
		if (mime.IsInstalled()) {
			CHK(mime.Delete() == B_OK);
			VerifyInstalledTypes();
			CHK(mime.Install() == B_OK);
			VerifyInstalledTypes();
		} else {
			CHK(mime.Install() == B_OK);
			VerifyInstalledTypes();
			CHK(mime.Delete() == B_OK);
			VerifyInstalledTypes();
		}
	}
	// Normal Function -- GetInstalledSupertypes()/GetInstalledTypes(char*,BMessage*)
	// This test gets the list of installed super types, then iterates through
	// the actual database directory listings and verifies they're identical.

}

// Short Description

void
MimeTypeTest::ShortDescriptionTest() {
	DescriptionTest(&BMimeType::GetShortDescription, &BMimeType::SetShortDescription,
#if TEST_R5
					   NULL
#else
					   &BMimeType::DeleteShortDescription
#endif
	);
}

// Long Description

void
MimeTypeTest::LongDescriptionTest() {
	DescriptionTest(&BMimeType::GetLongDescription, &BMimeType::SetLongDescription,
#if TEST_R5
					   NULL
#else
					   &BMimeType::DeleteLongDescription
#endif
	);
}

// DescriptionTest Helper Function
void
MimeTypeTest::DescriptionTest(GetDescriptionFunc getDescr, SetDescriptionFunc setDescr,
							    DeleteDescriptionFunc deleteDescr)
{
	char str[B_MIME_TYPE_LENGTH+1];

	// Uninitialized
	NextSubTest();
	{
		str[0] = 0;
		BMimeType mime;
		CPPUNIT_ASSERT(mime.InitCheck() == B_NO_INIT);
		CPPUNIT_ASSERT((mime.*getDescr)(str) != B_OK);	// R5 == B_BAD_VALUE
		CPPUNIT_ASSERT((mime.*setDescr)(str) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Non-installed type
	NextSubTest();
	{
		str[0] = 0;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK((mime.*getDescr)(str) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK((mime.*setDescr)(testDescr) == B_OK);	// R5 == Installs (but doesn't set), B_OK
		CHK(mime.IsInstalled());
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr) == 0);
	}
	// Non-installed type, NULL params
	NextSubTest();
	{
#if !TEST_R5	// NOTE: These tests crash for R5::LongDescription calls but not for R5::ShortDescription
				// calls. Considering the general instability exihibited by most R5 calls when passed
				// NULL pointers, however, I wouldn't suggest it, and thus they aren't even tested here.
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK((mime.*getDescr)(NULL) == B_BAD_VALUE);
		CHK(!mime.IsInstalled());
		CHK((mime.*setDescr)(NULL) == B_ENTRY_NOT_FOUND);	// Trying to delete non-existent attribute
		CHK(!mime.IsInstalled());
		CHK((mime.*setDescr)(testDescr) == B_OK);
		CHK(mime.IsInstalled());
		str[0] = 0;
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr) == 0);
		CHK((mime.*setDescr)(NULL) == B_OK);	// Delete the attribute
		CHK(mime.IsInstalled());
		CHK((mime.*getDescr)(str) == B_ENTRY_NOT_FOUND);
#endif
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK((mime.*deleteDescr)() != B_OK);
		CHK(!mime.IsInstalled());
		CHK((mime.*setDescr)(testDescr) == B_OK);
		CHK(mime.IsInstalled());
		str[0] = 0;
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr) == 0);
		CHK((mime.*deleteDescr)() == B_OK);
		CHK(mime.IsInstalled());
		CHK((mime.*getDescr)(str) != B_OK);
#endif
	}
	// Installed type
	NextSubTest();
	{
		str[0] = 0;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		// Get() with no description installed
		CHK(mime.IsInstalled());
		CHK((mime.*getDescr)(str) == B_ENTRY_NOT_FOUND);	// R5 == B_ENTRY_NOT_FOUND
		// Initial Set()/Get()
		CHK((mime.*setDescr)(testDescr) == B_OK);
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr) == 0);
		// Followup Set()/Get()
		CHK((mime.*setDescr)(testDescr2) == B_OK);
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr2) == 0);
	}
	// Installed Type, Description Too Long
	NextSubTest();
	{
		str[0] = 0;
		CHK(strlen(longDescr) > (B_MIME_TYPE_LENGTH+1));
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		// Initial Set()/Get()
		CHK((mime.*setDescr)(longDescr) != B_OK);		// R5 == B_BAD_VALUE
		CHK((mime.*getDescr)(str) == B_ENTRY_NOT_FOUND);
		// Followup Set()/Get()
		CHK((mime.*setDescr)(testDescr) == B_OK);
		CHK((mime.*setDescr)(longDescr) != B_OK);		// R5 == B_BAD_VALUE
		CHK((mime.*getDescr)(str) == B_OK);
		CHK(strcmp(str, testDescr) == 0);
	}

}


// Preferred App

void
MimeTypeTest::PreferredAppTest() {
	char str[B_MIME_TYPE_LENGTH+1];
	sprintf(str, "%s", testSig);

	// Uninitialized
	NextSubTest();
	{
		BMimeType mime;
		CPPUNIT_ASSERT(mime.InitCheck() == B_NO_INIT);
		CPPUNIT_ASSERT(mime.GetPreferredApp(str) != B_OK);	// R5 == B_BAD_VALUE
		CPPUNIT_ASSERT(mime.SetPreferredApp(str) != B_OK);	// R5 == B_BAD_VALUE
	}
	// Non-installed type
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.GetPreferredApp(str) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetPreferredApp(testSig) == B_OK);	// R5 == Installs (but doesn't set), B_OK
		CHK(mime.IsInstalled());
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig) == 0);
	}
	// Non-installed type, NULL params
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
#if TEST_R5
		CHK(!mime.IsInstalled());
		CHK(mime.GetPreferredApp(NULL) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetPreferredApp(NULL) != B_OK);		// R5 == Installs (but doesn't set), B_ENTRY_NOT_FOUND
		CHK(mime.IsInstalled());
		CHK(mime.GetPreferredApp(str) == B_ENTRY_NOT_FOUND);
#else
		CHK(!mime.IsInstalled());
		CHK(mime.GetPreferredApp(NULL) != B_OK);		// Haiku == B_BAD_VALUE
		CHK(!mime.IsInstalled());
		CHK(mime.SetPreferredApp(NULL) != B_OK);		// Haiku == B_ENTRY_NOT_FOUND
		CHK(!mime.IsInstalled());
		CHK(mime.SetPreferredApp(testSig) == B_OK);
		CHK(mime.IsInstalled());
		str[0] = 0;
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig) == 0);
		CHK(mime.SetPreferredApp(NULL) == B_OK);
		CHK(mime.IsInstalled());
		str[0] = 0;
		CHK(mime.GetPreferredApp(str) != B_OK);			// Haiku == B_ENTRY_NOT_FOUND
#endif // !TEST_R5
	}
	// Installed type, NULL params
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetPreferredApp(NULL) != B_OK);		// R5 == B_BAD_ADDRESS
		CHK(mime.SetPreferredApp(NULL) != B_OK);		// R5 == B_ENTRY_NOT_FOUND
		CHK(mime.GetPreferredApp(NULL) != B_OK);		// R5 == B_BAD_ADDRESS
	}
	// Delete test
	NextSubTest();
	{
#if !TEST_R5
		entry_ref ref;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Make sure the type isn't installed
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.DeletePreferredApp() != B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.SetPreferredApp(testSig) == B_OK);
		CHK(mime.IsInstalled());
		str[0] = 0;
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig) == 0);
		CHK(mime.DeletePreferredApp() == B_OK);
		CHK(mime.IsInstalled());
		CHK(mime.GetPreferredApp(str) != B_OK);
#endif
	}
	// Installed type
	NextSubTest();
	{
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		// Get() with no description installed
		CHK(mime.IsInstalled());
		CHK(mime.GetPreferredApp(str) == B_ENTRY_NOT_FOUND);	// R5 == B_ENTRY_NOT_FOUND
		// Initial Set()/Get()
		CHK(mime.SetPreferredApp(testSig) == B_OK);
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig) == 0);
		// Followup Set()/Get()
		CHK(mime.SetPreferredApp(testSig2) == B_OK);
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig2) == 0);
	}
	// Installed Type, Signature Too Long
	NextSubTest();
	{
		CHK(strlen(longDescr) > (B_MIME_TYPE_LENGTH+1));
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		// Uninstall then reinstall to clear attributes
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		// Initial Set()/Get()
		CHK(mime.SetPreferredApp(longSig) != B_OK);		// R5 == B_BAD_VALUE
		CHK(mime.GetPreferredApp(str) == B_ENTRY_NOT_FOUND);
		// Followup Set()/Get()
		CHK(mime.SetPreferredApp(testSig) == B_OK);
		CHK(mime.SetPreferredApp(longSig) != B_OK);		// R5 == B_BAD_VALUE
		CHK(mime.GetPreferredApp(str) == B_OK);
		CHK(strcmp(str, testSig) == 0);
	}

}

// Converts every character in str to lowercase and places
// the result in result.
void
to_lower(const char *str, std::string &result) {
	CHK(str != NULL);
	result = "";
	for (uint i = 0; i < strlen(str); i++)
		result += tolower(str[i]);
}

std::string
to_lower(const char *str) {
	std::string result;
	to_lower(str, result);
	return result;
}


// Manually removes the file in the MIME database corresponding to
// the given MIME type
void
remove_type(const char *type, const char *databaseDir) {
	CHK(type != NULL);

	// Since the MIME types are converted to lower case before their
	// corresponding file is created in the database, we need to do
	// the same
	std::string typeLower;
	to_lower(type, typeLower);

	BEntry entry((std::string(mimeDatabaseDir) + "/" + typeLower).c_str());
	CHK(entry.InitCheck() == B_OK);
	if (entry.Exists())
		CHK(entry.Remove() == B_OK);
	CHK(!entry.Exists());
}

// Manually verifies that the file in the MIME database corresponding to
// the given MIME type exists
bool
type_exists(const char *type, const char *databaseDir) {
	CHK(type != NULL);

	// Since the MIME types are converted to lower case before their
	// corresponding file is created in the database, we need to do
	// the same
	std::string typeLower;
	to_lower(type, typeLower);

	BEntry entry((std::string(databaseDir) + "/" + typeLower).c_str());
	CHK(entry.InitCheck() == B_OK);
	return entry.Exists();
}

void
MimeTypeTest::InstallDeleteTest() {
	// Uninitialzized
	NextSubTest();
	{
		BMimeType mime;
		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(!mime.IsInstalled());
		CHK(mime.Install() != B_OK);	// R5 == B_BAD_VALUE
		CHK(mime.Delete() != B_OK);	// R5 == B_BAD_VALUE
	}
	// Invalid Type String
	NextSubTest();
	{
		BMimeType mime(testTypeInvalid);
		CHK(mime.InitCheck() != B_OK);	// R5 == B_BAD_VALUE
		CHK(!mime.IsInstalled());
		CHK(mime.Install() != B_OK);	// R5 == B_BAD_VALUE
		CHK(mime.Delete() != B_OK);	// R5 == B_BAD_VALUE
	}
	// Normal function
	NextSubTest();
	{
		remove_type(testType);
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		CHK(!mime.IsInstalled());
		CHK(mime.Delete() != B_OK);	// R5 == B_ENTRY_NOT_FOUND, Haiku == B_ENTRY_NOT_FOUND
		CHK(!type_exists(testType));
		CHK(mime.Install() == B_OK);
		CHK(type_exists(testType));
		CHK(mime.IsInstalled());
#if !TEST_R5
		CHK(mime.Install() != B_OK);	// We ought to return something standard and logical here, as R5 is random
#endif
		CHK(mime.Delete() == B_OK);
		CHK(!type_exists(testType));
		CHK(!mime.IsInstalled());
	}

}

void FillWithMimeTypes(ContainerAdapter &container, BMessage &typeMessage, const char* fieldName) {
	type_code type;
	int32 count;
	status_t err;

//	typeMessage.PrintToStream();

	// Get a count of types in the message
	err = typeMessage.GetInfo(fieldName, &type, &count);
	if (err == B_NAME_NOT_FOUND)
		count = 0;			// No such types installed in the database! :-)
	else
		CHK(err == B_OK);	// Any other error is unacceptable

	// Add them all to the container, after converting to lowercase and
	// checking validity
	for (int i = 0; i < count; i++) {
		char *str;
		CHK(typeMessage.FindString(fieldName, i, (const char**)&str) == B_OK);
		std::string strLower;
		to_lower(str, strLower);
		// Make sure it's a valid type string, since the R5::GetInstalled*Types()
		// functions do no such verification, and we ignore invalid type files
		// in the database.
		if (BMimeType::IsValid(strLower.c_str()))
			container.Add(strLower);
	}
}

bool
types_fields_are_identical(const BMessage &msg1, const BMessage &msg2)
{
	const char *str1;
	const char *str2;
	int i = 0;
	bool result = true;
	while (true) {
		status_t err1 = msg1.FindString(typesField, i, &str1);
		status_t err2 = msg2.FindString(typesField, i, &str2);
		if (err1 != err2) {
			result = false;
			break;
		}
		if (err1 != B_OK)
			break;
		result &= to_lower(str1) == to_lower(str2);
		i++;
	}
	return result;
}

bool
is_supporting_app_for_all_types_in_message(const char *app, const BMessage &msg)
{
	const char *str;
	bool result = true;
	for (int i = 0; msg.FindString(typesField, i, &str) == B_OK; i++) {
		BMimeType supportedType(str);
		BMessage appMsg;

		// Get a list of supporting apps
		CHK(supportedType.InitCheck() == B_OK);
		CHK(supportedType.GetSupportingApps(&appMsg) == B_OK);
//		cout << "-----------------------------------------------------------" << endl;
//		cout << str << endl;
//		cout << "-----------------------------------------------------------" << endl;
//		appMsg.PrintToStream();

		// Look for our supporting app
		int32 directlySupportingAppsCount;
		CHK(appMsg.FindInt32("be:sub", &directlySupportingAppsCount) == B_OK);
		bool foundType = false;
		const char *supportingApp;
		for (int j = 0;
			   j < directlySupportingAppsCount
			     && appMsg.FindString(applicationsField, &supportingApp) == B_OK;
			       j++)
		{
			foundType |= to_lower(app) == to_lower(supportingApp);
		}
		result &= foundType;
	}
	return result;
}

void
MimeTypeTest::SupportedTypesTest() {
#if TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#else
	// Create some messages to sling around
	const int32 WHAT = 0;
	BMessage msg1a(WHAT), msg1b(WHAT), msg2(WHAT), msg3(WHAT), msgEmpty(WHAT);

	CHK(msg3.AddString(typesField, testType1) == B_OK);
	CHK(msg3.AddString(typesField, testType2) == B_OK);
	CHK(msg3.AddString(typesField, testType3) == B_OK);

	CHK(msg2.AddString(typesField, testType2) == B_OK);
	CHK(msg2.AddString(typesField, testType3) == B_OK);

	CHK(msg1a.AddString(typesField, testType5) == B_OK);

	CHK(msg1b.AddString(typesField, testType2) == B_OK);

	CHK(msg1a == msg1a);
	CHK(msg1b == msg1b);
	CHK(msg2 == msg2);
	CHK(msg3 == msg3);
	CHK(msg1a != msg2);
	CHK(msg1a != msg3);
	CHK(msg1a != msgEmpty);

	// Uninitialized
	NextSubTest();
	{
		BMimeType mime;
		BMessage msg;

		CHK(mime.InitCheck() == B_NO_INIT);
		CHK(mime.SetSupportedTypes(&msg, true) != B_OK);
		CHK(mime.SetSupportedTypes(&msg, false) != B_OK);
		CHK(mime.GetSupportedTypes(&msg) != B_OK);
		CHK(mime.DeleteSupportedTypes() != B_OK);
	}

	// Test that deleting a type from the database also removes
	// the app as a supporting app for all types it previously
	// supported
	NextSubTest();
	{
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);

		// Set a list of supported types
		CHK(mime.SetSupportedTypes(&msg3, true) == B_OK);

		// Verify that each of those types now lists the
		// type as a directly supporting app
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == true);

		// Delete the type
		CHK(mime.Delete() == B_OK);

		// Verify that each of those types no longer lists the
		// type as a directly supporting app
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == false);
	}

	// Test that SetSupportedTypes(..., false) does not remove the app as a supporting
	// app for newly unsupported types, while SetSupportedTypes(..., true) does. Also
	// test that supported types stranded by multiple sequential calls to
	// SetSupportedTypes(..., false) are properly updated so as to no longer list the
	// app as a supporting app once SetSupportedTypes(..., true) is finally called.
	NextSubTest();
	{
		BMessage msg;
		BMimeType mime(testType);
		CHK(mime.InitCheck() == B_OK);

		// Uninstall then reinstall to clear attributes, etc
		if (mime.IsInstalled())
			CHK(mime.Delete() == B_OK);
		if (!mime.IsInstalled())
			CHK(mime.Install() == B_OK);
		CHK(mime.IsInstalled());

		// Set a list of supported types
		CHK(mime.SetSupportedTypes(&msg3, true) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_OK);
		CHK(types_fields_are_identical(msg3, msg) == true);
		CHK(types_fields_are_identical(msg2, msg) == false);
		CHK(types_fields_are_identical(msg1a, msg) == false);
		CHK(types_fields_are_identical(msg1b, msg) == false);
		CHK(types_fields_are_identical(msgEmpty, msg) == false);

		// Verify that each of those types now lists the
		// type as a directly supporting app
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Set (no sync) to a new list of supported types containing one
		// fewer type than the original list
		CHK(mime.SetSupportedTypes(&msg2, false) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_OK);
		CHK(types_fields_are_identical(msg3, msg) == false);
		CHK(types_fields_are_identical(msg2, msg) == true);
		CHK(types_fields_are_identical(msg1a, msg) == false);
		CHK(types_fields_are_identical(msg1b, msg) == false);
		CHK(types_fields_are_identical(msgEmpty, msg) == false);

		// Verify that the app is still listed as a supporting app for
		// *all* of the originally supported types (even the one no longer
		// listed as being supported)
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Set (no sync) to a new list of supported types containing an
		// entirely new, never supported type
		CHK(mime.SetSupportedTypes(&msg1a, false) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_OK);
		CHK(types_fields_are_identical(msg3, msg) == false);
		CHK(types_fields_are_identical(msg2, msg) == false);
		CHK(types_fields_are_identical(msg1a, msg) == true);
		CHK(types_fields_are_identical(msg1b, msg) == false);
		CHK(types_fields_are_identical(msgEmpty, msg) == false);

		// Verify that the app is still listed as a supporting app for
		// *all* of the originally supported types (none of which are
		// supported any longer) as well as the newly supported type.
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Set (no sync) to a new list of supported types containing only
		// one of the originally supported types that had been previously
		// removed.
		CHK(mime.SetSupportedTypes(&msg1b, false) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_OK);
		CHK(types_fields_are_identical(msg3, msg) == false);
		CHK(types_fields_are_identical(msg2, msg) == false);
		CHK(types_fields_are_identical(msg1a, msg) == false);
		CHK(types_fields_are_identical(msg1b, msg) == true);
		CHK(types_fields_are_identical(msgEmpty, msg) == false);

		// Verify that the app is still listed as a supporting app for
		// *all* of the originally supported types (only one of which is
		// supported any longer) as well as the previous supported type.
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == true);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Set (with sync) to the same list as last time (containing the
		// one type from the original list of supported types)
		CHK(mime.SetSupportedTypes(&msg1b, true) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_OK);
		CHK(types_fields_are_identical(msg3, msg) == false);
		CHK(types_fields_are_identical(msg2, msg) == false);
		CHK(types_fields_are_identical(msg1a, msg) == false);
		CHK(types_fields_are_identical(msg1b, msg) == true);
		CHK(types_fields_are_identical(msgEmpty, msg) == false);

		// Verify that the app is now only listed as a supporting app for the
		// most recently supported type.
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Test SetSupportedTypes(NULL, false) for shits and giggles
		CHK(mime.SetSupportedTypes(NULL, false) == B_OK);
		CHK(mime.GetSupportedTypes(&msg) == B_ENTRY_NOT_FOUND);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == true);

		// Now test that SetSupportedTypes(NULL, true) updates the supporting
		// apps mappings, even if the supported types attribute has already
		// been removed.
		CHK(mime.SetSupportedTypes(NULL, true) == B_ENTRY_NOT_FOUND);
		CHK(mime.GetSupportedTypes(&msg) == B_ENTRY_NOT_FOUND);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg3) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg2) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1a) == false);
		CHK(is_supporting_app_for_all_types_in_message(testType, msg1b) == false);
	}
#endif	// #if TEST_R5 else
}

void
MimeTypeTest::SupportingAppsTest() {
/*	{
		BMessage msg;
		BMimeType::GetInstalledTypes(&msg);
		msg.PrintToStream();
	}
	{
		BMessage msg;
		BMimeType mime("application/octet-stream");
		CHK(mime.InitCheck() == B_OK);
		CHK(mime.GetSupportingApps(&msg) == B_OK);
		msg.PrintToStream();
	}
	{
		BMessage msg;
		BMimeType mime("text");
		CHK(mime.InitCheck() == B_OK);
		CHK(mime.GetSupportingApps(&msg) == B_OK);
		msg.PrintToStream();
	}
	{
		BMessage msg;
		BMimeType mime("text/html");
		CHK(mime.InitCheck() == B_OK);
		CHK(mime.GetSupportingApps(&msg) == B_OK);
		msg.PrintToStream();
	} */
	NextSubTest();
	if (true)
	{
		std::set<std::string> typeList;							// Stores all installed MIME types
		std::set<std::string> appList;							// Stores all installed application subtypes
		std::map< std::string, std::set<std::string> > typeAppMap;	// Stores mapping of types to apps that support them
		std::map< std::string, std::set<std::string> > fakeTypeAppMap;	// Used to keep timing info for R5 and Haiku tests orthogonal

		// Get a list of all the types in the database
		{
			BMessage msg;
			CHK(BMimeType::GetInstalledTypes(&msg) == B_OK);
			SetAdapter typeAdapter(typeList);
			FillWithMimeTypes(typeAdapter, msg, "types");
		}

		// Get a list of all the apps in the database
		{
			BMessage msg;
			CHK(BMimeType::GetInstalledTypes(applicationSupertype, &msg) == B_OK);
			SetAdapter appAdapter(appList);
			FillWithMimeTypes(appAdapter, msg, "types");
		}

		// For each app in the database, manually get a list of the MIME types
		// it supports by reading its META:FILE_TYPES attribute from the database,
		// and add the app to the type->app map for each such type
		{
			std::set<std::string>::iterator i;
			for (i = appList.begin(); i != appList.end(); i++) {
				// Grab the next application
				std::string app = *i;

				// The leaf is all we're interested in -- it's the subtype
//				CHK(StorageKit::split_path(app.c_str(), dir, leaf) == B_OK);
				std::string appFile = std::string(mimeDatabaseDir) + "/" + app;
//				printf("'%s'\n", appFile.c_str());
				BNode node(appFile.c_str());
				CHK(node.InitCheck() == B_OK);

				// Find out how much data there is in the META:FILE_TYPES attribute
				// (assuming it even exists, which it may not...)
				attr_info info;
				if (node.GetAttrInfo("META:FILE_TYPES", &info) == B_OK) {
//					printf("attr_info: type == %lx, size == %lld\n", info.type, info.size);
					// Attribute exists, so alloc a buffer and read it
					char *buffer = new char[info.size+1];
					CHK(node.ReadAttr("META:FILE_TYPES", B_MESSAGE_TYPE, 0, buffer, info.size) == info.size);
					BMessage msg;
					if (msg.Unflatten(buffer) == B_OK) {
//						msg.PrintToStream();

						// Fill up a list with all the supported types
						std::set<std::string> supportList;
						SetAdapter supportAdapter(supportList);
						FillWithMimeTypes(supportAdapter, msg, "types");

						// For each type, add the current application as a supporting
						// app in our type->apps map
						for (std::set<std::string>::iterator type = supportList.begin();
								type != supportList.end();
									type++)
						{
							NextSubTest();
							typeAppMap[*type].insert(app);
							fakeTypeAppMap[*type].insert(app);
						}
					} else {
						// Just in case some bozo writes something other than a flattened
						// BMessage to the META:FILE_TYPES attribute, we'll only issue a
						// warning when the BMessage can't unflatten itself
						printf("Warning: Unable to unflatten META:FILE_TYPES attribute for '%s' type.\n",
							app.c_str());
					}

					delete buffer;

				}
			}
		}

//#if !TEST_R5
		// Now, add in all the types listed in MIME_DB_DIR/__mime_table
		{
			BEntry entry((std::string(mimeDatabaseDir) + "/__mime_table").c_str());
			CHK(entry.InitCheck() == B_OK);
			if (entry.Exists()) {
				BFile file(&entry, B_READ_ONLY);
				CHK(file.InitCheck() == B_OK);
				BMessage msg;
				CHK(msg.Unflatten(&file) == B_OK);

//				msg.PrintToStream();

				char *type;
				uint32 typeVal;
				int32 count;
				for (int i = 0; msg.GetInfo(B_STRING_TYPE, i, &type, &typeVal, &count) == B_OK; i++ ) {
					// Add all the associated applications. Interestingly (or maybe not),
					// any types appearing ONLY in the __mime_table and not in the mime
					// database fail when GetSupportingApps is called on them. Thus, we
					// add them to the type->app map, but not to the list of types.
					const char *app;
					for (int j = 0; j < count; j++) {
						CHK(msg.FindString(type, j, &app) == B_OK);
#if TEST_R5
						typeAppMap[type].insert(to_lower(app));
#else
						fakeTypeAppMap[type].insert(to_lower(app));
#endif
					}
				}
			}
		}
//#endif

		// For each installed type, get a list of the supported apps, and
		// verify that the list matches the list we generated. Also check
		// that the list of apps for the type's supertype (if it exists)
		// is a subset of the list we generated for said supertype.
		for (std::set<std::string>::iterator i = typeList.begin(); i != typeList.end(); i++) {
			// Get the current type
			std::string type = *i;
			BMimeType mime(type.c_str());
			CHK(mime.InitCheck() == B_OK);
//			printf("------------------------------------------------------------\n");
//			printf("%s\n", type.c_str());

			// Get the set of supporting apps for this type (and its supertype, if
			// it's not a supertype itself) that we discovered by manually culling
			// the database
			std::set<std::string> appSetSuper;
			BMimeType superType;
			if (mime.GetSupertype(&superType) == B_OK)
				appSetSuper = typeAppMap[superType.Type()];		// Copy the supertype
/*
			printf("sub.size == %ld\n", appSet.size());
			std::set<std::string>::iterator i;
			for (i = appSet.begin(); i != appSet.end(); i++) {
				printf("  %s\n", (*i).c_str());
			}
			printf("super.size == %ld\n", appSetSuper.size());
			for (i = appSetSuper.begin(); i != appSetSuper.end(); i++) {
				printf("  %s\n", (*i).c_str());
			}
			char* str;
			if (mime.GetPreferredApp(str) == B_OK) {
				printf("preferred app:\n");
				printf("  %s\n", str);
			}
*/
			// Get the set of supporting apps via GetSupportingApps(), then
			// add them to a list.
			BMessage msg;
			CHK(mime.GetSupportingApps(&msg) == B_OK);
			std::queue<std::string> appList;
			QueueAdapter appAdapter(appList);
			FillWithMimeTypes(appAdapter, msg, "applications");

//			msg.PrintToStream();


		}
	}
}

void
MimeTypeTest::WildcardAppsTest() {
	// NULL param
	NextSubTest();
	{
#if TEST_R5
		CHK(BMimeType::GetWildcardApps(NULL) == B_OK);			// R5 == B_OK (???)
#else
		CHK(BMimeType::GetWildcardApps(NULL) == B_BAD_VALUE);
#endif
	}
	// Normal function (compare to BMimeType("application/octet-stream").GetSupportingApps())
	NextSubTest();
	{
		BMessage msg1, msg2;
		CHK(BMimeType::GetWildcardApps(&msg1) == B_OK);
		BMimeType mime(wildcardType);
		CHK(mime.InitCheck() == B_OK);
		CHK(mime.GetSupportingApps(&msg2) == B_OK);
		CHK(msg1 == msg2);
	}
}


// init_long_types
static
void
init_long_types(char *notTooLongType, char *tooLongType)
{
// R5: Allows buffer sizes up to `B_MIME_TYPE_LENGTH + 1'
// Haiku: We stay consistent: `*_LENGTH' defines the buffer size.
#ifdef TEST_R5
	const int notTooLongLength = B_MIME_TYPE_LENGTH;
#else
	const int notTooLongLength = B_MIME_TYPE_LENGTH - 1;
#endif
	const int tooLongLength = notTooLongLength + 1;
	strcpy(notTooLongType, "image/");
	memset(notTooLongType + strlen(notTooLongType), 'a',
		   notTooLongLength - strlen(notTooLongType));
	notTooLongType[notTooLongLength] = '\0';
	strcpy(tooLongType, "image/");
	memset(tooLongType + strlen(tooLongType), 'a',
		   tooLongLength - strlen(tooLongType));
	tooLongType[tooLongLength] = '\0';
}

// InitTest
void
MimeTypeTest::InitTest()
{
	// tests:
	// * constructors
	// * SetTo(), SetType()
	// * Unset()
	// * InitCheck()
	// (* Type())

	// We test only a few types here. Exhausting testing is done in
	// ValidityTest().
	const char *validType	= "image/gif";
	const char *validType2	= "application/octet-stream";
	const char *invalidType	= "invalid type";
	char notTooLongType[B_MIME_TYPE_LENGTH + 3];
	char tooLongType[B_MIME_TYPE_LENGTH + 3];
	init_long_types(notTooLongType, tooLongType);

	// default constructor
	NextSubTest();
	{
		BMimeType type;
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}

	// BMimeType(const char *)
	// valid type
	NextSubTest();
	{
		BMimeType type(validType);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == validType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// invalid type
	NextSubTest();
	{
		BMimeType type(invalidType);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// long, but not too long type
	NextSubTest();
	{
		BMimeType type(notTooLongType);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == notTooLongType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// too long type
	NextSubTest();
	{
		BMimeType type(tooLongType);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}

	// SetTo()
	// valid type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetTo(validType) == B_OK);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == validType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// invalid type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetTo(invalidType) == B_BAD_VALUE);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// long, but not too long type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetTo(notTooLongType) == B_OK);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == notTooLongType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// too long type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetTo(tooLongType) == B_BAD_VALUE);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}

	// SetType()
	// valid type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetType(validType) == B_OK);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == validType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// invalid type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetType(invalidType) == B_BAD_VALUE);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// long, but not too long type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetType(notTooLongType) == B_OK);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == notTooLongType);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}
	// too long type
	NextSubTest();
	{
		BMimeType type;
		CHK(type.SetType(tooLongType) == B_BAD_VALUE);
		CHK(type.InitCheck() == B_BAD_VALUE);
		CHK(type.Type() == NULL);
		type.Unset();
		CHK(type.InitCheck() == B_NO_INIT);
		CHK(type.Type() == NULL);
	}

	// reinitialization
	NextSubTest();
	{
		BMimeType type(validType);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == validType);
		CHK(type.SetTo(validType2) == B_OK);
		CHK(type.InitCheck() == B_OK);
		CHK(string(type.Type()) == validType2);
	}
	// bad args
	NextSubTest();
	{
		BMimeType type(NULL);
		CHK(type.Type() == NULL);
		CHK(type.InitCheck() != B_OK);		// R5 == B_NO_INIT, Haiku == B_BAD_VALUE
		CHK(type.Type() == NULL);
		CHK(type.SetTo(NULL) != B_OK);		// R5 == B_NO_INIT, Haiku == B_BAD_VALUE
		CHK(type.Type() == NULL);
		CHK(type.SetType(NULL) != B_OK);	// R5 == B_NO_INIT, Haiku == B_BAD_VALUE
		CHK(type.Type() == NULL);
	}
}

// StringTest
void
MimeTypeTest::StringTest()
{
	// tests:
	// * IsValid() (static/non static)
	// * Type()
	// * IsSupertypeOnly()
	// * GetSupertype()
	// * Contains()
	// * operator==()

	char notTooLongType[B_MIME_TYPE_LENGTH + 3];
	char tooLongType[B_MIME_TYPE_LENGTH + 3];
	init_long_types(notTooLongType, tooLongType);
	struct mime_type_test {
		const char	*type;
		bool		super_type;
		status_t	error;
	};
	mime_type_test tests[] = {
		// valid types
		{ "application",					true,	B_OK, },
		{ "application/octet-stream",		false,	B_OK, },
		{ "audio",							true,	B_OK, },
		{ "audio/x-aiff",					false,	B_OK, },
		{ "image",							true,	B_OK, },
		{ "image/gif",						false,	B_OK, },
		{ "message",						true,	B_OK, },
		{ "message/rfc822",					false,	B_OK, },
		{ "multipart",						true,	B_OK, },
		{ "multipart/mixed",				false,	B_OK, },
		{ "text",							true,	B_OK, },
		{ "text/plain",						false,	B_OK, },
		{ "video",							true,	B_OK, },
		{ "video/x-msvideo",				false,	B_OK, },
		{ "unknown",						true,	B_OK, },
		{ "unknown/mime-type",				false,	B_OK, },
		{ "$%&./`'~*+#|!^",					false,	B_OK, },
		// invalid types
		{ "",								false,	B_BAD_VALUE, },
		{ "application/",					false,	B_BAD_VALUE, },
		{ "audio/",							false,	B_BAD_VALUE, },
		{ "image/",							false,	B_BAD_VALUE, },
		{ "message/",						false,	B_BAD_VALUE, },
		{ "multipart/",						false,	B_BAD_VALUE, },
		{ "text/",							false,	B_BAD_VALUE, },
		{ "video/",							false,	B_BAD_VALUE, },
		{ "unknown/",						false,	B_BAD_VALUE, },
		{ "/gif",							false,	B_BAD_VALUE, },
		{ "image/very/nice",				false,	B_BAD_VALUE, },
		{ "tex t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla in",					false,	B_BAD_VALUE, },
		{ "tex\tt/plain",					false,	B_BAD_VALUE, },
		{ "text/pla\tin",					false,	B_BAD_VALUE, },
		{ "tex\nt/plain",					false,	B_BAD_VALUE, },
		{ "text/pla\nin",					false,	B_BAD_VALUE, },
		{ "tex<t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla<in",					false,	B_BAD_VALUE, },
		{ "tex>t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla>in",					false,	B_BAD_VALUE, },
		{ "tex@t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla@in",					false,	B_BAD_VALUE, },
		{ "tex,t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla,in",					false,	B_BAD_VALUE, },
		{ "tex;t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla;in",					false,	B_BAD_VALUE, },
		{ "tex:t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla:in",					false,	B_BAD_VALUE, },
		{ "tex\"t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla\"in",					false,	B_BAD_VALUE, },
		{ "tex(t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla(in",					false,	B_BAD_VALUE, },
		{ "tex)t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla)in",					false,	B_BAD_VALUE, },
		{ "tex[t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla[in",					false,	B_BAD_VALUE, },
		{ "tex]t/pla]in",					false,	B_BAD_VALUE, },
		{ "tex?t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla?in",					false,	B_BAD_VALUE, },
		{ "tex=t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla=in",					false,	B_BAD_VALUE, },
		{ "tex\\t/plain",					false,	B_BAD_VALUE, },
		{ "text/pla\\in",					false,	B_BAD_VALUE, },
		// (not) too long types
		{ notTooLongType,					false,	B_OK, },
		{ tooLongType,						false,	B_BAD_VALUE, },
	};
	int32 testCount = sizeof(tests) / sizeof(mime_type_test);
	// test loop
	for (int32 i = 0; i < testCount; i++) {
		NextSubTest();
		mime_type_test &test = tests[i];
		BMimeType type(test.type);
		CHK(type.InitCheck() == test.error);
		bool valid = (test.error == B_OK);
		bool validSuper = (valid && test.super_type);
		// Type()
		if (valid)
			CHK(string(type.Type()) == test.type);
		else
			CHK(type.Type() == NULL);
		// IsValid(), IsSuperTypeOnly()
		CHK(type.IsValid() == valid);
		CHK(type.IsSupertypeOnly() == validSuper);
		CHK(BMimeType::IsValid(test.type) == valid);
		// GetSupertype()
		if (valid && !validSuper) {
			BMimeType super;
			CHK(type.GetSupertype(&super) == B_OK);
			CHK(super.InitCheck() == B_OK);
			CHK(super.Contains(&type) == true);
			BString typeString(test.type);
			BString superString(typeString.String(),
								typeString.FindFirst('/'));
			CHK(superString == super.Type());
		} else {
			BMimeType super;
			CHK(type.GetSupertype(&super) == B_BAD_VALUE);
		}
		// Contains(), ==
		for (int32 k = 0; k < testCount; k++) {
			mime_type_test &test2 = tests[k];
			BMimeType type2(test2.type);
			CHK(type2.InitCheck() == test2.error);
			bool valid2 = (test2.error == B_OK);
			bool validSuper2 = (valid && test2.super_type);
			bool equal = (!strcmp(test.type, test2.type));
			// ==
			if (valid || valid2) {
				CHK((type == type2) == equal);
				CHK((type == test2.type) == equal);
			} else {
				CHK((type == type2) == false);
				CHK((type == test2.type) == false);
			}
			// Contains()
			if (valid || valid2) {
				if (equal)
					CHK(type.Contains(&type2) == true);
				else if (validSuper && valid2 && !validSuper2) {
					BMimeType super2;
					CHK(type2.GetSupertype(&super2) == B_OK);
					bool contains = string(super2.Type()) == type.Type();
					CHK(type.Contains(&type2) == contains);
				} else
					CHK(type.Contains(&type2) == false);
			} else
				CHK(type.Contains(&type2) == false);
		}
	}
	// bad args
	NextSubTest();
	{
		BMimeType type("image/gif");
// R5: crashes when passing NULL
#if !TEST_R5
		CHK(BMimeType::IsValid(NULL) == false);
		CHK(type.GetSupertype(NULL) == B_BAD_VALUE);
		CHK(type.Contains(NULL) == false);
#endif
		CHK((type == NULL) == false);
	}
}

// an easy to construct equivalent of a notification message
class NotificationMessage {
public:
	NotificationMessage(int32 which, string type, string extraType,
						bool largeIcon)
		: which(which), type(type), hasExtraType(true), extraType(extraType),
		  hasLargeIcon(true), largeIcon(largeIcon)
	{
	}

	NotificationMessage(int32 which, string type, string extraType)
		: which(which), type(type), hasExtraType(true), extraType(extraType),
		  hasLargeIcon(false), largeIcon(false)
	{
	}

	NotificationMessage(int32 which, string type, bool largeIcon)
		: which(which), type(type), hasExtraType(false), extraType(),
		  hasLargeIcon(true), largeIcon(largeIcon)
	{
	}

	NotificationMessage(int32 which, string type)
		: which(which), type(type), hasExtraType(false), extraType(),
		  hasLargeIcon(false), largeIcon(false)
	{
	}

public:
	int32	which;
	string	type;
	bool	hasExtraType;
	string	extraType;
	bool	hasLargeIcon;
	bool	largeIcon;
};

// FillAttrInfo
static
void
FillAttrInfo(BMessage &info, int32 variation = 0)
{
	switch (variation) {
		case 0:
		default:
			CHK(info.AddString("attr:name", "attribute1") == B_OK);
			CHK(info.AddString("attr:public_name", "Nice Attribute1") == B_OK);
			CHK(info.AddInt32("attr:type", B_STRING_TYPE) == B_OK);
			CHK(info.AddBool("attr:public", true) == B_OK);
			CHK(info.AddBool("attr:editable", true) == B_OK);
			break;
		case 1:
			CHK(info.AddString("attr:name", "attribute2") == B_OK);
			CHK(info.AddString("attr:public_name", "Nice Attribute2") == B_OK);
			CHK(info.AddInt32("attr:type", B_BOOL_TYPE) == B_OK);
			CHK(info.AddBool("attr:public", false) == B_OK);
			CHK(info.AddBool("attr:editable", false) == B_OK);
			break;
	}
}

// MonitoringTest
void
MimeTypeTest::MonitoringTest()
{
	// tests:
	// * Start/StopWatching()
	// * updates

	// test:
	// * StartWatching()
	// * change something, check message queue (not empty)
	//   - add type
	//   - set icon, preferred app, attr info, file ext., short/long desc.,
	//     icon for, app hint, sniffer rule
	//   - remove type
	// * StopWatching(anotherTarget)
	// * change something, check message queue (not empty)
	// * StopWatching()
	// * change something, check message queue (empty)

	CHK(fApplication != NULL);
	NextSubTest();
	// StartWatching()
	BMessenger target(&fApplication->Handler(), fApplication);
	CHK(BMimeType::StartWatching(target) == B_OK);
	// install
	BMimeType type(testType);
	CHK(type.InitCheck() == B_OK);
	CHK(type.IsInstalled() == false);
	CHK(type.Install() == B_OK);
	// icon
	IconHelper iconHelperLarge(B_LARGE_ICON);
	IconHelper iconHelperMini(B_MINI_ICON);
	CHK(type.SetIcon(iconHelperLarge.Bitmap1(), B_LARGE_ICON) == B_OK);
	CHK(type.SetIcon(iconHelperMini.Bitmap1(), B_MINI_ICON) == B_OK);
	// preferred app
	CHK(type.SetPreferredApp(testTypeApp) == B_OK);
	// attr info
	BMessage attrInfo;
	FillAttrInfo(attrInfo);
	CHK(type.SetAttrInfo(&attrInfo) == B_OK);
	// file extensions
	BMessage extensions;
	CHK(extensions.AddString("extensions", "arg") == B_OK);
	CHK(extensions.AddString("extensions", "ugh") == B_OK);
	CHK(type.SetFileExtensions(&extensions) == B_OK);
	// long/short description
	CHK(type.SetLongDescription("quite short for a long description") == B_OK);
	CHK(type.SetShortDescription("short description") == B_OK);
	// icon for type
	CHK(type.SetIconForType("text/plain", iconHelperLarge.Bitmap1(),
							B_LARGE_ICON) == B_OK);
	CHK(type.SetIconForType("text/plain", iconHelperMini.Bitmap1(),
							B_MINI_ICON) == B_OK);
	// app hint
	entry_ref appHintRef;
	CHK(get_ref_for_path("/boot/beos/apps/StyledEdit", &appHintRef) == B_OK);
	CHK(type.SetAppHint(&appHintRef) == B_OK);
	// sniffer rule
	const char *snifferRule = "0.5 [0:0] ('ARGH')";
	CHK(type.SetSnifferRule(snifferRule) == B_OK);
	{
	//   - set icon, preferred app, attr info, file ext., short/long desc.,
	//     icon for, app hint, sniffer rule
		typedef NotificationMessage NM;
		NotificationMessage messages[] = {
			NM(B_MIME_TYPE_CREATED, testType),
			NM(B_ICON_CHANGED, testType, true),
			NM(B_ICON_CHANGED, testType, false),
			NM(B_PREFERRED_APP_CHANGED, testType),
			NM(B_ATTR_INFO_CHANGED, testType),
			NM(B_FILE_EXTENSIONS_CHANGED, testType),
			NM(B_LONG_DESCRIPTION_CHANGED, testType),
			NM(B_SHORT_DESCRIPTION_CHANGED, testType),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", true),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", false),
			NM(B_APP_HINT_CHANGED, testType),
			NM(B_SNIFFER_RULE_CHANGED, testType),
		};
		CheckNotificationMessages(messages, sizeof(messages) / sizeof(NM));
	}

	// set the same values once again
	NextSubTest();
	// icon
	CHK(type.SetIcon(iconHelperLarge.Bitmap1(), B_LARGE_ICON) == B_OK);
	CHK(type.SetIcon(iconHelperMini.Bitmap1(), B_MINI_ICON) == B_OK);
	// preferred app
	CHK(type.SetPreferredApp(testTypeApp) == B_OK);
	// attr info
	CHK(type.SetAttrInfo(&attrInfo) == B_OK);
// file extensions
	CHK(extensions.AddString("extensions", "arg") == B_OK);
	CHK(extensions.AddString("extensions", "ugh") == B_OK);
	CHK(type.SetFileExtensions(&extensions) == B_OK);
	// long/short description
	CHK(type.SetLongDescription("quite short for a long description") == B_OK);
	CHK(type.SetShortDescription("short description") == B_OK);
	// icon for type
	CHK(type.SetIconForType("text/plain", iconHelperLarge.Bitmap1(),
							B_LARGE_ICON) == B_OK);
	CHK(type.SetIconForType("text/plain", iconHelperMini.Bitmap1(),
							B_MINI_ICON) == B_OK);
	// app hint
	CHK(type.SetAppHint(&appHintRef) == B_OK);
	// sniffer rule
	CHK(type.SetSnifferRule(snifferRule) == B_OK);
	{
	//   - set icon, preferred app, attr info, file ext., short/long desc.,
	//     icon for, app hint, sniffer rule
		typedef NotificationMessage NM;
		NotificationMessage messages[] = {
			NM(B_ICON_CHANGED, testType, true),
			NM(B_ICON_CHANGED, testType, false),
			NM(B_PREFERRED_APP_CHANGED, testType),
			NM(B_ATTR_INFO_CHANGED, testType),
			NM(B_FILE_EXTENSIONS_CHANGED, testType),
			NM(B_LONG_DESCRIPTION_CHANGED, testType),
			NM(B_SHORT_DESCRIPTION_CHANGED, testType),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", true),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", false),
			NM(B_APP_HINT_CHANGED, testType),
			NM(B_SNIFFER_RULE_CHANGED, testType),
		};
		CheckNotificationMessages(messages, sizeof(messages) / sizeof(NM));
	}

	// set different values
	NextSubTest();
	// icon
	CHK(type.SetIcon(iconHelperLarge.Bitmap2(), B_LARGE_ICON) == B_OK);
	CHK(type.SetIcon(iconHelperMini.Bitmap2(), B_MINI_ICON) == B_OK);
	// preferred app
	CHK(type.SetPreferredApp("application/x-vnd.Be-STEE") == B_OK);
	// attr info
	BMessage attrInfo2;
	FillAttrInfo(attrInfo2, 1);
	CHK(type.SetAttrInfo(&attrInfo2) == B_OK);
	// file extensions
	CHK(extensions.AddString("extensions", "uff") == B_OK);
	CHK(extensions.AddString("extensions", "err") == B_OK);
	CHK(type.SetFileExtensions(&extensions) == B_OK);
	// long/short description
	CHK(type.SetLongDescription("not that short description") == B_OK);
	CHK(type.SetShortDescription("pretty short description") == B_OK);
	// icon for type
	CHK(type.SetIconForType("text/plain", iconHelperLarge.Bitmap2(),
							B_LARGE_ICON) == B_OK);
	CHK(type.SetIconForType("text/plain", NULL,
							B_LARGE_ICON) == B_OK);
	CHK(type.SetIconForType("text/plain", iconHelperMini.Bitmap2(),
							B_MINI_ICON) == B_OK);
	// app hint
	entry_ref appHintRef2;
	CHK(get_ref_for_path("/boot/beos/apps/NetPositive", &appHintRef2) == B_OK);
	CHK(type.SetAppHint(&appHintRef2) == B_OK);
	// sniffer rule
	const char *snifferRule2 = "0.7 [0:5] ('YEAH!')";
	CHK(type.SetSnifferRule(snifferRule2) == B_OK);
	// delete
	CHK(type.Delete() == B_OK);
	{
	//   - set icon, preferred app, attr info, file ext., short/long desc.,
	//     icon for, app hint, sniffer rule
		typedef NotificationMessage NM;
		NotificationMessage messages[] = {
			NM(B_ICON_CHANGED, testType, true),
			NM(B_ICON_CHANGED, testType, false),
			NM(B_PREFERRED_APP_CHANGED, testType),
			NM(B_ATTR_INFO_CHANGED, testType),
			NM(B_FILE_EXTENSIONS_CHANGED, testType),
			NM(B_LONG_DESCRIPTION_CHANGED, testType),
			NM(B_SHORT_DESCRIPTION_CHANGED, testType),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", true),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", true),
			NM(B_ICON_FOR_TYPE_CHANGED, testType, "text/plain", false),
			NM(B_APP_HINT_CHANGED, testType),
			NM(B_SNIFFER_RULE_CHANGED, testType),
			NM(B_MIME_TYPE_DELETED, testType),
		};
		CheckNotificationMessages(messages, sizeof(messages) / sizeof(NM));
	}

	// StopWatching() and try again -- no messages should be sent anymore
	CHK(BMimeType::StopWatching(target) == B_OK);
	// install
	CHK(type.InitCheck() == B_OK);
	CHK(type.IsInstalled() == false);
	CHK(type.Install() == B_OK);
	// icon
	CHK(type.SetIcon(iconHelperLarge.Bitmap1(), B_LARGE_ICON) == B_OK);
	CHK(type.SetIcon(iconHelperMini.Bitmap1(), B_MINI_ICON) == B_OK);
	// preferred app
	CHK(type.SetPreferredApp(testTypeApp) == B_OK);
	// attr info
	CHK(type.SetAttrInfo(&attrInfo) == B_OK);
	// file extensions
	CHK(extensions.AddString("extensions", "arg") == B_OK);
	CHK(extensions.AddString("extensions", "ugh") == B_OK);
	CHK(type.SetFileExtensions(&extensions) == B_OK);
	// long/short description
	CHK(type.SetLongDescription("quite short for a long description") == B_OK);
	CHK(type.SetShortDescription("short description") == B_OK);
	// icon for type
	CHK(type.SetIconForType("text/plain", iconHelperLarge.Bitmap1(),
							B_LARGE_ICON) == B_OK);
	CHK(type.SetIconForType("text/plain", iconHelperMini.Bitmap1(),
							B_MINI_ICON) == B_OK);
	// app hint
	CHK(type.SetAppHint(&appHintRef) == B_OK);
	// sniffer rule
	CHK(type.SetSnifferRule(snifferRule) == B_OK);
	// delete
	CHK(type.Delete() == B_OK);
	{
		CheckNotificationMessages(NULL, 0);
	}

	// bad args
	// StopWatching() another target
	NextSubTest();
	// install
	CHK(type.InitCheck() == B_OK);
	CHK(type.IsInstalled() == false);
	CHK(type.Install() == B_OK);
	// try to start/stop watching with an invalid target, stop the wrong target
	BMessenger target2(fApplication);
	CHK(target2.IsValid() == true);
	BMessenger target3("application/does-not_exist");
	CHK(target3.IsValid() == false);
// R5: An invalid messenger is fine for any reason?!
#if !TEST_R5
	CHK(BMimeType::StartWatching(target3) == B_BAD_VALUE);
#endif
	CHK(BMimeType::StartWatching(target) == B_OK);
#if !TEST_R5
	CHK(BMimeType::StopWatching(target3) == B_BAD_VALUE);
#endif
	CHK(BMimeType::StopWatching(target2) != B_OK);	// R5 == B_BAD_VALUE, Haiku == B_ENTRY_NOT_FOUND
	CHK(BMimeType::StopWatching(target) == B_OK);
	// delete
	CHK(type.Delete() == B_OK);
}

// CheckNotificationMessage
void
MimeTypeTest::CheckNotificationMessages(const NotificationMessage *messages,
										int32 count)
{
	// wait for the messages
	snooze(100000);
	if (fApplication) {
		BMessageQueue &queue = fApplication->Handler().Queue();
		CPPUNIT_ASSERT( queue.Lock() );
		try {
			int32 messageNum = 0;
			while (BMessage *_message = queue.NextMessage()) {
				BMessage message(*_message);
				delete _message;
//printf("\nmessage: %ld\n", messageNum);
//message.PrintToStream();
				CPPUNIT_ASSERT( messageNum < count );
				const NotificationMessage &entry = messages[messageNum];
				CPPUNIT_ASSERT( message.what == B_META_MIME_CHANGED );
				// which
				int32 which;
				CPPUNIT_ASSERT( message.FindInt32("be:which", &which)
								== B_OK );
				CPPUNIT_ASSERT( entry.which == which );
				// type
				const char *type;
				CPPUNIT_ASSERT( message.FindString("be:type", &type) == B_OK );
				CPPUNIT_ASSERT( entry.type == type );
				// extra type
				const char *extraType;
				if (entry.hasExtraType) {
					CPPUNIT_ASSERT( message.FindString("be:extra_type",
													   &extraType) == B_OK);
					CPPUNIT_ASSERT( entry.extraType == extraType );
				} else {
					CPPUNIT_ASSERT( message.FindString("be:extra_type",
										&extraType) == B_NAME_NOT_FOUND);
				}
				// large icon
				bool largeIcon;
				if (entry.hasLargeIcon) {
					CPPUNIT_ASSERT( message.FindBool("be:large_icon",
													 &largeIcon) == B_OK);
					CPPUNIT_ASSERT( entry.largeIcon == largeIcon );
				} else {
					CPPUNIT_ASSERT( message.FindBool("be:large_icon",
										&largeIcon) == B_NAME_NOT_FOUND);
				}
				messageNum++;
			}
			CPPUNIT_ASSERT( messageNum == count );
		} catch (CppUnit::Exception exception) {
			queue.Unlock();
			throw exception;
		}
		queue.Unlock();
	}
}

// helper class for update_mime_info() tests
class MimeInfoTestFile {
public:
	MimeInfoTestFile(string name, string type, const void *data = NULL,
					 int32 size = -1)
		: name(name),
		  type(type),
		  data(NULL),
		  size(0)
	{
		if (data) {
			if (size == -1)
				this->size = strlen((const char*)data) + 1;
			else
				this->size = size;
			this->data = new char[this->size];
			memcpy(this->data, data, this->size);
		}
	}

	~MimeInfoTestFile()
	{
		delete[] data;
	}

	status_t Create()
	{
		BFile file(name.c_str(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		status_t error = file.InitCheck();
		if (error == B_OK && data) {
			ssize_t written = file.Write(data, size);
			if (written < 0)
				error = written;
			else if (written != size)
				error = B_ERROR;
		}
		return error;
	}

	status_t Delete()
	{
		return BEntry(name.c_str()).Remove();
	}

	string	name;
	string	type;
	char	*data;
	int32	size;
};

// UpdateMimeInfoTest
void
MimeTypeTest::UpdateMimeInfoTest()
{
// Uncomment the following lines to enjoy the quiet time provided
// by a nice, full mime update. :-)

//	cout << "begin..." << endl;
//	CHK(update_mime_info(NULL, true, true, false) == B_OK);
//	cout << "end..." << endl;

	// tests:
	// * update_mime_info()

	// Note:
	// * Only synchronous calls are tested.
	// * Updating all files is not tested as it takes too long.

	// individual files
	execCommand(string("mkdir ") + testDir + "/subdir1 "
				+ testDir + "/subdir2 "
				+ testDir + "/subdir2/subsubdir1");
	MimeInfoTestFile files[] = {
		MimeInfoTestFile(string(testDir) + "/file1.cpp", "text/x-source-code"),
		MimeInfoTestFile(string(testDir) + "/subdir1/file1.gif", "image/gif"),
		MimeInfoTestFile(string(testDir) + "/subdir2/subsubdir1/file1",
						 "text/html", "<html>\n<body>\n</body></html>\n")
	};
	int fileCount = sizeof(files) / sizeof(MimeInfoTestFile);
	// synchronous
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		MimeInfoTestFile &file = files[i];
		// no recursion
		CHK(file.Create() == B_OK);
		CHK(update_mime_info(file.name.c_str(), false, true, false) == B_OK);
		BNode node(file.name.c_str());
		CHK(node.InitCheck() == B_OK);
		BString type;
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
		CHK(type == file.type.c_str());
		CHK(file.Delete() == B_OK);
		// recursion
		CHK(file.Create() == B_OK);
		CHK(update_mime_info(file.name.c_str(), true, true, false) == B_OK);
		CHK(node.SetTo(file.name.c_str()) == B_OK);
		type = "";
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
		CHK(type == file.type.c_str());
		CHK(file.Delete() == B_OK);
	}

//------------------------------------------------------------------------------
// Asynchronous calls
//------------------------------------------------------------------------------

	const bigtime_t kSnoozeTime = 500000;
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		MimeInfoTestFile &file = files[i];
		// no recursion
		CHK(file.Create() == B_OK);
		CHK(update_mime_info(file.name.c_str(), false, false, false) == B_OK);
		// give the system some time to do the update asynchronously
		snooze(kSnoozeTime);
		BNode node(file.name.c_str());
		CHK(node.InitCheck() == B_OK);
		BString type;
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
		CHK(type == file.type.c_str());
		CHK(file.Delete() == B_OK);
		// recursion
		CHK(file.Create() == B_OK);
		CHK(update_mime_info(file.name.c_str(), true, false, false) == B_OK);
		// give the system some time to do the update asynchronously
		snooze(kSnoozeTime);
		CHK(node.SetTo(file.name.c_str()) == B_OK);
		type = "";
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
		CHK(type == file.type.c_str());
		CHK(file.Delete() == B_OK);
	}

// TODO: The BeBook says: "if force is true, files are updated even if they've
// been updated already."
// As I understand this, calling update_mime_info() with force == true on a
// file, should set the BEOS:TYPE attribute regardless of whether it already
// had a value. The following test shows, that BEOS:TYPE remains unchanged
// though.
#if TEST_OBOS
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
//printf("file: %s\n", file.name.c_str());
		CHK(file.Create() == B_OK);
		// add a type attribute
		BNode node(file.name.c_str());
		CHK(node.InitCheck() == B_OK);
		BString type("text/plain");
		CHK(node.WriteAttrString("BEOS:TYPE", &type) == B_OK);
		// update, force == false
		CHK(update_mime_info(file.name.c_str(), false, true, false) == B_OK);
		type = "";
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		CHK(type == "text/plain");
		// update, force == true
		CHK(update_mime_info(file.name.c_str(), false, true, true) == B_OK);
		type = "";
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
//printf("%s <-> %s\n", type.String(), file.type.c_str());
		CHK(type == file.type.c_str());
		CHK(file.Delete() == B_OK);
	}
#endif	// TEST_OBOS

	// directory
	NextSubTest();
	// create
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
		CHK(file.Create() == B_OK);
	}
	// update, not recursive
	CHK(update_mime_info(testDir, false, true, false) == B_OK);
	// check
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
		BNode node(file.name.c_str());
		CHK(node.InitCheck() == B_OK);
		BString type;
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_ENTRY_NOT_FOUND);
	}
	// delete, re-create
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
		CHK(file.Delete() == B_OK);
		CHK(file.Create() == B_OK);
	}
	// update, recursive
	CHK(update_mime_info(testDir, true, true, false) == B_OK);
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
		BNode node(file.name.c_str());
		CHK(node.InitCheck() == B_OK);
		BString type;
		CHK(node.ReadAttrString("BEOS:TYPE", &type) == B_OK);
		node.Unset();
		CHK(type == file.type.c_str());
	}
	// delete
	for (int32 i = 0; i < fileCount; i++) {
		MimeInfoTestFile &file = files[i];
		CHK(file.Delete() == B_OK);
	}

	// bad args: non-existing file
	NextSubTest();
	BEntry entry(files[0].name.c_str());
	CHK(entry.InitCheck() == B_OK);
	CHK(entry.Exists() == false);
//#if TEST_R5
	CHK(update_mime_info(files[0].name.c_str(), false, true, false) == B_OK);
//#else
//	CHK(update_mime_info(files[0].name.c_str(), false, true, false) == B_ENTRY_NOT_FOUND);
//#endif
}

// WriteStringAttr
static
status_t
WriteStringAttr(BNode &node, string name, string _value)
{
	// Wrapper for BNode::WriteAttrString() taking string rather than
	// const char*/BString* parameters.
	BString value(_value.c_str());
	return node.WriteAttrString(name.c_str(), &value);
}

const uint32 MINI_ICON_TYPE = 'MICN';
const uint32 LARGE_ICON_TYPE = 'ICON';

// helper class for create_app_meta_mime() tests
class AppMimeTestFile {
public:
	AppMimeTestFile(string name, string type, string signature,
					string snifferRule,
					const void *miniIcon = NULL, const void *largeIcon = NULL)
		: name(name),
		  type(type),
		  signature(signature),
		  snifferRule(snifferRule),
		  miniIcon(NULL),
		  largeIcon(NULL)
	{
		SetMiniIcon(miniIcon);
		SetLargeIcon(largeIcon);
	}

	~AppMimeTestFile()
	{
		SetMiniIcon(NULL);
		SetLargeIcon(NULL);
	}

	void SetMiniIcon(const void *icon)
	{
		if (miniIcon) {
			delete[] miniIcon;
			miniIcon = NULL;
		}
		if (icon) {
			miniIcon = new char[256];
			memcpy(miniIcon, icon, 256);
		}
	}

	void SetLargeIcon(const void *icon)
	{
		if (largeIcon) {
			delete[] largeIcon;
			largeIcon = NULL;
		}
		if (icon) {
			largeIcon = new char[1024];
			memcpy(largeIcon, icon, 1024);
		}
	}

	status_t Create(bool setAttributes, bool setResources)
	{
		BFile file(name.c_str(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		status_t error = file.InitCheck();
		// attributes
		if (error == B_OK && setAttributes) {
			// type
			if (type.length() > 0)
				error = WriteStringAttr(file, "BEOS:TYPE", type);
			// signature
			if (error == B_OK)
				error = WriteStringAttr(file, "BEOS:APP_SIG", signature);
			// sniffer rule
			if (error == B_OK)
				error = WriteStringAttr(file, "BEOS:SNIFF_RULE", snifferRule);
			// mini icon
			if (error == B_OK && miniIcon) {
				ssize_t written = file.WriteAttr("BEOS:M:STD_ICON",
												 MINI_ICON_TYPE, 0, miniIcon,
												 256);
				if (written < 0)
					error = written;
				else if (written != 256)
					error = B_ERROR;
			}
			// large icon (ignored)
			if (error == B_OK && largeIcon) {
				ssize_t written = file.WriteAttr("META:L:STD_ICON",
												 LARGE_ICON_TYPE, 0, largeIcon,
												 1024);
				if (written < 0)
					error = written;
				else if (written != 1024)
					error = B_ERROR;
			}
		}
		// resources
		if (error == B_OK && setResources) {
			BResources resources;
			error = resources.SetTo(&file, true);
			// type (ignored)
			if (error == B_OK && type.length() > 0) {
				error = resources.AddResource(B_STRING_TYPE, 2, type.c_str(),
											  type.length() + 1, "BEOS:TYPE");
			}
			// signature (ignored)
			if (error == B_OK) {
				error = resources.AddResource(B_STRING_TYPE, 1,
											  signature.c_str(),
											  signature.length() + 1,
											  "BEOS:APP_SIG");
			}
			// mini icon (ignored)
			if (error == B_OK && miniIcon) {
				error = resources.AddResource(MINI_ICON_TYPE, 101, miniIcon,
											  256, "BEOS:M:STD_ICON");
			}
			// file types (ignored)
			if (error == B_OK) {
				BMessage msg;
				char *buffer = NULL;
				error = msg.AddString("types", "text/x-email");
				if (!error)
					error = msg.AddString("types", "video/mpeg");
				if (!error) {
					buffer = new char[msg.FlattenedSize()];
					if (!buffer)
						error = B_NO_MEMORY;
				}
				if (!error)
					error = msg.Flatten(buffer, msg.FlattenedSize());
				if (!error)
					error = resources.AddResource(B_MESSAGE_TYPE, 1, buffer,
													msg.FlattenedSize(), "BEOS:FILE_TYPES");
				delete [] buffer;
			}
		}
		return error;
	}

	status_t Delete(bool deleteMimeType)
	{
		status_t error = BEntry(name.c_str()).Remove();
		if (error == B_OK && deleteMimeType) {
			BMimeType type;
			// the type need not necessarily exist
			error = type.SetTo(signature.c_str());
			if (error == B_OK && deleteMimeType && type.IsInstalled())
				error = type.Delete();
		}
		return error;
	}

	string	name;
	string	type;
	string	signature;
	string	snifferRule;
	char	*miniIcon;
	char	*largeIcon;
};

// CheckAppMetaMime
static
void
CheckAppMetaMime(AppMimeTestFile &file)
{
	BMimeType type;
	CHK(type.SetTo(file.signature.c_str()) == B_OK);
	CHK(type.IsInstalled() == true);
	// short description
	char shortDescription[B_MIME_TYPE_LENGTH + 1];
	CHK(type.GetShortDescription(shortDescription) == B_OK);
	BPath path(file.name.c_str(), NULL, true);
	CHK(string(path.Leaf()) == shortDescription);
	// preferred app
	char preferredApp[B_MIME_TYPE_LENGTH + 1];
	CHK(type.GetPreferredApp(preferredApp) == B_OK);
	CHK(file.signature == preferredApp);
	// META:PPATH
	BNode typeFile;
	string typeFilename(string(mimeDatabaseDir) + "/" + file.signature);
//	cout << "typeFilename == '" << typeFilename << "'" << endl;
	CHK(typeFile.SetTo(typeFilename.c_str()) == B_OK);
	char filePath[B_PATH_NAME_LENGTH + 1];
	CHK(typeFile.ReadAttr("META:PPATH", B_STRING_TYPE, 0, filePath,
						  B_PATH_NAME_LENGTH + 1) > 0);
	CHK(path == filePath);
	// mini icon
	if (file.miniIcon) {
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(type.GetIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(memcmp(icon.Bits(), file.miniIcon, 256) == 0);
	}
	// large icon
/*	if (file.miniIcon) {
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(type.GetIcon(&icon, B_LARGE_ICON) == B_OK);
		CHK(memcmp(icon.Bits(), file.largeIcon, 1024) == 0);
	}*/
}

// CreateAppMetaMimeTest
void
MimeTypeTest::CreateAppMetaMimeTest()
{

// Uncomment the following lines to enjoy the quiet time provided by
// a nice, full create_app_meta_mime() update. :-)

//	cout << "begin..." << endl;
//	CHK(create_app_meta_mime(NULL, true, true, false) == B_OK);
//	cout << "end" << endl;

	// tests:
	// * create_app_meta_mime()

	// Note:
	// * Only synchronous calls are tested.
	// * The recursive flag isn't tested -- the BeBook sais, it is unused.
	// * Updating all apps is not tested as it takes too long.

	// Create a couple of icons to play around with
	char miniIcon1[256];
	char miniIcon2[256];
	for (int ch = 0; ch < 256; ch++) {
		miniIcon1[ch] = ch;
		miniIcon2[ch] = 255-ch;
	}
	char largeIcon1[1024];
	char largeIcon2[1024];
	for (int i = 0; i < 1024; i++) {
		char ch = i % 256;
		largeIcon1[i] = ch;
		largeIcon2[i] = 255-ch;
	}

	// attributes and resources
	NextSubTest();
	execCommand(string("mkdir ") + testDir + "/subdir1 "
				+ testDir + "/subdir2 "
				+ testDir + "/subdir2/subsubdir1");
	AppMimeTestFile files[] = {
		// AppMimeTestFile(name, type, sig, rule, miniIcon, largeIcon)
		AppMimeTestFile(string(testDir) + "/file1",
						"",
						"application/x-vnd.obos.mime.test.test1",
						"0.0 ('abc')",
						miniIcon1,
						NULL),
		AppMimeTestFile(string(testDir) + "/file2",
						"text/x-source-code",
						"application/x-vnd.obos.mime.test.test2",
						"0.0 ('xyz')",
						miniIcon2,
						largeIcon2),
		AppMimeTestFile(string(testDir) + "/file3",
						"application/x-vnd.Be-elfexecutable",
						"application/x-vnd.obos.mime.test.test3",
						"0.0 ('rst')",
						NULL,
						largeIcon1),
	};
	const int fileCount = sizeof(files) / sizeof(AppMimeTestFile);
//------------------------------------------------------------------------------
// Synchronous calls
//------------------------------------------------------------------------------
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file);
		// clean up
		CHK(file.Delete(true) == B_OK);
	}
//	snooze(999000000);

	// attributes only
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(true, false) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file);
		// clean up
		CHK(file.Delete(true) == B_OK);
	}

	// resources only
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(false, true) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		BMimeType type;
		CHK(type.SetTo(file.signature.c_str()) == B_OK);
		CHK(type.IsInstalled() == false);
		// clean up
		CHK(file.Delete(false) == B_OK);
	}

//------------------------------------------------------------------------------
// Asynchronous calls
//------------------------------------------------------------------------------
	const bigtime_t kSnoozeTime = 500000;
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, false, false)
			== B_OK);
		// give the system some time to do the update asynchronously
		snooze(kSnoozeTime);
		// check the MIME type
		CheckAppMetaMime(file);
		// clean up
		CHK(file.Delete(true) == B_OK);
	}

	// attributes only
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(true, false) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, false, false)
			== B_OK);
		// give the system some time to do the update asynchronously
		snooze(kSnoozeTime);
		// check the MIME type
		CheckAppMetaMime(file);
		// clean up
		CHK(file.Delete(true) == B_OK);
	}

	// resources only
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		// create file, create_app_meta_mime()
		AppMimeTestFile &file = files[i];
		CHK(file.Create(false, true) == B_OK);
		CHK(create_app_meta_mime(file.name.c_str(), false, false, false)
			== B_OK);
		// give the system some time to do the update asynchronously
		snooze(kSnoozeTime);
		BMimeType type;
		CHK(type.SetTo(file.signature.c_str()) == B_OK);
		CHK(type.IsInstalled() == false);
		// clean up
		CHK(file.Delete(false) == B_OK);
	}


	// test the force flag
// TODO: The BeBook says: "If force is true, entries are created even if they
// already exist."
// As I understand this, re-calling create_app_meta_mime() with force == true,
// after modifying the original file (e.g. the mini icon attribute) or
// calling it on another file with the same signature, should update the
// database entry. But the following tests show, that this doesn't happen.
// They fail in the third CheckAppMetaMime().
#if !TEST_R5
	// same file, same signature, other parameters
	{
		char icon1[256];
		char icon2[256];
		memset(icon1, 1, 256);
		memset(icon2, 2, 256);
		AppMimeTestFile file1(string(testDir) + "/file1",
						"application/x-vnd.Be-elfexecutable",
						"application/x-vnd.obos.mime.test.test1",
						icon1);
		AppMimeTestFile file2(string(testDir) + "/file1",
						"application/x-vnd.Be-elfexecutable",
						"application/x-vnd.obos.mime.test.test1",
						icon2);
		// create file 1, create_app_meta_mime()
		CHK(file1.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file1.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file1);
		// create file 2, create_app_meta_mime(), no force
		CHK(file2.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file2.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file1);
		// create_app_meta_mime(), force
		CHK(create_app_meta_mime(file2.name.c_str(), false, true, true)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file2);
		// clean up
		CHK(file2.Delete(true) == B_OK);
	}
	// different file, same signature, other parameters
	{
		char icon1[256];
		char icon2[256];
		memset(icon1, 1, 256);
		memset(icon2, 2, 256);
		AppMimeTestFile file1(string(testDir) + "/file1",
						"application/x-vnd.Be-elfexecutable",
						"application/x-vnd.obos.mime.test.test1",
						icon1);
		AppMimeTestFile file2(string(testDir) + "/file2",
						"application/x-vnd.Be-elfexecutable",
						"application/x-vnd.obos.mime.test.test1",
						icon2);
		// create file 1, create_app_meta_mime()
		CHK(file1.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file1.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file1);
		// create file 2, create_app_meta_mime(), no force
		CHK(file2.Create(true, true) == B_OK);
		CHK(create_app_meta_mime(file2.name.c_str(), false, true, false)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file1);
		// create_app_meta_mime(), force
		CHK(create_app_meta_mime(file2.name.c_str(), false, true, true)
			== B_OK);
		// check the MIME type
		CheckAppMetaMime(file2);
		// clean up
		CHK(file1.Delete(true) == B_OK);
		CHK(file2.Delete(true) == B_OK);
	}
#endif	// !TEST_R5

	// bad args
	NextSubTest();
	// no signature
	CHK(files[0].Create(false, false) == B_OK);
	CHK(create_app_meta_mime(files[0].name.c_str(), false, true, false)
		== B_OK);
	CHK(files[0].Delete(false) == B_OK);
	// non-existing file
	CHK(create_app_meta_mime(files[0].name.c_str(), false, true, false)
		== B_OK);

}

// CheckIconData
static
void
CheckIconData(const char *device, int32 iconSize, const void* data)
{
	// open the device
	int fd = open(device, O_RDONLY);
	CHK(fd != -1);
	// get the icon
	char buffer[1024];
	device_icon iconData = {
		iconSize,
		buffer
	};
	int error = ioctl(fd, B_GET_ICON, &iconData);
	// close the device
	CHK(close(fd) == 0);
	CHK(error == 0);
	// compare the icon data
	CHK(memcmp(data, buffer, iconSize * iconSize) == 0);
}

// GetDeviceIconTest
void
MimeTypeTest::GetDeviceIconTest()
{
	// tests:
	// * get_device_icon()

	// test a volume device, a non-volume device, and an invalid dev name
	struct test_case {
		const char	*path;
		bool		valid;
	} testCases[] = {
		{ "/dev/zero", false },
		{ "/boot", true },
		{ "/boot/home", false }
	};
	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int32 i = 0; i < testCaseCount; i++) {
		NextSubTest();
		test_case &testCase = testCases[i];
		// get device name from path name
		fs_info info;
		const char *deviceName = testCase.path;
		if (testCase.valid) {
			dev_t dev = dev_for_path(testCase.path);
			CHK(dev > 0);
			CHK(fs_stat_dev(dev, &info) == 0);
			deviceName = info.device_name;
		}
		// the two valid and one invalid icon size
		const int32	iconSizes[] = { 16, 32, 20 };
		const bool 	validSizes[] = { true, true, false };
		const int sizeCount = sizeof(iconSizes) / sizeof(int32);
		for (int32 k = 0; k < sizeCount; k++) {
			int32 size = iconSizes[k];
			bool valid = testCase.valid && validSizes[k];
			char buffer[1024];
			if (valid) {
				CHK(get_device_icon(deviceName, buffer, size) == B_OK);
				CheckIconData(deviceName, size, buffer);
				// bad args: NULL buffer
// R5: Wanna see KDL? Here you go...
#if !TEST_R5
				CHK(get_device_icon(deviceName, NULL, size) == B_BAD_VALUE);
#endif
			} else
				CHK(get_device_icon(deviceName, buffer, size) != B_OK);
		}
	}
}

// SnifferRuleTest
void
MimeTypeTest::SnifferRuleTest()
{
	// tests:
	// * status_t GetSnifferRule(BString *result) const;
	// * status_t SetSnifferRule(const char *);
	// * static status_t CheckSnifferRule(const char *rule, BString *parseError);

	// test a couple of valid and invalid rules
	struct test_case {
		const char	*rule;
		const char	*error;	// NULL, if valid
	} testCases[] = {
		// valid rules
		{ "1.0 (\"ABCD\")", NULL },
		{ "1.0 ('ABCD')", NULL },
		{ "  1.0 ('ABCD')  ", NULL },
		{ "0.8 [0:3] ('ABCDEFG' | 'abcdefghij')", NULL },
		{ "0.5([10]'ABCD'|[17]'abcd'|[13]'EFGH')", NULL } ,
		{ "0.5  \n   [0:3]  \t ('ABCD' \n | 'abcd' | 'EFGH')", NULL },
		{ "0.8 [  0  :  3  ] ('ABCDEFG' | 'abcdefghij')", NULL },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg')", NULL },
// These two rules are accepted by the R5 sniffer checker, but not
// by the parser. Thus, we're not accepting them with either.
#if TEST_R5
		{ "1.0 ('ABCD') | ('EFGH')", NULL },
		{ "1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", NULL },
#else
		{ "1.0 ('ABCD') | ('EFGH')", "Sniffer pattern error: missing pattern" },
		{ "1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", "Sniffer pattern error: missing pattern" },
#endif
		{ "0.8 [0:3] (\\077Mkl0x34 & 'abcdefgh')", NULL },
		{ "0.8 [0:3] (\\077034 & 'abcd')", NULL },
		{ "0.8 [0:3] (\\077\\034 & 'ab')", NULL },
		{ "0.8 [0:3] (\\77\\034 & 'ab')", NULL },
		{ "0.8 [0:3] (\\7 & 'a')", NULL },
		{ "0.8 [0:3] (\"\\17\" & 'a')", NULL },
		{ "0.8 [0:3] ('\\17' & 'a')", NULL },
		{ "0.8 [0:3] (\\g & 'a')", NULL },
		{ "0.8 [0:3] (\\g&\\b)", NULL },
		{ "0.8 [0:3] (\\g\\&b & 'abc')", NULL },
		{ "0.8 [0:3] (0x3457 & 'ab')", NULL },
		{ "0.8 [0:3] (0xA4b7 & 'ab')", NULL },
		{ "0.8 [0:3] ('ab\"' & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\\"\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\A\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab'\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\\\\" & 'abc')", NULL },
		{ "0.8 [-5:-3] (\"abc\" & 'abc')", NULL },
// Also accepted by the R5 sniffer but not the R5 parser. We reject.
#if TEST_R5
		{ "0.8 [5:3] (\"abc\" & 'abc')", NULL },
#else
		{ "0.8 [5:3] (\"abc\" & 'abc')", "Sniffer Parser Error -- Invalid range: [5:3]" },
#endif
		{ "1.0 ('ABCD')", NULL },
		{ ".2 ('ABCD')", NULL },
		{ "0. ('ABCD')", NULL },
		{ "1 ('ABCD')", NULL },
		{ "+1 ('ABCD')", NULL },
// We accept extended notation floating point numbers now, but
// not invalid priorities. Thus our checker chokes on these rules,
// whilest R5's does not
#if TEST_R5
		{ "1E25 ('ABCD')", NULL },
		{ "1e25 ('ABCD')", NULL },
#else
		{ "1E25 ('ABCD')", "Sniffer pattern error: invalid priority" },
		{ "1e25 ('ABCD')", "Sniffer pattern error: invalid priority" },
#endif

// R5 chokes on this rule :-( Why? I don't know. :-)
#if TEST_R5
		{ "1e-3 ('ABCD')", "Sniffer pattern error: missing pattern" },
#else
		{ "1e-3 ('ABCD')", NULL },
#endif
		{ "+.003e2 ('ABCD')", NULL },
// R5 chokes on this one too. See how much better our checker/parser is? ;-)
#if TEST_R5
		{ "-123e-9999999999 ('ABCD')", "Sniffer pattern error: bad token" },	// Hooray for the stunning accuracy of floating point :-)
#else
		{ "-123e-9999999999 ('ABCD')", NULL },	// Hooray for the stunning accuracy of floating point :-)
#endif
		// invalid rules
		{ "0.0 ('')",
			"Sniffer pattern error: illegal empty pattern" },
		{ "('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "[0:3] ('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "0.8 [0:3] ( | 'abcdefghij')",
		  "Sniffer pattern error: missing pattern" },
		{ "0.8 [0:3] ('ABCDEFG' | )",
		  "Sniffer pattern error: missing pattern" },
		{ "[0:3] ('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "1.0 (ABCD')",
#if TEST_R5
			"Sniffer pattern error: misplaced single quote"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 ('ABCD)",
#if TEST_R5
			"Sniffer pattern error: unterminated rule"
#else
			"Sniffer pattern error: unterminated single-quoted string"
#endif
		},
		{ "1.0 (ABCD)",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 (ABCD 'ABCD')",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 'ABCD')",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: missing pattern"
#endif
		},
		{ "1.0 ('ABCD'",
			"Sniffer pattern error: unterminated rule" },
		{ "1.0 'ABCD'",
#if TEST_R5
			"Sniffer pattern error: missing sniff pattern"
#else
			"Sniffer pattern error: missing pattern"
#endif
		},
		{ "0.5 [0:3] ('ABCD' | 'abcd' | [13] 'EFGH')",
		  	"Sniffer pattern error: missing pattern" },
		{ "0.5('ABCD'|'abcd'|[13]'EFGH')",
		  	"Sniffer pattern error: missing pattern" },
		{ "0.5[0:3]([10]'ABCD'|[17]'abcd'|[13]'EFGH')",
		  	"Sniffer pattern error: missing pattern" },
		{ "0.8 [0x10:3] ('ABCDEFG' | 'abcdefghij')",
		  	"Sniffer pattern error: pattern offset expected" },
		{ "0.8 [0:A] ('ABCDEFG' | 'abcdefghij')",
#if TEST_R5
		  	"Sniffer pattern error: pattern range end expected"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefghij')",
		  	"Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg' & 'xyzwmno')",
#if TEST_R5
		  	"Sniffer pattern error: unterminated rule"
#else
			"Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		},
		{ "0.8 [0:3] (\\g&b & 'a')",
#if TEST_R5
			"Sniffer pattern error: missing mask"
#else
			"Sniffer pattern error: invalid character 'b'"
#endif
		},
		{ "0.8 [0:3] (\\19 & 'a')",
		  	"Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] (0x345 & 'ab')",
		  	"Sniffer pattern error: bad hex literal" },
		{ "0.8 [0:3] (0x3457M & 'abc')",
#if TEST_R5
		  	"Sniffer pattern error: expecting '|' or '&'"
#else
			"Sniffer pattern error: invalid character 'M'"
#endif
		},
		{ "0.8 [0:3] (0x3457\\7 & 'abc')",
#if TEST_R5
		  	"Sniffer pattern error: expecting '|' or '&'"
#else
			"Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		},

		// Miscellaneous tests designed to hit every remaining
		// relevant "throw new Err()" statement in our scanner.
		// R5 versions may come later, but I don't really see any
		// good reason why at this point...
#if !TEST_R5
		{ "\x03  ", "Sniffer pattern error: invalid character '\x03'" },
		{ "\"blah", "Sniffer pattern error: unterminated double-quoted string" },
		{ "0xThisIsNotAHexCode", "Sniffer pattern error: incomplete hex code" },
		{ "0xAndNeitherIsThis:-)", "Sniffer pattern error: bad hex literal" },
		{ ".NotAFloat", "Sniffer pattern error: incomplete floating point number" },
		{ "-NotANumber", "Sniffer pattern error: incomplete signed number" },
		{ "+NotANumber", "Sniffer pattern error: incomplete signed number" },

		{ "0.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },

		{ "0.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },

		{ "0.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },

		{ "\\11\\", "Sniffer pattern error: incomplete escape sequence" },
		{ "\"Escape!! \\", "Sniffer pattern error: incomplete escape sequence" },
		{ "'Escape!! \\", "Sniffer pattern error: incomplete escape sequence" },

		{ "\\x", "Sniffer pattern error: incomplete escaped hex code" },
		{ "\\xNotAHexCode", "Sniffer pattern error: incomplete escaped hex code" },
		{ "\\xAlsoNotAHexCode", "Sniffer pattern error: incomplete escaped hex code" },
		{ "\\x0", "Sniffer pattern error: incomplete escaped hex code" },

		{ "1.0 (\\377)", NULL },
		{ "\\400", "Sniffer pattern error: invalid octal literal (octals must be between octal 0 and octal 377 inclusive)" },
		{ "\\777", "Sniffer pattern error: invalid octal literal (octals must be between octal 0 and octal 377 inclusive)" },
		{ "1.0 (\\800)", NULL },

		{ NULL, "Sniffer pattern error: NULL pattern" },

		{ "-2", "Sniffer pattern error: invalid priority" },
		{ "+2", "Sniffer pattern error: invalid priority" },

		{ "1.0", "Sniffer pattern error: missing expression" },
#endif	// !TEST_R5

//! \todo Our parser chokes on this rule and I have no idea why
// I don't currently understand what's wrong with the following rule...
// R5 rejects it though, for whatever reason.
#if TEST_R5
		{ "1E-25 ('ABCD')", "Sniffer pattern error: missing pattern" },
#else
//		{ "1E-25 ('ABCD')", NULL },
#endif
	};

	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	BMimeType type;
	CHK(type.SetTo(testType) == B_OK);
	CHK(type.Install() == B_OK);
	for (int32 i = 0; i < testCaseCount; i++) {
		NextSubTest();
		test_case &testCase = testCases[i];
		BString parseError;
		status_t error = BMimeType::CheckSnifferRule(testCase.rule,
													 &parseError);
//		printf("\n---------------------\n");
//		printf("rule == '%s', %s\n", testCase.rule, (testCase.error ? "should not pass" : "should pass"));
		if (testCase.error == NULL) {
if (error != B_OK)
printf("\nerror:\n%s\n", parseError.String());
			CHK(error == B_OK);
			CHK(type.SetSnifferRule(testCase.rule) == B_OK);
			BString rule;
			CHK(type.GetSnifferRule(&rule) == B_OK);
			CHK(rule == testCase.rule);
		} else {
//			printf("error == 0x%lx\n", error);
//			if (parseError.FindLast(testCase.error) < 0) {
//				printf("\nexpected:\n%s\n", testCase.error);
//				printf("\nfound:\n%s\n", parseError.String());
//			}
			CHK(error == (testCase.rule ? B_BAD_MIME_SNIFFER_RULE : B_BAD_VALUE));
			CHK(parseError.FindLast(testCase.error) >= 0);

// R5 treats a NULL rule string as an error, and thus R5::SetSnifferRule(NULL) fails.
// We also treat a NULL rule string as an error, but OBOS::SetSnifferRule(NULL) does
// not fail, as all OBOS::BMimeType::Set*(NULL) calls are equivalent to the
// corresponding OBOS::BMimeType::Delete*() calls.
#if TEST_R5
			CHK(type.SetSnifferRule(testCase.rule) == B_BAD_MIME_SNIFFER_RULE);
#else
			if (testCase.rule)
				CHK(type.SetSnifferRule(testCase.rule) == B_BAD_MIME_SNIFFER_RULE);
			else
				CHK(type.SetSnifferRule(testCase.rule) == B_OK);
#endif
		}
	}

	// bad args: NULL rule/result string
	NextSubTest();
	BString parseError;
	CHK(BMimeType::CheckSnifferRule("0.0 ('')", NULL)
		== B_BAD_MIME_SNIFFER_RULE);
// R5: crashes when passing a NULL rule/result buffer.
#if !TEST_R5
	CHK(BMimeType::CheckSnifferRule(NULL, &parseError) == B_BAD_VALUE);
	CHK(BMimeType::CheckSnifferRule(NULL, NULL) == B_BAD_VALUE);
	CHK(type.GetSnifferRule(NULL) == B_BAD_VALUE);
#endif

	BString rule;

	// NULL rule to SetSnifferRule unsets the attribute
	NextSubTest();
#if TEST_R5
	CHK(type.IsInstalled() == true);
	CHK(type.SetSnifferRule(NULL) == B_OK);
	CHK(type.GetSnifferRule(&rule) == B_ENTRY_NOT_FOUND);
#else
	CHK(type.IsInstalled() == true);
	if (type.GetSnifferRule(&rule) == B_ENTRY_NOT_FOUND)
		CHK(type.SetSnifferRule("0.0 ('abc')") == B_OK);
	CHK(type.GetSnifferRule(&rule) == B_OK);
	CHK(type.SetSnifferRule(NULL) == B_OK);
	CHK(type.GetSnifferRule(&rule) == B_ENTRY_NOT_FOUND);
	CHK(type.SetSnifferRule(NULL) == B_ENTRY_NOT_FOUND);
#endif

	// bad args: uninstalled type
	CHK(type.Delete() == B_OK);
	CHK(type.GetSnifferRule(&rule) == B_ENTRY_NOT_FOUND);
	CHK(type.SetSnifferRule("0.0 ('ABC')") == B_OK);
#if TEST_R5
	CHK(type.GetSnifferRule(&rule) == B_ENTRY_NOT_FOUND);
#else
	CHK(type.GetSnifferRule(&rule) == B_OK);
#endif

	// bad args: uninitialized BMimeType
	type.Unset();
	CHK(type.GetSnifferRule(&rule) != B_OK);
	CHK(type.SetSnifferRule("0.0 ('ABC')") != B_OK);
}

// helper class for GuessMimeType() tests
class SniffingTestFile {
public:
	SniffingTestFile(string name, string extensionType, string contentType,
					 const void *data = NULL, int32 size = -1, string metaType = "")
		: name(name)
		, extensionType(extensionType)
		, contentType(contentType)
		, data(NULL)
		, size(0)
		, metaType(metaType)
	{
		// replace wildcard types
		if (this->extensionType == "")
			this->extensionType = "application/octet-stream";
		if (this->contentType == "")
			this->contentType = "application/octet-stream";
		// copy data
		if (data) {
			if (size == -1)
				this->size = strlen((const char*)data) + 1;
			else
				this->size = size;
			this->data = new char[this->size];
			memcpy(this->data, data, this->size);
		}
	}

	~SniffingTestFile()
	{
		delete[] data;
	}

	status_t Create()
	{
		BFile file(name.c_str(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		ssize_t error = file.InitCheck();
		if (error == B_OK && data) {
			ssize_t written = file.Write(data, size);
			if (written < 0)
				error = written;
			else if (written != size)
				error = B_ERROR;
		}
		if (!error && metaType.length() > 0) {
			error = file.WriteAttr("META:TYPE", B_STRING_TYPE, 0, metaType.c_str(),
				metaType.length()+1);
			error = error == (ssize_t)(metaType.length()+1) ? B_OK : error;
		}
		return error;
	}

	status_t Delete()
	{
		return BEntry(name.c_str()).Remove();
	}

	string	name;
	string	extensionType;
	string	contentType;
	char	*data;
	int32	size;
	string metaType;
};

// SniffingTest
void
MimeTypeTest::SniffingTest()
{
	// tests:
	// * GuessMimeType()

	// install some test types with sniffer rules
	{
		BMimeType type;
		CHK(type.SetTo(testType) == B_OK);
		CHK(type.Install() == B_OK);
		CHK(type.SetSnifferRule("0.5 [0:1] ('ABCD_EFGH' & 0xffffffff00ffffffff)")
			== B_OK);
		CHK(type.SetTo(testType1) == B_OK);
		CHK(type.Install() == B_OK);
		CHK(type.SetSnifferRule("0.4 ('ABCD')") == B_OK);
		CHK(type.SetTo(testType2) == B_OK);
		CHK(type.Install() == B_OK);
#if TEST_R5
		// This rule is invalid!
		CHK(type.SetSnifferRule("0.4 [0] ('XYZ') | [0:5] ('CD  E')") == B_OK);
#else
//		CHK(type.SetSnifferRule("0.4 ([0] 'XYZ' | [0:5] 'CD  E')") == B_OK);
#endif
		CHK(type.SetTo(testType3) == B_OK);
		CHK(type.Install() == B_OK);
		CHK(type.SetSnifferRule("0.3 [0:8] ('ABCD' | 'EFGH')") == B_OK);
		CHK(type.SetTo(testType4) == B_OK);
		CHK(type.Install() == B_OK);
		CHK(type.SetSnifferRule("0.2 [0:3] ('ABCD' | 'abcd')") == B_OK);
		CHK(type.SetTo(testType5) == B_OK);
		CHK(type.Install() == B_OK);
		CHK(type.SetSnifferRule("0.2 ('LMNO' & 0xfffeffff)") == B_OK);
	}

	SniffingTestFile files[] = {
		SniffingTestFile(string(testDir) + "/file1.cpp",
						 "text/x-source-code", ""),
		SniffingTestFile(string(testDir) + "/file2.gif",
						 "image/gif", ""),
		SniffingTestFile(string(testDir) + "/file3",
						 "", "text/html",
						 "<html>\n<body>\n</body></html>\n"),
		SniffingTestFile(string(testDir) + "/file4.cpp",
						 "text/x-source-code", "text/html",
						 "<html>\n<body>\n</body></html>\n"),
		SniffingTestFile(string(testDir) + "/file5", "", testType1, "ABCD"),
		SniffingTestFile(string(testDir) + "/file6", "", testType3, " ABCD"),
		SniffingTestFile(string(testDir) + "/file7", "", testType4, "abcd"),
		SniffingTestFile(string(testDir) + "/file8", "", testType3,
						 " ABCDEFGH"),
		SniffingTestFile(string(testDir) + "/file9", "", testType,
						 " ABCD EFGH"),
//		SniffingTestFile(string(testDir) + "/file10", "", testType2,
		SniffingTestFile(string(testDir) + "/file10", "", testType3,
						 " ABCD  EFGH"),
		SniffingTestFile(string(testDir) + "/file11", "", testType5,
						 "LMNO"),
		SniffingTestFile(string(testDir) + "/file12", "", testType5,
						 "LLNO"),
		SniffingTestFile(string(testDir) + "/file13", "", "",
						 "LNNO"),
		// meta mime test
// bonefish: TODO: Now that content sniffing is enabled again, this doesn't
// work properly anymore, since there are actually three types involved: The
// extension type ("text/html"), the content type (also "text/html") and the
// real type ("application/x-vnd.be-meta-mime") which is concluded from the
// existence of the "META:TYPE" attribute rather than the extension or content.
//#if !TEST_R5
//		SniffingTestFile(string(testDir) + "/file14.html",
//						 "text/html", "application/x-vnd.be-meta-mime",
//						 "<html>\n<body>\n</body></html>\n", -1,
//						 "fake-meta-mime-string"),
//#endif	// !TEST_R5
	};
	int fileCount = sizeof(files) / sizeof(SniffingTestFile);
	for (int32 i = 0; i < fileCount; i++) {
		NextSubTest();
		SniffingTestFile &file = files[i];
		const char *filename = file.name.c_str();
//printf("file: %s\n", filename);
		const char *extensionType = file.extensionType.c_str();
		const char *contentType = file.contentType.c_str();
		const char *realType = contentType;
		if (file.contentType == "application/octet-stream")
			realType = extensionType;
		// GuessMimeType(const char*,)
		BMimeType type;
		CHK(BMimeType::GuessMimeType(filename, &type) == B_OK);
//printf("type: `%s', extensionType: `%s'\n", type.Type(), extensionType);
		CHK(type == extensionType);
		type.Unset();
		// GuessMimeType(const void*, int32,)
		if (file.data != NULL) {
			CHK(BMimeType::GuessMimeType(file.data, file.size, &type) == B_OK);
if (!(type == contentType))
printf("type: %s, should be: %s\n", type.Type(), realType);
			CHK(type == contentType);
			type.Unset();
		}
		CHK(file.Create() == B_OK);
		// set BEOS:TYPE to something confusing ;-)
		BNode node;
		CHK(node.SetTo(filename) == B_OK);
		CHK(WriteStringAttr(node, "BEOS:TYPE", "application/x-person") == B_OK);
		// GuessMimeType(const ref*,)
		entry_ref ref;
		CHK(get_ref_for_path(filename, &ref) == B_OK);
		CHK(BMimeType::GuessMimeType(&ref, &type) == B_OK);
if (!(type == realType))
printf("type: %s, should be: %s (file == '%s')\n", type.Type(), realType, filename);
		CHK(type == realType);
		type.Unset();
		CHK(file.Delete() == B_OK);
	}

	// GuessMimeType(const ref*,), invalid/abstract entry
	{
		NextSubTest();
		string filename = string(testDir) + "/file100.cpp";
		BMimeType type;
		entry_ref ref;
// invalid entry_ref: R5: Is fine! Haiku: no dice
#if TEST_R5
		CHK(BMimeType::GuessMimeType(&ref, &type) == B_OK);
		CHK(type == "application/octet-stream");
#else
		CHK(BMimeType::GuessMimeType(&ref, &type) != B_OK);
#endif
		// abstract entry_ref
		CHK(get_ref_for_path(filename.c_str(), &ref) == B_OK);
		// R5: B_NAME_NOT_FOUND, Haiku:
		CHK(BMimeType::GuessMimeType(&ref, &type) != B_OK);
	}

	// bad args
	{
		NextSubTest();
		SniffingTestFile &file = files[0];
		CHK(file.Create() == B_OK);
		const char *filename = file.name.c_str();
		entry_ref ref;
		CHK(get_ref_for_path(filename, &ref) == B_OK);
		BMimeType type;
		// NULL BMimeType
		CHK(BMimeType::GuessMimeType(filename, NULL) == B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType(file.data, file.size, NULL)
			== B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType(&ref, NULL) == B_BAD_VALUE);
		// NULL filename/ref/data
		CHK(BMimeType::GuessMimeType((const char*)NULL, &type) == B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType(NULL, 10, &type) == B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType((const entry_ref*)NULL, &type)
			== B_BAD_VALUE);
		// NULL BMimeType and filename/ref/data
		CHK(BMimeType::GuessMimeType((const char*)NULL, NULL) == B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType(NULL, 10, NULL) == B_BAD_VALUE);
		CHK(BMimeType::GuessMimeType((const entry_ref*)NULL, NULL)
			== B_BAD_VALUE);
		CHK(file.Delete() == B_OK);
	}
}


/* KEY:
   + == Tests implemented
   * == Function implemented
*/

/* Ingo's functions:

	// initialization
+*	BMimeType();
+*	BMimeType(const char *mimeType);
(*	virtual ~BMimeType();)

+*	status_t SetTo(const char *mimeType);
+*	status_t SetType(const char *mimeType);
+*	void Unset();
+*	status_t InitCheck() const;

	// string access
+*	const char *Type() const;
+*	bool IsValid() const;
+*	static bool IsValid(const char *mimeType);
+*	bool IsSupertypeOnly() const;
+*	status_t GetSupertype(BMimeType *superType) const;
+*	bool Contains(const BMimeType *type) const;
+*	bool operator==(const BMimeType &type) const;
+*	bool operator==(const char *type) const;

	// MIME database monitoring
+	static status_t StartWatching(BMessenger target);
+	static status_t StopWatching(BMessenger target);

	// C functions
+	int update_mime_info(const char *path, int recursive, int synchronous,
						 int force);
+	status_t create_app_meta_mime(const char *path, int recursive,
								  int synchronous, int force);
+	status_t get_device_icon(const char *dev, void *icon, int32 size);

	// sniffer rule manipulation
+	status_t GetSnifferRule(BString *result) const;
+	status_t SetSnifferRule(const char *);
+	static status_t CheckSnifferRule(const char *rule, BString *parseError);

	// sniffing
+	status_t GuessMimeType(const entry_ref *file, BMimeType *result);
+	static status_t GuessMimeType(const void *buffer, int32 length,
								  BMimeType *result);
+	static status_t GuessMimeType(const char *filename, BMimeType *result);
*/


/* Tyler's functions:

	// MIME database access
+	status_t Install();
+	status_t Delete();
+	bool IsInstalled() const;
+	status_t GetIcon(BBitmap *icon, icon_size size) const;
+	status_t GetPreferredApp(char *signature, app_verb verb = B_OPEN) const;
+	status_t GetAttrInfo(BMessage *info) const;
+	status_t GetFileExtensions(BMessage *extensions) const;
+	status_t GetShortDescription(char *description) const;
+	status_t GetLongDescription(char *description) const;
	status_t GetSupportingApps(BMessage *signatures) const;

+	status_t SetIcon(const BBitmap *icon, icon_size size);
+	status_t SetPreferredApp(const char *signature, app_verb verb = B_OPEN);
+	status_t SetAttrInfo(const BMessage *info);
+	status_t SetFileExtensions(const BMessage *extensions);
+	status_t SetShortDescription(const char *description);
+	status_t SetLongDescription(const char *description);

+	static status_t GetInstalledSupertypes(BMessage *super_types);
+	static status_t GetInstalledTypes(BMessage *types);
+	static status_t GetInstalledTypes(const char *super_type,
									  BMessage *subtypes);
+	static status_t GetWildcardApps(BMessage *wild_ones);

+	status_t GetAppHint(entry_ref *ref) const;
+	status_t SetAppHint(const entry_ref *ref);

+	status_t GetIconForType(const char *type, BBitmap *icon,
							icon_size which) const;
+	status_t SetIconForType(const char *type, const BBitmap *icon,
							icon_size which);
*/


