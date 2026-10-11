/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string>
#include <unistd.h>

#include <ByteOrder.h>
#include <Entry.h>
#include <File.h>
#include <Mime.h>
#include <ResourceStrings.h>
#include <Resources.h>
#include <StorageDefs.h>
#include <String.h>
#include <SupportDefs.h>
#include <TypeConstants.h>
#include <image.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"


static const char* testDir = "/tmp/testDir";
static const char* x86ResFile = "/tmp/testDir/x86.rsrc";
static const char* ppcResFile = "/tmp/testDir/ppc.rsrc";
static const char* elfFile = "/tmp/testDir/elf";
static const char* pefFile = "/tmp/testDir/pef";
static const char* emptyFile = "/tmp/testDir/empty-file";
static const char* noResFile = "/tmp/testDir/no-res-file";
static const char* testFile1 = "/tmp/testDir/testFile1";
static const char* testFile2 = "/tmp/testDir/testFile2";
static const char* noSuchFile = "/tmp/testDir/no-such-file";
static const char* x86ResName = "x86.rsrc";
static const char* ppcResName = "ppc.rsrc";
static const char* elfName = "elf";
static const char* elfNoResName = "elf-no-res";
static const char* pefName = "pef";
static const char* pefNoResName = "pef-no-res";

struct ResourceInfo {
	ResourceInfo(type_code type, int32 id, const void* data, size_t size, const char* name = NULL)
		: type(type),
		  id(id),
		  name(NULL),
		  data(NULL),
		  size(size)
	{
		if (data) {
			this->data = new char[size];
			memcpy(this->data, data, size);
		}
		if (name) {
			int32 len = strlen(name);
			this->name = new char[len + 1];
			strcpy(this->name, name);
		}
	}

	~ResourceInfo()
	{
		delete[] name;
		delete[] data;
	}

	type_code type;
	int32 id;
	char* name;
	char* data;
	size_t size;
};

struct StringResourceInfo : public ResourceInfo {
	StringResourceInfo(int32 id, const char* data, const char* name = NULL)
		: ResourceInfo(B_STRING_TYPE, id, data, strlen(data) + 1, name)
	{
	}
};

static const char* testResData1 = "I like strings, especially cellos.";
static const int32 testResSize1 = strlen(testResData1) + 1;
static const int32 testResData2 = 42;
static const int32 testResSize2 = sizeof(int32);
static const char* testResData3 = "application/bread-roll-counter";
static const int32 testResSize3 = strlen(testResData3) + 1;
static const char* testResData4 = "This is a long string. At least longer "
								  "than the first one";
static const int32 testResSize4 = strlen(testResData1) + 1;
static const char* testResData6 = "Short, but true.";
static const int32 testResSize6 = strlen(testResData6) + 1;

static const ResourceInfo testResource1(B_STRING_TYPE, 74, testResData1, testResSize1,
	"a string resource");
static const ResourceInfo testResource2(B_INT32_TYPE, 17, &testResData2, testResSize2,
	"just a resource");
static const ResourceInfo testResource3(B_MIME_STRING_TYPE, 29, testResData3, testResSize3,
	"another resource");
static const ResourceInfo testResource4(B_STRING_TYPE, 75, &testResData4, testResSize4,
	"a second string resource");
static const ResourceInfo testResource5(B_MIME_STRING_TYPE, 74, &testResData1, testResSize1,
	"a string resource");
static const ResourceInfo testResource6(B_STRING_TYPE, 74, &testResData6, testResSize6,
	"a third string resource");

static const StringResourceInfo stringResource1(0, "What?");
static const StringResourceInfo stringResource2(12, "What?", "string 2");
static const StringResourceInfo stringResource3(19, "Who cares?", "string 3");
static const StringResourceInfo stringResource4(23,
	"a little bit longer than "
	"the others",
	"string 4");
static const StringResourceInfo stringResource5(24,
	"a lot longer than "
	"the other strings, but it "
	"it doesn't have a name");
static const StringResourceInfo stringResource6(26, "short");
static const StringResourceInfo stringResource7(27, "");
static const StringResourceInfo stringResource8(123, "the very last resource", "last resource");


static string
get_app_path()
{
	std::string result;
	image_info info;
	int32 cookie = 0;
	bool found = false;
	while (!found && get_next_image_info(0, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE) {
			result = info.name;
			found = true;
		}
	}
	return result;
}

static entry_ref
ref_for(const char* path)
{
	entry_ref ref;
	get_ref_for_path(path, &ref);
	return ref;
}

static entry_ref
get_app_ref()
{
	return ref_for(get_app_path().c_str());
}

static void
add_resource(BResources& resources, const ResourceInfo& resource)
{
	resources.AddResource(resource.type, resource.id, resource.data, resource.size, resource.name);
}


static void
FindStringTest(BResourceStrings& resourceStrings, const ResourceInfo& resource, bool ok)
{
	BString* newString = resourceStrings.NewString(resource.id);
	const char* foundString = resourceStrings.FindString(resource.id);
	if (ok) {
		CPPUNIT_ASSERT(newString != NULL && foundString != NULL);
		CPPUNIT_ASSERT(*newString == (const char*)resource.data);
		CPPUNIT_ASSERT(*newString == foundString);
		delete newString;
	} else {
		CPPUNIT_ASSERT(newString == NULL && foundString == NULL);
	}
}


class ResourceStringsTest : public BasicTest {
	CPPUNIT_TEST_SUITE(ResourceStringsTest);
	CPPUNIT_TEST(DefaultResourceStrings_Constructor_ReturnsOk);
	CPPUNIT_TEST(AppFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(X86ResFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(PpcResFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(ElfFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(PefFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(TestFile1_Constructor_ReturnsOk);
	CPPUNIT_TEST(TestFile2_Constructor_ReturnsOk);
	CPPUNIT_TEST(EmptyFile_Constructor_ReturnsOk);
	CPPUNIT_TEST(NonResourceFile_Constructor_ReturnsError);
	CPPUNIT_TEST(NonExistingFile_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(InvalidArguments_Constructor_ReturnsError);

	CPPUNIT_TEST(AppFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(X86ResFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(PpcResFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(ElfFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(PefFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(TestFile1_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(TestFile2_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(EmptyFile_SetStringFile_ReturnsOk);
	CPPUNIT_TEST(NonResourceFile_SetStringFile_ReturnsError);
	CPPUNIT_TEST(NonExistingFile_SetStringFile_ReturnsNotFound);
	CPPUNIT_TEST(NullRef_SetStringFile_ReturnsOk);

	CPPUNIT_TEST(DefaultResourceStrings_FindString_ReturnsNotFound);
	CPPUNIT_TEST(ExplicitAppFile_FindString_ReturnsNotFound);
	CPPUNIT_TEST(TestFile1_FindString_ReturnsOk);
	CPPUNIT_TEST(TestFile2_FindString_ReturnsOk);
	CPPUNIT_TEST(X86ResFile_FindString_ReturnsOk);
	CPPUNIT_TEST(PpcResFile_FindString_ReturnsOk);
	CPPUNIT_TEST(ElfFile_FindString_ReturnsOk);
	CPPUNIT_TEST(PefFile_FindString_ReturnsOk);
	CPPUNIT_TEST(EmptyFile_FindString_ReturnsNotFound);
	CPPUNIT_TEST(NonResourceFile_FindString_ReturnsNotFound);
	CPPUNIT_TEST(NonExistingFile_FindString_ReturnsNotFound);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		BasicTest::setUp();
		std::string resourcesTestDir(BTestShell::GlobalTestDir());
		resourcesTestDir += "/resources/kits/storage";
		execCommand(std::string ("mkdir ") + testDir + " ; cp " + resourcesTestDir + "/" + x86ResName
			+ " " + resourcesTestDir + "/" + ppcResName + " " + resourcesTestDir + "/" + elfName
			+ " " + resourcesTestDir + "/" + elfNoResName + " " + resourcesTestDir + "/" + pefName
			+ " " + resourcesTestDir + "/" + pefNoResName + " " + testDir + " ; touch " + emptyFile
			+ " ; echo \"That's not a resource file.\" > " + noResFile);
		// prepare the test files
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		BResources resources(&file, true);
		add_resource(resources, stringResource1);
		add_resource(resources, stringResource2);
		add_resource(resources, testResource2);
		add_resource(resources, stringResource3);
		add_resource(resources, stringResource4);
		add_resource(resources, stringResource5);
		add_resource(resources, testResource3);
		resources.Sync();
		file.SetTo(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		resources.SetTo(&file, true);
		add_resource(resources, testResource3);
		add_resource(resources, stringResource4);
		add_resource(resources, stringResource5);
		add_resource(resources, stringResource6);
		add_resource(resources, testResource2);
		add_resource(resources, stringResource7);
		add_resource(resources, stringResource8);
		resources.Sync();
	}

	void tearDown()
	{
		execCommand(std::string ("rm -rf ") + testDir);
		BasicTest::tearDown();
	}

	void DefaultResourceStrings_Constructor_ReturnsOk()
	{
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.GetStringFile(&ref));
	}

	void AppFile_Constructor_ReturnsOk()
	{
		entry_ref ref = get_app_ref();
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void X86ResFile_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(x86ResFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void PpcResFile_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(ppcResFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void ElfFile_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(elfFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void PefFile_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(pefFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void TestFile1_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testFile1, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void TestFile2_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testFile2, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void EmptyFile_Constructor_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(emptyFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void NonResourceFile_Constructor_ReturnsError()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(noResFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT(equals(resourceStrings.InitCheck(), B_ERROR, B_IO_ERROR));
		entry_ref ref2;
		CPPUNIT_ASSERT(equals(resourceStrings.GetStringFile(&ref2), B_ERROR, B_IO_ERROR));
	}

	void NonExistingFile_Constructor_ReturnsNotFound()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(noSuchFile, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.GetStringFile(&ref2));
	}

	void InvalidArguments_Constructor_ReturnsError()
	{
#if !TEST_R5
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testFile1, &ref));
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resourceStrings.GetStringFile(NULL));
#endif
	}

	void AppFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref = get_app_ref();
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void X86ResFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(x86ResFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void PpcResFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(ppcResFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void ElfFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(elfFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void PefFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(pefFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void TestFile1_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testFile1, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void TestFile2_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testFile2, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void EmptyFile_SetStringFile_ReturnsOk()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(emptyFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.GetStringFile(&ref2));
		CPPUNIT_ASSERT(BEntry(&ref, true) == BEntry(&ref2, true));
	}

	void NonResourceFile_SetStringFile_ReturnsError()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(noResFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT(equals(resourceStrings.SetStringFile(&ref), B_ERROR, B_IO_ERROR));
		CPPUNIT_ASSERT(equals(resourceStrings.InitCheck(), B_ERROR, B_IO_ERROR));
		entry_ref ref2;
		CPPUNIT_ASSERT(equals(resourceStrings.GetStringFile(&ref2), B_ERROR, B_IO_ERROR));
	}

	void NonExistingFile_SetStringFile_ReturnsNotFound()
	{
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(noSuchFile, &ref));
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.SetStringFile(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.GetStringFile(&ref2));
	}

	void NullRef_SetStringFile_ReturnsOk()
	{
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.SetStringFile(NULL));
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		entry_ref ref2;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.GetStringFile(&ref2));
	}

	void DefaultResourceStrings_FindString_ReturnsNotFound()
	{
		BResourceStrings resourceStrings;
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void ExplicitAppFile_FindString_ReturnsNotFound()
	{
		entry_ref ref = get_app_ref();
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void TestFile1_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(testFile1);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, true);
		::FindStringTest(resourceStrings, stringResource2, true);
		::FindStringTest(resourceStrings, stringResource3, true);
		::FindStringTest(resourceStrings, stringResource4, true);
		::FindStringTest(resourceStrings, stringResource5, true);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void TestFile2_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(testFile2);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, true);
		::FindStringTest(resourceStrings, stringResource5, true);
		::FindStringTest(resourceStrings, stringResource6, true);
		::FindStringTest(resourceStrings, stringResource7, true);
		::FindStringTest(resourceStrings, stringResource8, true);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void X86ResFile_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(x86ResFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, true);
	}

	void PpcResFile_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(ppcResFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, true);
	}

	void ElfFile_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(elfFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, true);
	}

	void PefFile_FindString_ReturnsOk()
	{
		entry_ref ref = ref_for(pefFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, true);
	}

	void EmptyFile_FindString_ReturnsNotFound()
	{
		entry_ref ref = ref_for(emptyFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void NonResourceFile_FindString_ReturnsNotFound()
	{
		entry_ref ref = ref_for(noResFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT(equals(resourceStrings.InitCheck(), B_ERROR, B_IO_ERROR));
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}

	void NonExistingFile_FindString_ReturnsNotFound()
	{
		entry_ref ref = ref_for(noSuchFile);
		BResourceStrings resourceStrings(ref);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resourceStrings.InitCheck());
		::FindStringTest(resourceStrings, stringResource1, false);
		::FindStringTest(resourceStrings, stringResource2, false);
		::FindStringTest(resourceStrings, stringResource3, false);
		::FindStringTest(resourceStrings, stringResource4, false);
		::FindStringTest(resourceStrings, stringResource5, false);
		::FindStringTest(resourceStrings, stringResource6, false);
		::FindStringTest(resourceStrings, stringResource7, false);
		::FindStringTest(resourceStrings, stringResource8, false);
		::FindStringTest(resourceStrings, testResource1, false);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ResourceStringsTest, getTestSuiteName());
