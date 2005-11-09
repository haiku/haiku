// ResourcesTest.cpp

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <ByteOrder.h>
#include <File.h>
#include <Mime.h>
#include <Resources.h>
#include <String.h>
#include <TypeConstants.h>
#include <TestShell.h>

#include "ResourcesTest.h"

static const char *testDir		= "/tmp/testDir";
static const char *x86ResFile	= "/tmp/testDir/x86.rsrc";
static const char *ppcResFile	= "/tmp/testDir/ppc.rsrc";
static const char *elfFile		= "/tmp/testDir/elf";
static const char *elfNoResFile	= "/tmp/testDir/elf-no-res";
static const char *pefFile		= "/tmp/testDir/pef";
static const char *pefNoResFile	= "/tmp/testDir/pef-no-res";
static const char *emptyFile	= "/tmp/testDir/empty-file";
static const char *noResFile	= "/tmp/testDir/no-res-file";
static const char *testFile1	= "/tmp/testDir/testFile1";
static const char *testFile2	= "/tmp/testDir/testFile2";
static const char *noSuchFile	= "/tmp/testDir/no-such-file";
static const char *x86ResName	= "x86.rsrc";
static const char *ppcResName	= "ppc.rsrc";
static const char *elfName		= "elf";
static const char *elfNoResName	= "elf-no-res";
static const char *pefName		= "pef";
static const char *pefNoResName	= "pef-no-res";

static const int32 kEmptyResourceFileSize	= 1984;

struct ResourceInfo {
	ResourceInfo(type_code type, int32 id, const void *data, size_t size,
				 const char *name = NULL)
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

	type_code	type;
	int32		id;
	char		*name;
	char		*data;
	size_t		size;
};

static const char *testResData1 = "I like strings, especially cellos.";
static const int32 testResSize1 = strlen(testResData1) + 1;
static const int32 testResData2 = 42;
static const int32 testResSize2 = sizeof(int32);
static const char *testResData3 = "application/bread-roll-counter";
static const int32 testResSize3 = strlen(testResData3) + 1;
static const char *testResData4 = "This is a long string. At least longer "
								  "than the first one";
static const int32 testResSize4 = strlen(testResData1) + 1;
static const char *testResData6 = "Short, but true.";
static const int32 testResSize6 = strlen(testResData6) + 1;

static const ResourceInfo testResource1(B_STRING_TYPE, 74, testResData1,
										testResSize1, "a string resource");
static const ResourceInfo testResource2(B_INT32_TYPE, 17, &testResData2,
										testResSize2, "just a resource");
static const ResourceInfo testResource3(B_MIME_STRING_TYPE, 29, testResData3,
										testResSize3, "another resource");
static const ResourceInfo testResource4(B_STRING_TYPE, 75, &testResData4,
										testResSize4,
										"a second string resource");
static const ResourceInfo testResource5(B_MIME_STRING_TYPE, 74, &testResData1,
										testResSize1, "a string resource");
static const ResourceInfo testResource6(B_STRING_TYPE, 74, &testResData6,
										testResSize6,
										"a third string resource");

// helper class: maintains a ResourceInfo set
struct ResourceSet {
	typedef vector<const ResourceInfo*> ResInfoSet;

	ResourceSet() : fResources() { }

	ResourceSet(const ResourceSet &resourceSet)
		: fResources()
	{
		fResources = resourceSet.fResources;
	}

	void add(const ResourceInfo *info)
	{
		if (info) {
			remove(info->type, info->id);
			fResources.insert(fResources.end(), info);
		}
	}

	void remove(const ResourceInfo *info)
	{
		if (info)
			remove(info->type, info->id);
	}

	void remove(type_code type, int32 id)
	{
		for (ResInfoSet::iterator it = fResources.begin();
			 it != fResources.end();
			 it++) {
			const ResourceInfo *info = *it;
			if (info->type == type && info->id == id) {
				fResources.erase(it);
				break;
			}
		}
	}

	int32 size() const
	{
		return fResources.size();
	}

	const ResourceInfo *infoAt(int32 index) const
	{
		const ResourceInfo *info = NULL;
		if (index >= 0 && index < size())
			info = fResources[index];
		return info;
	}

	const ResourceInfo *find(type_code type, int32 id) const
	{
		const ResourceInfo *result = NULL;
		for (ResInfoSet::const_iterator it = fResources.begin();
			 result == NULL && it != fResources.end();
			 it++) {
			const ResourceInfo *info = *it;
			if (info->type == type && info->id == id)
				result = info;
		}
		return result;
	}

	ResInfoSet	fResources;
};


// Suite
CppUnit::Test*
ResourcesTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<ResourcesTest> TC;
		
	suite->addTest( new TC("BResources::Init Test",
						   &ResourcesTest::InitTest) );
	suite->addTest( new TC("BResources::Read Test",
						   &ResourcesTest::ReadTest) );
	suite->addTest( new TC("BResources::Sync Test",
						   &ResourcesTest::SyncTest) );
	suite->addTest( new TC("BResources::Merge Test",
						   &ResourcesTest::MergeTest) );
	suite->addTest( new TC("BResources::WriteTo Test",
						   &ResourcesTest::WriteToTest) );
	suite->addTest( new TC("BResources::AddRemove Test",
						   &ResourcesTest::AddRemoveTest) );
	suite->addTest( new TC("BResources::ReadWrite Test",
						   &ResourcesTest::ReadWriteTest) );

	return suite;
}		

// setUp
void
ResourcesTest::setUp()
{
	BasicTest::setUp();
	BString unescapedTestDir(BTestShell::GlobalTestDir());
	unescapedTestDir.CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
	string resourcesTestDir(unescapedTestDir.String());
	resourcesTestDir += "/resources";
	execCommand(string("mkdir ") + testDir
				+ " ; cp " + resourcesTestDir + "/" + x86ResName + " "
						   + resourcesTestDir + "/" + ppcResName + " "
						   + resourcesTestDir + "/" + elfName + " "
						   + resourcesTestDir + "/" + elfNoResName + " "
						   + resourcesTestDir + "/" + pefName + " "
						   + resourcesTestDir + "/" + pefNoResName + " "
						   + testDir
				+ " ; touch " + emptyFile
				+ " ; echo \"That's not a resource file.\" > " + noResFile
				);
}
	
// tearDown
void
ResourcesTest::tearDown()
{
	execCommand(string("rm -rf ") + testDir);
	BasicTest::tearDown();
}

// InitTest
void
ResourcesTest::InitTest()
{
	// 1. existing files, read only
	// x86 resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ppc resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ELF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ELF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// PEF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// PEF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// empty file
	NextSubTest();
	{
		BResources resources;
		BFile file(emptyFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// non-resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( equals(resources.SetTo(&file, false), B_ERROR,
							   B_IO_ERROR) );
	}

	// 2. existing files, read only, clobber
	// x86 resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// ppc resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// ELF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// ELF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// PEF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// PEF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// empty file
	NextSubTest();
	{
		BResources resources;
		BFile file(emptyFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}
	// non-resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}

	// 3. existing files, read/write
	// x86 resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ppc resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ELF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// ELF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// PEF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// PEF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// empty file
	NextSubTest();
	{
		BResources resources;
		BFile file(emptyFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
	}
	// non-resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( equals(resources.SetTo(&file, false), B_ERROR,
							   B_IO_ERROR) );
	}

	// 4. existing files, read/write, clobber
	// x86 resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// ppc resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(ppcResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// ELF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// ELF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(elfNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// PEF binary containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// PEF binary not containing resources
	NextSubTest();
	{
		BResources resources;
		BFile file(pefNoResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// empty file
	NextSubTest();
	{
		BResources resources;
		BFile file(emptyFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// non-resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
	}

	// 5. bad args
	// uninitialized file
	NextSubTest();
	{
		BResources resources;
		BFile file;
		CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_NO_INIT );
	}
	// badly initialized file
	NextSubTest();
	{
		BResources resources;
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_ENTRY_NOT_FOUND );
	}
	// NULL file
	NextSubTest();
	{
		BResources resources;
		// R5: A NULL file is B_OK!
//		CPPUNIT_ASSERT( resources.SetTo(NULL, false) == B_BAD_VALUE );
		CPPUNIT_ASSERT( resources.SetTo(NULL, false) == B_OK );
	}
}

// ReadResTest
static
void
ReadResTest(BResources& resources, const ResourceInfo& info, bool exists)
{
	if (exists) {
		// test an existing resource
		// HasResource()
		CPPUNIT_ASSERT( resources.HasResource(info.type, info.id) == true );
		CPPUNIT_ASSERT( resources.HasResource(info.type, info.name)
						== true );
		// GetResourceInfo()
		const char *name;
		size_t length;
		int32 id;
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.id,
												  &name, &length) == true );
		CPPUNIT_ASSERT( name == NULL && info.name == NULL
						|| name != NULL && info.name != NULL
						   && !strcmp(name, info.name) );
		CPPUNIT_ASSERT( length == info.size );
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.name,
												  &id, &length) == true );
		CPPUNIT_ASSERT( id == info.id );
		CPPUNIT_ASSERT( length == info.size );
		// LoadResource
		size_t length1;
		size_t length2;
		const void *data1
			= resources.LoadResource(info.type, info.id, &length1);
		const void *data2
			= resources.LoadResource(info.type, info.name, &length2);
		CPPUNIT_ASSERT( length1 == info.size && length2 == info.size );
		CPPUNIT_ASSERT( data1 != NULL && data1 == data2 );
		CPPUNIT_ASSERT( !memcmp(data1, info.data, info.size) );
		// GetResourceInfo()
		type_code type = 0;
		id = 0;
		length = 0;
		name = NULL;
		CPPUNIT_ASSERT( resources.GetResourceInfo(data1, &type, &id, &length,
												  &name) == true );
		CPPUNIT_ASSERT( type == info.type );
		CPPUNIT_ASSERT( id == info.id );
		CPPUNIT_ASSERT( length == info.size );
		CPPUNIT_ASSERT( name == NULL && info.name == NULL
						|| name != NULL && info.name != NULL
						   && !strcmp(name, info.name) );
		// ReadResource()
		const int32 bufferSize = 1024;
		char buffer[bufferSize];
		memset(buffer, 0x0f, bufferSize);
		// read past the end
		status_t error = resources.ReadResource(info.type, info.id, buffer,
												info.size + 2, bufferSize);
		CPPUNIT_ASSERT( error == B_OK );
		for (int32 i = 0; i < bufferSize; i++)
			CPPUNIT_ASSERT( buffer[i] == 0x0f );
		// read 2 bytes from the middle
		int32 offset = (info.size - 2) / 2;
		error = resources.ReadResource(info.type, info.id, buffer, offset, 2);
		CPPUNIT_ASSERT( error == B_OK );
		CPPUNIT_ASSERT( !memcmp(buffer, (const char*)info.data + offset, 2) );
		for (int32 i = 2; i < bufferSize; i++)
			CPPUNIT_ASSERT( buffer[i] == 0x0f );
		// read the whole chunk
		error = resources.ReadResource(info.type, info.id, buffer, 0,
									   bufferSize);
		CPPUNIT_ASSERT( error == B_OK );
		CPPUNIT_ASSERT( !memcmp(buffer, info.data, info.size) );
		// FindResource()
		size_t lengthFound1;
		size_t lengthFound2;
		void *dataFound1
			= resources.FindResource(info.type, info.id, &lengthFound1);
		void *dataFound2
			= resources.FindResource(info.type, info.name, &lengthFound2);
		CPPUNIT_ASSERT( dataFound1 != NULL && dataFound2 != NULL );
		CPPUNIT_ASSERT( lengthFound1 == info.size
						&& lengthFound2 == info.size );
		CPPUNIT_ASSERT( !memcmp(dataFound1, info.data, info.size) );
		CPPUNIT_ASSERT( !memcmp(dataFound2, info.data, info.size) );
		free(dataFound1);
		free(dataFound2);
	} else {
		// test a non-existing resource
		// HasResource()
		CPPUNIT_ASSERT( resources.HasResource(info.type, info.id) == false );
		CPPUNIT_ASSERT( resources.HasResource(info.type, info.name)
						== false );
		// GetResourceInfo()
		const char *name;
		size_t length;
		int32 id;
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.id,
												  &name, &length) == false );
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.name,
												  &id, &length) == false );
		// LoadResource
		size_t length1;
		size_t length2;
		const void *data1
			= resources.LoadResource(info.type, info.id, &length1);
		const void *data2
			= resources.LoadResource(info.type, info.name, &length2);
		CPPUNIT_ASSERT( data1 == NULL && data2 == NULL );
		// ReadResource()
		const int32 bufferSize = 1024;
		char buffer[bufferSize];
		status_t error = resources.ReadResource(info.type, info.id, buffer,
												0, bufferSize);
		CPPUNIT_ASSERT( error == B_BAD_VALUE );
		// FindResource()
		size_t lengthFound1;
		size_t lengthFound2;
		void *dataFound1
			= resources.FindResource(info.type, info.id, &lengthFound1);
		void *dataFound2
			= resources.FindResource(info.type, info.name, &lengthFound2);
		CPPUNIT_ASSERT( dataFound1 == NULL && dataFound2 == NULL );
	}
}

// ReadBadResTest
static
void
ReadBadResTest(BResources& resources, const ResourceInfo& info)
{
	// HasResource()
	CPPUNIT_ASSERT( resources.HasResource(info.type, info.id) == false );
	CPPUNIT_ASSERT( resources.HasResource(info.type, info.name)
					== false );
	// GetResourceInfo()
	type_code type;
	const char *name;
	size_t length;
	int32 id;
	CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.id,
											  &name, &length) == false );
	CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.name,
											  &id, &length) == false );
	CPPUNIT_ASSERT( resources.GetResourceInfo(0, &type, &id,
											  &name, &length) == false );
	CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, 0, &id,
											  &name, &length) == false );
	// LoadResource
	size_t length1;
	size_t length2;
	const void *data1
		= resources.LoadResource(info.type, info.id, &length1);
	const void *data2
		= resources.LoadResource(info.type, info.name, &length2);
	CPPUNIT_ASSERT( data1 == NULL && data2 == NULL );
	// ReadResource()
	const int32 bufferSize = 1024;
	char buffer[bufferSize];
	status_t error = resources.ReadResource(info.type, info.id, buffer,
											0, bufferSize);
	CPPUNIT_ASSERT( error == B_BAD_VALUE );
	// FindResource()
	size_t lengthFound1;
	size_t lengthFound2;
	void *dataFound1
		= resources.FindResource(info.type, info.id, &lengthFound1);
	void *dataFound2
		= resources.FindResource(info.type, info.name, &lengthFound2);
	CPPUNIT_ASSERT( dataFound1 == NULL && dataFound2 == NULL );
}

// ReadTest
void
ResourcesTest::ReadTest()
{
	// tests:
	// * HasResource()
	// * GetResourceInfo()
	// * LoadResource()
	// * ReadResource()
	// * FindResource()
	// * PreloadResource()

	// 1. basic tests
	// x86 resource file
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// ppc resource file
	NextSubTest();
	{
		BFile file(ppcResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// ELF binary containing resources
	NextSubTest();
	{
		BFile file(elfFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// ELF binary not containing resources
	NextSubTest();
	{
		BFile file(elfNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, false);
		ReadResTest(resources, testResource2, false);
		ReadResTest(resources, testResource3, false);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// PEF binary containing resources
	NextSubTest();
	{
		BFile file(pefFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// PEF binary not containing resources
	NextSubTest();
	{
		BFile file(pefNoResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		ReadResTest(resources, testResource1, false);
		ReadResTest(resources, testResource2, false);
		ReadResTest(resources, testResource3, false);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}

	// 2. PreloadResource()
	// Hard to test: just preload all and check, if it still works.
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		// non existing type
		CPPUNIT_ASSERT( resources.PreloadResourceType(B_MESSENGER_TYPE)
						== B_OK );
		// int32 type
		CPPUNIT_ASSERT( resources.PreloadResourceType(B_INT32_TYPE) == B_OK );
		// all types
		CPPUNIT_ASSERT( resources.PreloadResourceType() == B_OK );
		ReadResTest(resources, testResource1, true);
		ReadResTest(resources, testResource2, true);
		ReadResTest(resources, testResource3, true);
		ReadResTest(resources, testResource4, false);
		ReadResTest(resources, testResource5, false);
	}
	// uninitialized BResources
	NextSubTest();
	{
		BResources resources;
		// int32 type
		CPPUNIT_ASSERT( resources.PreloadResourceType(B_INT32_TYPE) == B_OK );
		// all types
		CPPUNIT_ASSERT( resources.PreloadResourceType() == B_OK );
	}

	// 3. the index versions of GetResourceInfo()
	// index only
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo *resourceInfos[] = {
			&testResource1, &testResource2, &testResource3
		};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo &info = *resourceInfos[i];
			type_code type;
			int32 id;
			const char *name;
			size_t length;
			CPPUNIT_ASSERT( resources.GetResourceInfo(i, &type, &id, &name,
													  &length) == true );
			CPPUNIT_ASSERT( id == info.id );
			CPPUNIT_ASSERT( type == info.type );
			CPPUNIT_ASSERT( name == NULL && info.name == NULL
							|| name != NULL && info.name != NULL
							   && !strcmp(name, info.name) );
			CPPUNIT_ASSERT( length == info.size );
		}
		type_code type;
		int32 id;
		const char *name;
		size_t length;
		CPPUNIT_ASSERT( resources.GetResourceInfo(resourceCount, &type, &id,
												  &name, &length) == false );
	}
	// type and index
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		CPPUNIT_ASSERT( resources.AddResource(testResource4.type,
											  testResource4.id,
											  testResource4.data,
											  testResource4.size,
											  testResource4.name) == B_OK );
		
		const ResourceInfo *resourceInfos[] = {
			&testResource1, &testResource4
		};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		type_code type = resourceInfos[0]->type;
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo &info = *resourceInfos[i];
			int32 id;
			const char *name;
			size_t length;
			CPPUNIT_ASSERT( resources.GetResourceInfo(type, i, &id, &name,
													  &length) == true );
			CPPUNIT_ASSERT( id == info.id );
			CPPUNIT_ASSERT( type == info.type );
			CPPUNIT_ASSERT( name == NULL && info.name == NULL
							|| name != NULL && info.name != NULL
							   && !strcmp(name, info.name) );
			CPPUNIT_ASSERT( length == info.size );
		}
		int32 id;
		const char *name;
		size_t length;
		CPPUNIT_ASSERT( resources.GetResourceInfo(type, resourceCount, &id,
												  &name, &length) == false );
	}

	// 4. error cases
	// non-resource file
	NextSubTest();
	{
		BResources resources;
		BFile file(noResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		CPPUNIT_ASSERT( equals(resources.SetTo(&file, false), B_ERROR,
							   B_IO_ERROR) );
		ReadBadResTest(resources, testResource1);
	}
	// uninitialized file
	NextSubTest();
	{
		BResources resources;
		BFile file;
		CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_NO_INIT );
		ReadBadResTest(resources, testResource1);
	}
	// badly initialized file
	NextSubTest();
	{
		BResources resources;
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_ENTRY_NOT_FOUND );
		ReadBadResTest(resources, testResource1);
	}
	// NULL file
	NextSubTest();
	{
		BResources resources;
		// R5: A NULL file is B_OK!
//		CPPUNIT_ASSERT( resources.SetTo(NULL, false) == B_BAD_VALUE );
		CPPUNIT_ASSERT( resources.SetTo(NULL, false) == B_OK );
		ReadBadResTest(resources, testResource1);
	}
	// bad args
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo &info = testResource1;
		// GetResourceInfo()
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.id,
												  NULL, NULL) == true );
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, info.name,
												  NULL, NULL) == true );
		CPPUNIT_ASSERT( resources.GetResourceInfo(0L, (type_code*)NULL, NULL,
												  NULL, NULL) == true );
		CPPUNIT_ASSERT( resources.GetResourceInfo(info.type, 0, NULL,
												  NULL, NULL) == true );
		// LoadResource
		const void *data1
			= resources.LoadResource(info.type, info.id, NULL);
		const void *data2
			= resources.LoadResource(info.type, info.name, NULL);
		CPPUNIT_ASSERT( data1 != NULL && data2 != NULL );
		// ReadResource()
		const int32 bufferSize = 1024;
		status_t error = resources.ReadResource(info.type, info.id, NULL,
												0, bufferSize);
		CPPUNIT_ASSERT( error == B_BAD_VALUE );
		// FindResource()
		void *dataFound1
			= resources.FindResource(info.type, info.id, NULL);
		void *dataFound2
			= resources.FindResource(info.type, info.name, NULL);
		CPPUNIT_ASSERT( dataFound1 != NULL && dataFound2 != NULL );
		free(dataFound1);
		free(dataFound2);
	}
}

// AddResources
static
void
AddResources(BResources &resources, const ResourceInfo *resourceInfos[],
			 int32 count)
{
	for (int32 i = 0; i < count; i++) {
		const ResourceInfo &info = *resourceInfos[i];
		CPPUNIT_ASSERT( resources.AddResource(info.type, info.id, info.data,
											  info.size, info.name) == B_OK );
	}
}

// CompareFiles
static
void
CompareFiles(BFile &file1, BFile &file2)
{
	off_t size1 = 0;
	off_t size2 = 0;
	CPPUNIT_ASSERT( file1.GetSize(&size1) == B_OK );
	CPPUNIT_ASSERT( file2.GetSize(&size2) == B_OK );
	CPPUNIT_ASSERT( size1 == size2 );
	char *buffer1 = new char[size1];
	char *buffer2 = new char[size2];
	CPPUNIT_ASSERT( file1.ReadAt(0, buffer1, size1) == size1 );
	CPPUNIT_ASSERT( file2.ReadAt(0, buffer2, size2) == size2 );
	for (int32 i = 0; i < size1; i++)
		CPPUNIT_ASSERT( buffer1[i] == buffer2[i] );
	delete[] buffer1;
	delete[] buffer2;
}

// CompareResources
static
void
CompareResources(BResources &resources, const ResourceSet &resourceSet)
{
	// compare the count
	int32 count = resourceSet.size();
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
		const ResourceInfo &info = *resourceSet.infoAt(i);
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

// AddResource
static
void
AddResource(BResources &resources, ResourceSet &resourceSet,
			const ResourceInfo &info)
{
	resourceSet.add(&info);
	CPPUNIT_ASSERT( resources.AddResource(info.type, info.id, info.data,
										  info.size, info.name) == B_OK );
	CompareResources(resources, resourceSet);
}

// RemoveResource
static
void
RemoveResource(BResources &resources, ResourceSet &resourceSet,
			   const ResourceInfo &info, bool firstVersion)
{
	resourceSet.remove(&info);
	if (firstVersion) {
		CPPUNIT_ASSERT( resources.RemoveResource(info.type, info.id) == B_OK );
	} else {
		size_t size;
		const void *data = resources.LoadResource(info.type, info.id, &size);
		CPPUNIT_ASSERT( data != NULL );
		CPPUNIT_ASSERT( resources.RemoveResource(data) == B_OK );
	}
	CompareResources(resources, resourceSet);
}

// ListResources
/*static
void
ListResources(BResources &resources)
{
	printf("Resources:\n");
	type_code type;
	int32 id;
	const char *name;
	size_t size;
	for (int32 i = 0;
		 resources.GetResourceInfo(i, &type, &id, &name, &size);
		 i++) {
		type_code bigType = htonl(type);
		printf("resource %2ld: type: `%.4s', id: %3ld, size: %5lu\n", i,
			   (char*)&bigType, id, size);
	}
}*/

// SyncTest
void
ResourcesTest::SyncTest()
{
	// Sync() is not easy to test. We just check its return value for now.
	NextSubTest();
	execCommand(string("cp ") + x86ResFile + " " + testFile1);
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		// add a resource and Sync()
		CPPUNIT_ASSERT( resources.AddResource(testResource4.type,
											  testResource4.id,
											  testResource4.data,
											  testResource4.size,
											  testResource4.name) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
		// remove a resource and Sync()
		CPPUNIT_ASSERT( resources.RemoveResource(testResource1.type,
												 testResource1.id) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_OK );
	}
	// error cases
	// read only file
	NextSubTest();
	{
		BFile file(testFile1, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		CPPUNIT_ASSERT( resources.Sync() == B_NOT_ALLOWED );
	}
	// uninitialized BResources
	NextSubTest();
	{
		BResources resources;
		CPPUNIT_ASSERT( resources.Sync() == B_NO_INIT );
	}
	// badly initialized BResources
	NextSubTest();
	{
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_ENTRY_NOT_FOUND );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( resources.Sync() == B_NO_INIT );
	}
}

// MergeTest
void
ResourcesTest::MergeTest()
{
	// empty file, merge with resources from another file
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( resources.MergeFrom(&file2) == B_OK );
		ResourceSet resourceSet;
		resourceSet.add(&testResource1);
		resourceSet.add(&testResource2);
		resourceSet.add(&testResource3);
		CompareResources(resources, resourceSet);
	}
	// empty file, add some resources first and then merge with resources
	// from another file
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		// add some resources
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource4);
		AddResource(resources, resourceSet, testResource5);
		AddResource(resources, resourceSet, testResource6);
		// merge
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( resources.MergeFrom(&file2) == B_OK );
		resourceSet.add(&testResource1);	// replaces testResource6
		resourceSet.add(&testResource2);
		resourceSet.add(&testResource3);
		CompareResources(resources, resourceSet);
	}
	// error cases
	// bad args
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		BFile file2(noResFile, B_READ_ONLY);
		// non-resource file
		CPPUNIT_ASSERT( resources.MergeFrom(&file2) == B_IO_ERROR );
		// NULL file
// R5: crashs
#if !TEST_R5
		CPPUNIT_ASSERT( resources.MergeFrom(NULL) == B_BAD_VALUE );
#endif
	}
	// uninitialized BResources
	// R5: returns B_OK!
	NextSubTest();
	{
		BResources resources;
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( resources.MergeFrom(&file2) == B_OK );
	}
	// badly initialized BResources
	// R5: returns B_OK!
	NextSubTest();
	{
		BFile file(noSuchFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_ENTRY_NOT_FOUND );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_ENTRY_NOT_FOUND );
		BFile file2(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( resources.MergeFrom(&file2) == B_OK );
	}
}

// WriteToTest
void
ResourcesTest::WriteToTest()
{
	// take a file with resources and write them to an empty file
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.WriteTo(&file2) == B_OK);
		CPPUNIT_ASSERT( resources.File() == file2 );
	}	
	// take a file with resources and write them to an non-empty non-resource
	// file
	NextSubTest();
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		BFile file2(noResFile, B_READ_WRITE);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.WriteTo(&file2) == B_IO_ERROR);
		CPPUNIT_ASSERT( resources.File() == file2 );
	}	
	// take a file with resources and write them to an ELF file
	NextSubTest();
	execCommand(string("cp ") + elfNoResFile + " " + testFile1);
	{
		BFile file(x86ResFile, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.WriteTo(&file2) == B_OK);
		CPPUNIT_ASSERT( resources.File() == file2 );
	}	
	// empty file, add a resource, write it to another file
	NextSubTest();
	{
		BFile file(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		// add a resource
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource1);
		// write to another file
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.WriteTo(&file2) == B_OK);
		CPPUNIT_ASSERT( resources.File() == file2 );
		// check, whether the first file has been modified -- it shouldn't be
		off_t fileSize = 0;
		CPPUNIT_ASSERT( file.GetSize(&fileSize) == B_OK );
		CPPUNIT_ASSERT( fileSize == 0 );
	}
	// uninitialized BResources
	NextSubTest();
	execCommand(string("cp ") + elfNoResFile + " " + testFile1);
	{
		BResources resources;
		BFile file2(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( resources.WriteTo(&file2) == B_OK);
		CPPUNIT_ASSERT( resources.File() == file2 );
	}	
	// bad args: NULL file
	// R5: crashs
	NextSubTest();
	{
		BFile file(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
#if !TEST_R5
		CPPUNIT_ASSERT( resources.WriteTo(NULL) == B_BAD_VALUE);
#endif
	}	
}

// AddRemoveTest
void
ResourcesTest::AddRemoveTest()
{
	// tests:
	// * AddResource()
	// * RemoveResource()
	// * WriteResource()

	// Start with an empty file, add all the resources of our x86 resource
	// file and compare the two bytewise.
	// This does of course only work on x86 machines.
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		const ResourceInfo *resourceInfos[] = {
			&testResource1, &testResource2, &testResource3
		};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		AddResources(resources, resourceInfos, resourceCount);
	}
	{
		BFile file1(testFile1, B_READ_ONLY);
		BFile file2(x86ResFile, B_READ_ONLY);
		CompareFiles(file1, file2);
	}
	// Now remove all resources and compare the file with an empty resource
	// file.
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo *resourceInfos[] = {
			&testResource1, &testResource2, &testResource3
		};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo &info = *resourceInfos[i];
			CPPUNIT_ASSERT( resources.RemoveResource(info.type, info.id)
							== B_OK );
		}
		BFile file2(testFile2, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		BResources resources2;
		CPPUNIT_ASSERT( resources2.SetTo(&file2, true) == B_OK );
	}
	{
		BFile file1(testFile1, B_READ_ONLY);
		BFile file2(testFile2, B_READ_ONLY);
		CompareFiles(file1, file2);
	}
	// Same file, use the other remove version.
	NextSubTest();
	execCommand(string("cp ") + x86ResFile + " " + testFile1);
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo *resourceInfos[] = {
			&testResource1, &testResource2, &testResource3
		};
		int32 resourceCount = sizeof(resourceInfos) / sizeof(ResourceInfo*);
		for (int32 i = 0; i < resourceCount; i++) {
			const ResourceInfo &info = *resourceInfos[i];
			size_t size;
			const void *data = resources.LoadResource(info.type, info.id,
													  &size);
			CPPUNIT_ASSERT( data != NULL );
			CPPUNIT_ASSERT( resources.RemoveResource(data) == B_OK );
		}
	}
	{
		BFile file1(testFile1, B_READ_ONLY);
		BFile file2(testFile2, B_READ_ONLY);
		CompareFiles(file1, file2);
	}
	// some arbitrary adding and removing...
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		ResourceSet resourceSet;
		AddResource(resources, resourceSet, testResource1);
		AddResource(resources, resourceSet, testResource2);
		RemoveResource(resources, resourceSet, testResource1, true);
		AddResource(resources, resourceSet, testResource3);
		AddResource(resources, resourceSet, testResource4);
		RemoveResource(resources, resourceSet, testResource3, false);
		AddResource(resources, resourceSet, testResource5);
		AddResource(resources, resourceSet, testResource1);
		AddResource(resources, resourceSet, testResource6);		// replaces 1
		RemoveResource(resources, resourceSet, testResource2, true);
		RemoveResource(resources, resourceSet, testResource5, false);
		RemoveResource(resources, resourceSet, testResource6, true);
		RemoveResource(resources, resourceSet, testResource4, false);
	}
	// bad args
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, true) == B_OK );
		CPPUNIT_ASSERT( resources.RemoveResource(NULL) == B_BAD_VALUE );
		CPPUNIT_ASSERT( resources.RemoveResource(B_INT32_TYPE, 0)
						== B_BAD_VALUE );
		CPPUNIT_ASSERT( resources.AddResource(B_INT32_TYPE, 0, NULL, 7,
											  "Hey!") == B_BAD_VALUE );
	}
	// uninitialized BResources
	NextSubTest();
	{
		BResources resources;
		const ResourceInfo &info = testResource1;
		CPPUNIT_ASSERT( resources.AddResource(info.type, info.id, info.data,
											  info.size, info.name)
						== B_OK );
	}
}

// ReadWriteTest
void
ResourcesTest::ReadWriteTest()
{
	// ReadResource() has already been tested in ReadTest(). Thus we focus on
	// WriteResource().
	// Open a file and write a little bit into one of its resources.
	NextSubTest();
	execCommand(string("cp ") + x86ResFile + " " + testFile1);
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo &info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		// write at the beginning of the resource
		char writeBuffer[1024];
		for (int32 i = 0; i < 1024; i++)
			writeBuffer[i] = i & 0xff;
		char readBuffer[1024];
		memset(readBuffer, 0, 1024);
		CPPUNIT_ASSERT( resources.WriteResource(type, id, writeBuffer, 0, 10)
						== B_OK );
		size_t length;
		const void *data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT( data != NULL && length == info.size );
		CPPUNIT_ASSERT( !memcmp(data, writeBuffer, 10) );
		CPPUNIT_ASSERT( !memcmp((const char*)data + 10, info.data + 10,
								info.size - 10) );
		// completely overwrite the resource with the former data
		CPPUNIT_ASSERT( resources.WriteResource(type, id, info.data, 0,
												info.size) == B_OK );
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT( data != NULL && length == info.size );
		CPPUNIT_ASSERT( !memcmp(data, info.data, info.size) );
		// write something in the middle
		size_t offset = (info.size - 2) / 2;
		CPPUNIT_ASSERT( resources.WriteResource(type, id, writeBuffer, offset,
												2) == B_OK );
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT( data != NULL && length == info.size );
		CPPUNIT_ASSERT( !memcmp(data, info.data, offset) );
		CPPUNIT_ASSERT( !memcmp((const char*)data + offset, writeBuffer, 2) );
		CPPUNIT_ASSERT( !memcmp((const char*)data + offset + 2,
								info.data + offset + 2,
								info.size - offset - 2) );
		// write starting inside the resource, but extending it
		size_t writeSize = info.size + 3;
		CPPUNIT_ASSERT( resources.WriteResource(type, id, writeBuffer, offset,
												writeSize) == B_OK );
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT( data != NULL && length == (size_t)offset + writeSize );
		CPPUNIT_ASSERT( !memcmp(data, info.data, offset) );
		CPPUNIT_ASSERT( !memcmp((const char*)data + offset, writeBuffer,
								writeSize) );
		// write past the end of the resource
		size_t newOffset = length + 30;
		size_t newWriteSize = 17;
		CPPUNIT_ASSERT( resources.WriteResource(type, id, writeBuffer,
												newOffset, newWriteSize)
						== B_OK );
		data = resources.LoadResource(type, id, &length);
		CPPUNIT_ASSERT( data != NULL && length == newOffset + newWriteSize );
		CPPUNIT_ASSERT( !memcmp(data, info.data, offset) );
		CPPUNIT_ASSERT( !memcmp((const char*)data + offset, writeBuffer,
								writeSize) );
		// [offset + writeSize, newOffset) unspecified
		CPPUNIT_ASSERT( !memcmp((const char*)data + newOffset, writeBuffer,
								newWriteSize) );
	}
	// error cases
	// bad args
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BResources resources;
		CPPUNIT_ASSERT( resources.SetTo(&file, false) == B_OK );
		const ResourceInfo &info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		// NULL buffer
		CPPUNIT_ASSERT( resources.WriteResource(type, id, NULL, 0, 10)
						== B_BAD_VALUE );
		// non existing resource
		char writeBuffer[1024];
		CPPUNIT_ASSERT( resources.WriteResource(B_MESSENGER_TYPE, 0,
												writeBuffer, 0, 10)
						== B_BAD_VALUE );
	}
	// uninitialized BResources
	NextSubTest();
	{
		BResources resources;
		const ResourceInfo &info = testResource1;
		type_code type = info.type;
		int32 id = info.id;
		char writeBuffer[1024];
		CPPUNIT_ASSERT( resources.WriteResource(type, id, writeBuffer, 0, 10)
						== B_BAD_VALUE );
	}
}





