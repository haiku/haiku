/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <ByteOrder.h>
#include <File.h>
#include <Mime.h>
#include <Resources.h>
#include <StorageDefs.h>
#include <String.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <TestShell.h>
#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"


static const char* testDir = "/tmp/testDir";
static const char* x86ResFile = "/tmp/testDir/x86.rsrc";
static const char* ppcResFile = "/tmp/testDir/ppc.rsrc";
static const char* elfFile = "/tmp/testDir/elf";
static const char* elfNoResFile = "/tmp/testDir/elf-no-res";
static const char* pefFile = "/tmp/testDir/pef";
static const char* pefNoResFile = "/tmp/testDir/pef-no-res";
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

// helper class: maintains a ResourceInfo set
struct ResourceSet {
	typedef std::vector<const ResourceInfo*> ResInfoSet;

	ResourceSet()
		: fResources()
	{
	}

	ResourceSet(const ResourceSet& resourceSet)
		: fResources()
	{
		fResources = resourceSet.fResources;
	}

	void add(const ResourceInfo* info)
	{
		if (info) {
			remove(info->type, info->id);
			fResources.push_back(info);
		}
	}

	void remove(const ResourceInfo* info)
	{
		if (info)
			remove(info->type, info->id);
	}

	void remove(type_code type, int32 id)
	{
		for (ResInfoSet::iterator it = fResources.begin(); it != fResources.end(); ++it) {
			if ((*it)->type == type && (*it)->id == id) {
				fResources.erase(it);
				break;
			}
		}
	}

	int32 count() const { return fResources.size(); }

	const ResourceInfo* at(int32 index) const
	{
		if (index >= 0 && index < count())
			return fResources[index];
		return NULL;
	}

	const ResourceInfo* find(type_code type, int32 id) const
	{
		for (int32 i = 0; i < count(); i++) {
			if (fResources[i]->type == type && fResources[i]->id == id)
				return fResources[i];
		}
		return NULL;
	}

	ResInfoSet fResources;
};

// Forward declarations
static void ReadResTest(BResources& resources, const ResourceInfo& info, bool exists);
static void ReadBadResTest(BResources& resources, const ResourceInfo& info);

static void
ReadResTest(BResources& resources, const ResourceInfo& info, bool exists)
{
	if (exists) {
		// test an existing resource
		// HasResource()
		CPPUNIT_ASSERT(resources.HasResource(info.type, info.id) == true);
		CPPUNIT_ASSERT(resources.HasResource(info.type, info.name) == true);
		// GetResourceInfo()
		const char* name;
		size_t length;
		int32 id;
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.id, &name, &length) == true);
		CPPUNIT_ASSERT(name == NULL && info.name == NULL
			|| name != NULL && info.name != NULL && !strcmp(name, info.name));
		CPPUNIT_ASSERT(length == info.size);
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.name, &id, &length) == true);
		CPPUNIT_ASSERT(id == info.id);
		CPPUNIT_ASSERT(length == info.size);
		// LoadResource
		size_t length1;
		size_t length2;
		const void* data1 = resources.LoadResource(info.type, info.id, &length1);
		const void* data2 = resources.LoadResource(info.type, info.name, &length2);
		CPPUNIT_ASSERT(length1 == info.size && length2 == info.size);
		CPPUNIT_ASSERT(data1 != NULL && data1 == data2);
		CPPUNIT_ASSERT(!memcmp(data1, info.data, info.size));
		// GetResourceInfo()
		type_code type = 0;
		id = 0;
		length = 0;
		name = NULL;
		CPPUNIT_ASSERT(resources.GetResourceInfo(data1, &type, &id, &length, &name) == true);
		CPPUNIT_ASSERT(type == info.type);
		CPPUNIT_ASSERT(id == info.id);
		CPPUNIT_ASSERT(length == info.size);
		CPPUNIT_ASSERT(name == NULL && info.name == NULL
			|| name != NULL && info.name != NULL && !strcmp(name, info.name));
		// ReadResource()
		const int32 bufferSize = 1024;
		char buffer[bufferSize];
		memset(buffer, 0x0f, bufferSize);
		// read past the end
		status_t error
			= resources.ReadResource(info.type, info.id, buffer, info.size + 2, bufferSize);
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
		for (int32 i = 0; i < bufferSize; i++)
			CPPUNIT_ASSERT(buffer[i] == 0x0f);
		// read 2 bytes from the middle
		int32 offset = (info.size - 2) / 2;
		error = resources.ReadResource(info.type, info.id, buffer, offset, 2);
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
		CPPUNIT_ASSERT(!memcmp(buffer, (const char*)info.data + offset, 2));
		for (int32 i = 2; i < bufferSize; i++)
			CPPUNIT_ASSERT(buffer[i] == 0x0f);
		// read the whole chunk
		error = resources.ReadResource(info.type, info.id, buffer, 0, bufferSize);
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
		CPPUNIT_ASSERT(!memcmp(buffer, info.data, info.size));
		// FindResource()
		size_t lengthFound1;
		size_t lengthFound2;
		void* dataFound1 = resources.FindResource(info.type, info.id, &lengthFound1);
		void* dataFound2 = resources.FindResource(info.type, info.name, &lengthFound2);
		CPPUNIT_ASSERT(dataFound1 != NULL && dataFound2 != NULL);
		CPPUNIT_ASSERT(lengthFound1 == info.size && lengthFound2 == info.size);
		CPPUNIT_ASSERT(!memcmp(dataFound1, info.data, info.size));
		CPPUNIT_ASSERT(!memcmp(dataFound2, info.data, info.size));
		free(dataFound1);
		free(dataFound2);
	} else {
		// test a non-existing resource
		// HasResource()
		CPPUNIT_ASSERT(resources.HasResource(info.type, info.id) == false);
		CPPUNIT_ASSERT(resources.HasResource(info.type, info.name) == false);
		// GetResourceInfo()
		const char* name;
		size_t length;
		int32 id;
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.id, &name, &length) == false);
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.name, &id, &length) == false);
		// LoadResource
		size_t length1;
		size_t length2;
		const void* data1 = resources.LoadResource(info.type, info.id, &length1);
		const void* data2 = resources.LoadResource(info.type, info.name, &length2);
		CPPUNIT_ASSERT(data1 == NULL && data2 == NULL);
		// ReadResource()
		const int32 bufferSize = 1024;
		char buffer[bufferSize];
		status_t error = resources.ReadResource(info.type, info.id, buffer, 0, bufferSize);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, error);
		// FindResource()
		size_t lengthFound1;
		size_t lengthFound2;
		void* dataFound1 = resources.FindResource(info.type, info.id, &lengthFound1);
		void* dataFound2 = resources.FindResource(info.type, info.name, &lengthFound2);
		CPPUNIT_ASSERT(dataFound1 == NULL && dataFound2 == NULL);
	}
}

static void
ReadBadResTest(BResources& resources, const ResourceInfo& info)
{
	// HasResource()
	CPPUNIT_ASSERT(resources.HasResource(info.type, info.id) == false);
	CPPUNIT_ASSERT(resources.HasResource(info.type, info.name) == false);
	// GetResourceInfo()
	type_code type;
	const char* name;
	size_t length;
	int32 id;
	CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.id, &name, &length) == false);
	CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.name, &id, &length) == false);
	CPPUNIT_ASSERT(resources.GetResourceInfo(0, &type, &id, &name, &length) == false);
	CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, 0, &id, &name, &length) == false);
	// LoadResource
	size_t length1;
	size_t length2;
	const void* data1 = resources.LoadResource(info.type, info.id, &length1);
	const void* data2 = resources.LoadResource(info.type, info.name, &length2);
	CPPUNIT_ASSERT(data1 == NULL && data2 == NULL);
	// ReadResource()
	const int32 bufferSize = 1024;
	char buffer[bufferSize];
	status_t error = resources.ReadResource(info.type, info.id, buffer, 0, bufferSize);
	CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, error);
	// FindResource()
	size_t lengthFound1;
	size_t lengthFound2;
	void* dataFound1 = resources.FindResource(info.type, info.id, &lengthFound1);
	void* dataFound2 = resources.FindResource(info.type, info.name, &lengthFound2);
	CPPUNIT_ASSERT(dataFound1 == NULL && dataFound2 == NULL);
}

static void
CompareResources(BResources& resources, const ResourceSet& resourceSet);

static void
AddResource(BResources& resources, ResourceSet& resourceSet, const ResourceInfo& info)
{
	CPPUNIT_ASSERT(
		resources.AddResource(info.type, info.id, info.data, info.size, info.name) == B_OK);
	resourceSet.add(&info);
	// check the resource set after each operation
	CompareResources(resources, resourceSet);
}

static void
AddResources(BResources& resources, const ResourceInfo** infos, int32 count)
{
	for (int32 i = 0; i < count; i++) {
		const ResourceInfo& info = *infos[i];
		CPPUNIT_ASSERT(
			resources.AddResource(info.type, info.id, info.data, info.size, info.name) == B_OK);
	}
}

static void
RemoveResource(BResources& resources, ResourceSet& resourceSet, const ResourceInfo& info, bool firstVersion)
{
	resourceSet.remove(&info);
	if (firstVersion) {
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.RemoveResource(info.type, info.id));
	} else {
		size_t size;
		const void* data = resources.LoadResource(info.type, info.id, &size);
		CPPUNIT_ASSERT(data != NULL);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.RemoveResource(data));
	}
	// check the resource set after each operation
	CompareResources(resources, resourceSet);
}

static void
CompareResources(BResources& resources, const ResourceSet& resourceSet)
{
	// compare the count
	int32 count = resourceSet.count();
	{
		type_code type;
		int32 id;
		const char *name;
		size_t length;
		CPPUNIT_ASSERT( resources.GetResourceInfo(count, &type, &id,
												  &name, &length) == false );
	}
	// => resources contains at most count resources. If it contains all the
	// resources that are in resourceSet, then the sets are equal.
	for (int32 i = 0; i < count; i++) {
		const ResourceInfo &info = *resourceSet.at(i);
		const char *name;
		size_t length;
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.id, &name,
												  &length) == true );
		CPPUNIT_ASSERT( name == NULL && info.name == NULL
						|| name != NULL && info.name != NULL
						   && !strcmp(name, info.name) );
		CPPUNIT_ASSERT( length == info.size );
		const void *data = resources.LoadResource(info.type, info.id, &length);
		CPPUNIT_ASSERT( data != NULL && length == info.size );
		CPPUNIT_ASSERT( !memcmp(data, info.data, info.size) );
	}
}

// CompareFiles
static void
CompareFiles(BFile& file1, BFile& file2)
{
	off_t size1;
	off_t size2;
	CPPUNIT_ASSERT_EQUAL(B_OK, file1.GetSize(&size1));
	CPPUNIT_ASSERT_EQUAL(B_OK, file2.GetSize(&size2));
	CPPUNIT_ASSERT(size1 == size2);
	char* buffer1 = new char[size1];
	char* buffer2 = new char[size2];
	CPPUNIT_ASSERT(file1.ReadAt(0, buffer1, size1) == size1);
	CPPUNIT_ASSERT(file2.ReadAt(0, buffer2, size2) == size2);
	CPPUNIT_ASSERT(!memcmp(buffer1, buffer2, size1));
	delete[] buffer1;
	delete[] buffer2;
}

class ResourcesTest : public BasicTest {
	CPPUNIT_TEST_SUITE(ResourcesTest);
	CPPUNIT_TEST(ReadOnly_SetTo_x86ResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_ppcResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_elfFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_elfNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_pefFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_pefNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_emptyFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnly_SetTo_noResFile_ReturnsIoError);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_x86ResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_ppcResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_elfFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_elfNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_pefFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_pefNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_emptyFile_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyClobber_SetTo_noResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_x86ResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_ppcResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_elfFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_elfNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_pefFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_pefNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_emptyFile_ReturnsOk);
	CPPUNIT_TEST(ReadWrite_SetTo_noResFile_ReturnsIoError);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_x86ResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_ppcResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_elfFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_elfNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_pefFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_pefNoResFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_emptyFile_ReturnsOk);
	CPPUNIT_TEST(ReadWriteClobber_SetTo_noResFile_ReturnsOk);
	CPPUNIT_TEST(UninitializedFile_SetTo_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitializedFile_SetTo_ReturnsEntryNotFound);
	CPPUNIT_TEST(NullFile_SetTo_ReturnsOk);

	CPPUNIT_TEST(x86_Read_ReturnsOk);
	CPPUNIT_TEST(Ppc_Read_ReturnsOk);
	CPPUNIT_TEST(ElfWithResources_Read_ReturnsOk);
	CPPUNIT_TEST(ElfNoResources_Read_ReturnsNotFound);
	CPPUNIT_TEST(PefWithResources_Read_ReturnsOk);
	CPPUNIT_TEST(PefNoResources_Read_ReturnsNotFound);
	CPPUNIT_TEST(InitializedResources_Preload_ReturnsOk);
	CPPUNIT_TEST(UninitializedResources_Preload_ReturnsOk);
	CPPUNIT_TEST(InitializedResources_GetResourceInfoByIndex_ReturnsOk);
	CPPUNIT_TEST(InitializedResources_GetResourceInfoByTypeIndex_ReturnsOk);
	CPPUNIT_TEST(NonResourceFile_Read_ReturnsError);
	CPPUNIT_TEST(UninitializedFile_Read_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitializedFile_Read_ReturnsEntryNotFound);
	CPPUNIT_TEST(NullFile_Read_ReturnsOk);
	CPPUNIT_TEST(InvalidArguments_Read_ReturnsError);

	CPPUNIT_TEST(ModifiedResources_Sync_ReturnsOk);
	CPPUNIT_TEST(ReadOnlyFile_Sync_ReturnsNotAllowed);
	CPPUNIT_TEST(UninitializedResources_Sync_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitializedResources_Sync_ReturnsNoInit);

	CPPUNIT_TEST(EmptyFile_Merge_ReturnsOk);
	CPPUNIT_TEST(NonEmptyFile_Merge_ReturnsOk);
	CPPUNIT_TEST(InvalidArguments_Merge_ReturnsError);
	CPPUNIT_TEST(UninitializedResources_Merge_ReturnsOk);
	CPPUNIT_TEST(BadlyInitializedResources_Merge_ReturnsOk);

	CPPUNIT_TEST(EmptyFile_WriteTo_ReturnsOk);
	CPPUNIT_TEST(NonResourceFile_WriteTo_ReturnsError);
	CPPUNIT_TEST(ElfFile_WriteTo_ReturnsOk);
	CPPUNIT_TEST(ModifiedResources_WriteTo_ReturnsOk);
	CPPUNIT_TEST(UninitializedResources_WriteTo_ReturnsOk);
	CPPUNIT_TEST(NullFile_WriteTo_ReturnsError);

	CPPUNIT_TEST(EmptyResources_AddCompareX86_ReturnsOk);
	CPPUNIT_TEST(NonEmptyResources_RemoveAllCompareEmpty_ReturnsOk);
	CPPUNIT_TEST(NonEmptyResources_RemoveByPointer_ReturnsOk);
	CPPUNIT_TEST(InitializedResources_ArbitraryAddRemove_ReturnsOk);
	CPPUNIT_TEST(InvalidArguments_AddRemove_ReturnsError);
	CPPUNIT_TEST(UninitializedResources_AddRemove_ReturnsOk);

	CPPUNIT_TEST(InitializedResource_Modify_ReturnsOk);
	CPPUNIT_TEST(InvalidArguments_Modify_ReturnsError);
	CPPUNIT_TEST(UninitializedResources_Modify_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		BasicTest::setUp();
		BString unescapedTestDir(BTestShell::GlobalTestDir());
		unescapedTestDir.CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
		string resourcesTestDir(unescapedTestDir.String());
		resourcesTestDir += "/resources/kits/storage";
		execCommand(std::string ("mkdir ") + testDir + " ; cp " + resourcesTestDir + "/" + x86ResName
			+ " " + resourcesTestDir + "/" + ppcResName + " " + resourcesTestDir + "/" + elfName
			+ " " + resourcesTestDir + "/" + elfNoResName + " " + resourcesTestDir + "/" + pefName
			+ " " + resourcesTestDir + "/" + pefNoResName + " " + testDir + " ; touch " + emptyFile
			+ " ; echo \"That's not a resource file.\" > " + noResFile);
	}

	void tearDown()
	{
		execCommand(std::string ("rm -rf ") + testDir);
		BasicTest::tearDown();
	}

	void ReadOnly_SetTo_x86ResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_ppcResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_elfFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_elfNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_pefFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_pefNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_emptyFile_ReturnsOk()
	{
		BResources resources;
		BFile file(emptyFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadOnly_SetTo_noResFile_ReturnsIoError()
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(equals(resources.SetTo(&file, false), B_ERROR, B_IO_ERROR));
	}

	void ReadOnlyClobber_SetTo_x86ResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_ppcResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_elfFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_elfNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_pefFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_pefNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_emptyFile_ReturnsOk()
	{
		BResources resources;
		BFile file(emptyFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadOnlyClobber_SetTo_noResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void ReadWrite_SetTo_x86ResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_ppcResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_elfFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_elfNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_pefFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_pefNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_emptyFile_ReturnsOk()
	{
		BResources resources;
		BFile file(emptyFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
	}

	void ReadWrite_SetTo_noResFile_ReturnsIoError()
	{
		BResources resources;
		BFile file(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(equals(resources.SetTo(&file, false), B_ERROR, B_IO_ERROR));
	}

	void ReadWriteClobber_SetTo_x86ResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_ppcResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_elfFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_elfNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_pefFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_pefNoResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_emptyFile_ReturnsOk()
	{
		BResources resources;
		BFile file(emptyFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
	}

	void ReadWriteClobber_SetTo_noResFile_ReturnsOk()
	{
		BResources resources;
		BFile file(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
	}

	void UninitializedFile_SetTo_ReturnsNoInit()
	{
		BResources resources;
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, resources.SetTo(&file, false));
	}

	void BadlyInitializedFile_SetTo_ReturnsEntryNotFound()
	{
		BResources resources;
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resources.SetTo(&file, false));
	}

	void NullFile_SetTo_ReturnsOk()
	{
		BResources resources;
		// R5: A NULL file is B_OK!
		//	CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.SetTo(NULL, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo((BFile*)NULL, false));
	}

	void x86_Read_ReturnsOk()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void Ppc_Read_ReturnsOk()
	{
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void ElfWithResources_Read_ReturnsOk()
	{
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void ElfNoResources_Read_ReturnsNotFound()
	{
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, false);
		ReadResTest(resources, testResource2, false);
		ReadResTest(resources, testResource3, false);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void PefWithResources_Read_ReturnsOk()
	{
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void PefNoResources_Read_ReturnsNotFound()
	{
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		ReadResTest(resources, testResource1, false);
		ReadResTest(resources, testResource2, false);
		ReadResTest(resources, testResource3, false);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void InitializedResources_Preload_ReturnsOk()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		// non existing type
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.PreloadResourceType(B_MESSENGER_TYPE));
		// int32 type
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.PreloadResourceType(B_INT32_TYPE));
		// all types
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.PreloadResourceType());
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	void UninitializedResources_Preload_ReturnsOk()
	{
		BResources resources;
		// int32 type
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.PreloadResourceType(B_INT32_TYPE));
		// all types
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.PreloadResourceType());
	}

	void InitializedResources_GetResourceInfoByIndex_ReturnsOk()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo* resourceInfos[] = {&testResource1, &testResource2, &testResource3};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo& info = *resourceInfos[i];
			type_code type;
			int32 id;
			const char* name;
			size_t length;
			CPPUNIT_ASSERT(resources.GetResourceInfo(i, &type, &id, &name, &length) == true);
			CPPUNIT_ASSERT(id == info.id);
			CPPUNIT_ASSERT(type == info.type);
			CPPUNIT_ASSERT(name == NULL && info.name == NULL
				|| name != NULL && info.name != NULL && !strcmp(name, info.name));
			CPPUNIT_ASSERT(length == info.size);
		}
		type_code type;
		int32 id;
		const char* name;
		size_t length;
		CPPUNIT_ASSERT(
			resources.GetResourceInfo(resourceCount, &type, &id, &name, &length) == false);
	}

	void InitializedResources_GetResourceInfoByTypeIndex_ReturnsOk()
	{
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		CPPUNIT_ASSERT(resources.AddResource(testResource4.type, testResource4.id,
						   testResource4.data, testResource4.size, testResource4.name)
			== B_OK);

		const ResourceInfo* resourceInfos[] = {&testResource1, &testResource4};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		type_code type = resourceInfos[0]->type;
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo& info = *resourceInfos[i];
			int32 id;
			const char* name;
			size_t length;
			CPPUNIT_ASSERT(resources.GetResourceInfo(type, i, &id, &name, &length) == true);
			CPPUNIT_ASSERT(id == info.id);
			CPPUNIT_ASSERT(type == info.type);
			CPPUNIT_ASSERT(name == NULL && info.name == NULL
				|| name != NULL && info.name != NULL && !strcmp(name, info.name));
			CPPUNIT_ASSERT(length == info.size);
		}
		int32 id;
		const char* name;
		size_t length;
		CPPUNIT_ASSERT(
			resources.GetResourceInfo(type, resourceCount, &id, &name, &length) == false);
	}

	void NonResourceFile_Read_ReturnsError()
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(equals(resources.SetTo(&file, false), B_ERROR, B_IO_ERROR));
		ReadBadResTest(resources, testResource1);
	}

	void UninitializedFile_Read_ReturnsNoInit()
	{
		BResources resources;
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, resources.SetTo(&file, false));
		ReadBadResTest(resources, testResource1);
	}

	void BadlyInitializedFile_Read_ReturnsEntryNotFound()
	{
		BResources resources;
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resources.SetTo(&file, false));
		ReadBadResTest(resources, testResource1);
	}

	void NullFile_Read_ReturnsOk()
	{
		BResources resources;
		// R5: A NULL file is B_OK!
		//	CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.SetTo(NULL, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo((BFile*)NULL, false));
		ReadBadResTest(resources, testResource1);
	}

	void InvalidArguments_Read_ReturnsError()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo& info = testResource1;
		// GetResourceInfo()
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.id, NULL, NULL) == true);
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, info.name, NULL, NULL) == true);
		CPPUNIT_ASSERT(
			resources.GetResourceInfo(int32(0), (type_code*)NULL, NULL, NULL, NULL) == true);
		CPPUNIT_ASSERT(resources.GetResourceInfo(info.type, 0, NULL, NULL, NULL) == true);
		// LoadResource
		const void* data1 = resources.LoadResource(info.type, info.id, NULL);
		const void* data2 = resources.LoadResource(info.type, info.name, NULL);
		CPPUNIT_ASSERT(data1 != NULL && data2 != NULL);
		// ReadResource()
		const int32 bufferSize = 1024;
		status_t error = resources.ReadResource(info.type, info.id, NULL, 0, bufferSize);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, error);
		// FindResource()
		void* dataFound1 = resources.FindResource(info.type, info.id, NULL);
		void* dataFound2 = resources.FindResource(info.type, info.name, NULL);
		CPPUNIT_ASSERT(dataFound1 != NULL && dataFound2 != NULL);
		free(dataFound1);
		free(dataFound2);
	}

	void ModifiedResources_Sync_ReturnsOk()
	{
		execCommand(std::string ("cp ") + x86ResFile + " " + testFile1);
		{
			BFile file(testFile1, B_READ_WRITE);
			CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
			BResources resources;
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
			// add a resource and Sync()
			CPPUNIT_ASSERT(resources.AddResource(testResource4.type, testResource4.id,
							   testResource4.data, testResource4.size, testResource4.name)
				== B_OK);
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
			// remove a resource and Sync()
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.RemoveResource(testResource1.type, testResource1.id));
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.Sync());
		}
	}

	void ReadOnlyFile_Sync_ReturnsNotAllowed()
	{
		execCommand(std::string ("cp ") + x86ResFile + " " + testFile1);
		BFile file(testFile1, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, resources.Sync());
	}

	void UninitializedResources_Sync_ReturnsNoInit()
	{
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, resources.Sync());
	}

	void BadlyInitializedResources_Sync_ReturnsNoInit()
	{
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resources.SetTo(&file, false));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, resources.Sync());
	}

	void EmptyFile_Merge_ReturnsOk()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.MergeFrom(&file2));
		ResourceSet resourceSet;
		resourceSet.add(&testResource1);
		resourceSet.add(&testResource2);
		resourceSet.add(&testResource3);
		CompareResources(resources, resourceSet);
	}

	void NonEmptyFile_Merge_ReturnsOk()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		// add some resources
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource4);
		AddResource(resources, resourceSet, testResource5);
		AddResource(resources, resourceSet, testResource6);
		// merge
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.MergeFrom(&file2));
		resourceSet.add(&testResource1); // replaces testResource6
		resourceSet.add(&testResource2);
		resourceSet.add(&testResource3);
		CompareResources(resources, resourceSet);
	}

	void InvalidArguments_Merge_ReturnsError()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		BFile file2(noResFile, B_READ_ONLY);
		// non-resource file
		CPPUNIT_ASSERT_EQUAL(B_IO_ERROR, resources.MergeFrom(&file2));
		// NULL file
// R5: crashs
#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.MergeFrom(NULL));
#endif
	}

	void UninitializedResources_Merge_ReturnsOk()
	{
		BResources resources;
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.MergeFrom(&file2));
	}

	void BadlyInitializedResources_Merge_ReturnsOk()
	{
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, resources.SetTo(&file, true));
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.MergeFrom(&file2));
	}

	void EmptyFile_WriteTo_ReturnsOk()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteTo(&file2));
		CPPUNIT_ASSERT(resources.File() == file2);
	}

	void NonResourceFile_WriteTo_ReturnsError()
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		BFile file2(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_IO_ERROR, resources.WriteTo(&file2));
		CPPUNIT_ASSERT(resources.File() == file2);
	}

	void ElfFile_WriteTo_ReturnsOk()
	{
		execCommand(std::string ("cp ") + elfNoResFile + " " + testFile1);
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteTo(&file2));
		CPPUNIT_ASSERT(resources.File() == file2);
	}

	void ModifiedResources_WriteTo_ReturnsOk()
	{
		BFile file(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		// add a resource
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource1);
		// write to another file
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteTo(&file2));
		CPPUNIT_ASSERT(resources.File() == file2);
		// check, whether the first file has been modified -- it shouldn't be
		off_t fileSize = 0;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&fileSize));
		CPPUNIT_ASSERT(fileSize == 0);
	}

	void UninitializedResources_WriteTo_ReturnsOk()
	{
		execCommand(std::string ("cp ") + elfNoResFile + " " + testFile1);
		BResources resources;
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteTo(&file2));
		CPPUNIT_ASSERT(resources.File() == file2);
	}

	void NullFile_WriteTo_ReturnsError()
	{
		BFile file(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.WriteTo(NULL));
#endif
	}

	void EmptyResources_AddCompareX86_ReturnsOk()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		const ResourceInfo* resourceInfos[] = {&testResource1, &testResource2, &testResource3};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		AddResources(resources, resourceInfos, resourceCount);

		BFile file1(testFile1, B_READ_ONLY);
		BFile file2(x86ResFile, B_READ_ONLY);
		CompareFiles(file1, file2);
	}

	void NonEmptyResources_RemoveAllCompareEmpty_ReturnsOk()
	{
		execCommand(std::string ("cp ") + x86ResFile + " " + testFile1);
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo* resourceInfos[] = {&testResource1, &testResource2, &testResource3};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo& info = *resourceInfos[i];
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.RemoveResource(info.type, info.id));
		}
		BFile file2(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		BResources resources2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources2.SetTo(&file2, true));

		BFile file1(testFile1, B_READ_ONLY);
		BFile resFile2(testFile2, B_READ_ONLY);
		CompareFiles(file1, resFile2);
	}

	void NonEmptyResources_RemoveByPointer_ReturnsOk()
	{
		execCommand(std::string ("cp ") + x86ResFile + " " + testFile1);
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo* resourceInfos[] = {&testResource1, &testResource2, &testResource3};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo& info = *resourceInfos[i];
			size_t size;
			const void* data = resources.LoadResource(info.type, info.id, &size);
			CPPUNIT_ASSERT(data != NULL);
			CPPUNIT_ASSERT_EQUAL(B_OK, resources.RemoveResource(data));
		}
		BFile file2(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		BResources resources2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources2.SetTo(&file2, true));

		BFile file1(testFile1, B_READ_ONLY);
		BFile resFile2(testFile2, B_READ_ONLY);
		CompareFiles(file1, resFile2);
	}

	void InitializedResources_ArbitraryAddRemove_ReturnsOk()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource1);
		AddResource(resources, resourceSet, testResource2);
		RemoveResource(resources, resourceSet, testResource1, true);
		AddResource(resources, resourceSet, testResource3);
		AddResource(resources, resourceSet, testResource4);
		RemoveResource(resources, resourceSet, testResource3, false);
		AddResource(resources, resourceSet, testResource5);
		AddResource(resources, resourceSet, testResource1);
		AddResource(resources, resourceSet, testResource6); // replaces 1
		RemoveResource(resources, resourceSet, testResource2, true);
		RemoveResource(resources, resourceSet, testResource5, false);
		RemoveResource(resources, resourceSet, testResource6, true);
		RemoveResource(resources, resourceSet, testResource4, false);
	}

	void InvalidArguments_AddRemove_ReturnsError()
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, true));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.RemoveResource(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.RemoveResource(B_INT32_TYPE, 0));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.AddResource(B_INT32_TYPE, 0, NULL, 7, "Hey!"));
	}

	void UninitializedResources_AddRemove_ReturnsOk()
	{
		BResources resources;
		const ResourceInfo& info = testResource1;
		CPPUNIT_ASSERT_EQUAL(B_OK,
			resources.AddResource(info.type, info.id, info.data, info.size, info.name));
	}

	void InitializedResource_Modify_ReturnsOk()
	{
		execCommand(std::string ("cp ") + x86ResFile + " " + testFile1);
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo& info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		// write at the beginning of the resource
		char writeBuffer[1024];
		for (int32 i = 0; i < 1024; i++)
			writeBuffer[i] = i & 0xff;
		char readBuffer[1024];
		memset(readBuffer, 0, 1024);
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteResource(type, id, writeBuffer, 0, 10));
		size_t length;
		const void* data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT(data != NULL && length == info.size);
		CPPUNIT_ASSERT(!memcmp(data, writeBuffer, 10));
		CPPUNIT_ASSERT(!memcmp((const char*)data + 10, info.data + 10, info.size - 10));
		// completely overwrite the resource with the former data
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteResource(type, id, info.data, 0, info.size));
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT(data != NULL && length == info.size);
		CPPUNIT_ASSERT(!memcmp(data, info.data, info.size));
		// write something in the middle
		size_t offset = (info.size - 2) / 2;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.WriteResource(type, id, writeBuffer, offset, 2));
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT(data != NULL && length == info.size);
		CPPUNIT_ASSERT(!memcmp(data, info.data, offset));
		CPPUNIT_ASSERT(!memcmp((const char*)data + offset, writeBuffer, 2));
		CPPUNIT_ASSERT(!memcmp((const char*)data + offset + 2, info.data + offset + 2,
			info.size - offset - 2));
		// write starting inside the resource, but extending it
		size_t writeSize = info.size + 3;
		CPPUNIT_ASSERT_EQUAL(B_OK,
			resources.WriteResource(type, id, writeBuffer, offset, writeSize));
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT(data != NULL && length == (size_t)offset + writeSize);
		CPPUNIT_ASSERT(!memcmp(data, info.data, offset));
		CPPUNIT_ASSERT(!memcmp((const char*)data + offset, writeBuffer, writeSize));
		// write past the end of the resource
		size_t newOffset = length + 30;
		size_t newWriteSize = 17;
		CPPUNIT_ASSERT_EQUAL(B_OK,
			resources.WriteResource(type, id, writeBuffer, newOffset, newWriteSize));
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT(data != NULL && length == newOffset + newWriteSize);
		CPPUNIT_ASSERT(!memcmp(data, info.data, offset));
		CPPUNIT_ASSERT(!memcmp((const char*)data + offset, writeBuffer, writeSize));
		// [offset + writeSize, newOffset) unspecified
		CPPUNIT_ASSERT(!memcmp((const char*)data + newOffset, writeBuffer, newWriteSize));
	}

	void InvalidArguments_Modify_ReturnsError()
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BResources resources;
		CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file, false));
		const ResourceInfo& info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		// NULL buffer
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.WriteResource(type, id, NULL, 0, 10));
		// non existing resource
		char writeBuffer[1024];
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE,
			resources.WriteResource(B_MESSENGER_TYPE, 0, writeBuffer, 0, 10));
	}

	void UninitializedResources_Modify_ReturnsError()
	{
		BResources resources;
		const ResourceInfo& info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		char writeBuffer[1024];
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, resources.WriteResource(type, id, writeBuffer, 0, 10));
	}
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ResourcesTest, getTestSuiteName());
