// PathTest.cpp
#include "PathTest.h"

#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <TypeConstants.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
using std::string;


// Suite
CppUnit::Test*
PathTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<PathTest> TC;
		
	suite->addTest( new TC("BPath::Init Test1", &PathTest::InitTest1) );
	suite->addTest( new TC("BPath::Init Test2", &PathTest::InitTest2) );
	suite->addTest( new TC("BPath::Append Test", &PathTest::AppendTest) );
	suite->addTest( new TC("BPath::Leaf Test", &PathTest::LeafTest) );
	suite->addTest( new TC("BPath::Parent Test", &PathTest::ParentTest) );
	suite->addTest( new TC("BPath::Comparison Test",
						   &PathTest::ComparisonTest) );
	suite->addTest( new TC("BPath::Assignment Test",
						   &PathTest::AssignmentTest) );
	suite->addTest( new TC("BPath::Flattenable Test",
						   &PathTest::FlattenableTest) );
		
	return suite;
}		

// setUp
void
PathTest::setUp()
{
	BasicTest::setUp();
}
	
// tearDown
void
PathTest::tearDown()
{
	BasicTest::tearDown();
}

// InitTest1
void
PathTest::InitTest1()
{
	// 1. default constructor
	NextSubTest();
	{
		BPath path;
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}

	// 2. BPath(const char*, const char*, bool)
	// absolute existing path (root dir), no leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// absolute existing path, no leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// absolute non-existing path, no leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist/but/who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// absolute existing path (root dir), no leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/.///.";
		const char *normalizedPathName = "/";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	}
	// absolute existing path, no leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/boot/";
		const char *normalizedPathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	}
	// absolute non-existing path, no leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist/but///who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// absolute existing path (root dir), no leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/";
		BPath path(pathName, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// absolute existing path, no leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/boot";
		BPath path(pathName, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// absolute non-existing path, no leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist/but/who/cares";
		BPath path(pathName, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// relative existing path, no leaf, no normalization needed, but done
	chdir("/");
	NextSubTest();
	{
		const char *pathName = "boot";
		const char *absolutePathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// relative non-existing path, no leaf, no normalization needed, but done
	chdir("/boot");
	NextSubTest();
	{
		const char *pathName = "doesn't/exist/but/who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// relative existing path, no leaf, auto normalization
	chdir("/");
	NextSubTest();
	{
		const char *pathName = "boot/";
		const char *normalizedPathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	}
	// relative non-existing path, no leaf, auto normalization
	chdir("/boot");
	NextSubTest();
	{
		const char *pathName = "doesn't/exist/but///who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// relative existing path, no leaf, normalization forced
	chdir("/");
	NextSubTest();
	{
		const char *pathName = "boot";
		const char *absolutePathName = "/boot";
		BPath path(pathName, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// relative non-existing path, no leaf, normalization forced
	chdir("/boot");
	NextSubTest();
	{
		const char *pathName = "doesn't/exist/but/who/cares";
		BPath path(pathName, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// absolute existing path (root dir), leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/";
		const char *leafName = "boot";
		const char *absolutePathName = "/boot";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// absolute existing path, leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		const char *leafName = "home/Desktop";
		const char *absolutePathName = "/boot/home/Desktop";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// absolute non-existing path, leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist";
		const char *leafName = "but/who/cares";
		const char *absolutePathName = "/doesn't/exist/but/who/cares";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// absolute existing path (root dir), leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/.///";
		const char *leafName = ".";
		const char *absolutePathName = "/";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// absolute existing path, leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		const char *leafName = "home/..";
		const char *absolutePathName = "/boot";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// absolute non-existing path, leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist";
		const char *leafName = "but//who/cares";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// absolute non-existing path, leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/doesn't/exist";
		const char *leafName = "but/who/cares";
		BPath path(pathName, leafName, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// relative existing path, leaf, no normalization needed, but done
	chdir("/");
	NextSubTest();
	{
		const char *pathName = "boot";
		const char *leafName = "home";
		const char *absolutePathName = "/boot/home";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// relative non-existing path, leaf, no normalization needed, but done
	chdir("/boot");
	NextSubTest();
	{
		const char *pathName = "doesn't/exist";
		const char *leafName = "but/who/cares";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// relative existing path, leaf, auto normalization
	chdir("/boot");
	NextSubTest();
	{
		const char *pathName = "home";
		const char *leafName = "Desktop//";
		const char *normalizedPathName = "/boot/home/Desktop";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	}
	// bad args (absolute lead)
	NextSubTest();
	{
		const char *pathName = "/";
		const char *leafName = "/boot";
		BPath path(pathName, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args
	NextSubTest();
	{
		BPath path((const char*)NULL, "test");
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args
	NextSubTest();
	{
		BPath path((const char*)NULL);
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}

	// 3. BPath(const BDirectory*, const char*, bool)
	// existing dir (root dir), no leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/";
		BDirectory dir(pathName);
		BPath path(&dir, NULL);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing dir, no leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		BDirectory dir(pathName);
		BPath path(&dir, NULL);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing dir (root dir), no leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/";
		BDirectory dir(pathName);
		BPath path(&dir, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing dir, no leaf, normalization forced
	NextSubTest();
	{
		const char *pathName = "/boot";
		BDirectory dir(pathName);
		BPath path(&dir, NULL, true);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing dir (root dir), leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/";
		const char *leafName = "boot";
		const char *absolutePathName = "/boot";
		BDirectory dir(pathName);
		BPath path(&dir, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// existing dir, leaf, no normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		const char *leafName = "home/Desktop";
		const char *absolutePathName = "/boot/home/Desktop";
		BDirectory dir(pathName);
		BPath path(&dir, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// existing dir, leaf, auto normalization
	NextSubTest();
	{
		const char *pathName = "/boot";
		const char *leafName = "home/..";
		const char *absolutePathName = "/boot";
		BDirectory dir(pathName);
		BPath path(&dir, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	}
	// bad args (absolute leaf)
	NextSubTest();
	{
		const char *pathName = "/";
		const char *leafName = "/boot";
		BDirectory dir(pathName);
		BPath path(&dir, leafName);
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (uninitialized dir)
	NextSubTest();
	{
		BDirectory dir;
		BPath path(&dir, "test");
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (badly initialized dir)
	NextSubTest();
	{
		BDirectory dir("/this/dir/doesn't/exists");
		BPath path(&dir, "test");
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (NULL dir)
// R5: crashs, when passing a NULL BDirectory
#if !TEST_R5
	NextSubTest();
	{
		BPath path((const BDirectory*)NULL, "test");
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (NULL dir)
	NextSubTest();
	{
		BPath path((const BDirectory*)NULL, NULL);
		CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
#endif

	// 4. BPath(const BEntry*)
	// existing entry (root dir)
	NextSubTest();
	{
		const char *pathName = "/";
		BEntry entry(pathName);
		BPath path(&entry);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing entry
	NextSubTest();
	{
		const char *pathName = "/boot";
		BEntry entry(pathName);
		BPath path(&entry);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// abstract entry
	NextSubTest();
	{
		const char *pathName = "/boot/shouldn't exist";
		BEntry entry(pathName);
		BPath path(&entry);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// bad args (uninitialized BEntry)
	NextSubTest();
	{
		BEntry entry;
		BPath path(&entry);
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (badly initialized BEntry)
	NextSubTest();
	{
		BEntry entry("/this/doesn't/exist");
		BPath path(&entry);
		CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_ENTRY_NOT_FOUND) );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
	// bad args (NULL BEntry)
	NextSubTest();
	{
		BPath path((const BEntry*)NULL);
		CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE) );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}

	// 5. BPath(const entry_ref*)
	// existing entry (root dir)
	NextSubTest();
	{
		const char *pathName = "/";
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BPath path(&ref);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// existing entry
	NextSubTest();
	{
		const char *pathName = "/boot";
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BPath path(&ref);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// abstract entry
	NextSubTest();
	{
		const char *pathName = "/boot/shouldn't exist";
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BPath path(&ref);
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		CPPUNIT_ASSERT( string(pathName) == path.Path() );
	}
	// bad args (NULL entry_ref)
	NextSubTest();
	{
		BPath path((const entry_ref*)NULL);
		CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE) );
		CPPUNIT_ASSERT( path.Path() == NULL );
	}
}

// InitTest2
void
PathTest::InitTest2()
{
	BPath path;
	const char *pathName;
	const char *leafName;
	const char *absolutePathName;
	const char *normalizedPathName;
	BDirectory dir;
	BEntry entry;
	entry_ref ref;

	// 2. SetTo(const char*, const char*, bool)
	// absolute existing path (root dir), no leaf, no normalization
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	// absolute existing path, no leaf, no normalization
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	// absolute non-existing path, no leaf, no normalization
	NextSubTest();
	pathName = "/doesn't/exist/but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	// absolute existing path (root dir), no leaf, auto normalization
	NextSubTest();
	pathName = "/.///.";
	normalizedPathName = "/";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	path.Unset();
	// absolute existing path, no leaf, auto normalization
	NextSubTest();
	pathName = "/boot/";
	normalizedPathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	path.Unset();
	// absolute non-existing path, no leaf, auto normalization
	NextSubTest();
	pathName = "/doesn't/exist/but///who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// absolute existing path (root dir), no leaf, normalization forced
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( path.SetTo(pathName, NULL, true) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	// absolute existing path, no leaf, normalization forced
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName, NULL, true) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	// absolute non-existing path, no leaf, normalization forced
	NextSubTest();
	pathName = "/doesn't/exist/but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, NULL, true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// relative existing path, no leaf, no normalization needed, but done
	chdir("/");
	NextSubTest();
	pathName = "boot";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// relative non-existing path, no leaf, no normalization needed, but done
	chdir("/boot");
	NextSubTest();
	pathName = "doesn't/exist/but/who/cares";
	absolutePathName = "/boot/doesn't/exist/but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// relative existing path, no leaf, auto normalization
	chdir("/");
	NextSubTest();
	pathName = "boot/";
	normalizedPathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	path.Unset();
	// relative non-existing path, no leaf, auto normalization
	chdir("/boot");
	NextSubTest();
	pathName = "doesn't/exist/but///who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// relative existing path, no leaf, normalization forced
	chdir("/");
	NextSubTest();
	pathName = "boot";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName, NULL, true) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// relative non-existing path, no leaf, normalization forced
	chdir("/boot");
	NextSubTest();
	pathName = "doesn't/exist/but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, NULL, true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// absolute existing path (root dir), leaf, no normalization
	NextSubTest();
	pathName = "/";
	leafName = "boot";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute existing path, leaf, no normalization
	NextSubTest();
	pathName = "/boot";
	leafName = "home/Desktop";
	absolutePathName = "/boot/home/Desktop";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute non-existing path, leaf, no normalization
	NextSubTest();
	pathName = "/doesn't/exist";
	leafName = "but/who/cares";
	absolutePathName = "/doesn't/exist/but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute existing path (root dir), leaf, auto normalization
	NextSubTest();
	pathName = "/.///";
	leafName = ".";
	absolutePathName = "/";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute existing path, leaf, auto normalization
	NextSubTest();
	pathName = "/boot";
	leafName = "home/..";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute existing path, leaf, self assignment
	NextSubTest();
	pathName = "/boot/home";
	leafName = "home/Desktop";
	absolutePathName = "/boot/home/Desktop";
	CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(path.Path(), ".///./") == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	CPPUNIT_ASSERT( path.SetTo(path.Path(), "..") == B_OK );
	CPPUNIT_ASSERT( string("/boot") == path.Path() );
	CPPUNIT_ASSERT( path.SetTo(path.Path(), leafName) == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// absolute non-existing path, leaf, auto normalization
	NextSubTest();
	pathName = "/doesn't/exist";
	leafName = "but//who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// absolute non-existing path, leaf, normalization forced
	NextSubTest();
	pathName = "/doesn't/exist";
	leafName = "but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName, true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// relative existing path, leaf, no normalization needed, but done
	chdir("/");
	NextSubTest();
	pathName = "boot";
	leafName = "home";
	absolutePathName = "/boot/home";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	// relative non-existing path, leaf, no normalization needed, but done
	chdir("/boot");
	NextSubTest();
	pathName = "doesn't/exist";
	leafName = "but/who/cares";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// relative existing path, leaf, auto normalization
	chdir("/boot");
	NextSubTest();
	pathName = "home";
	leafName = "Desktop//";
	normalizedPathName = "/boot/home/Desktop";
	CPPUNIT_ASSERT( path.SetTo(pathName, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(normalizedPathName) == path.Path() );
	path.Unset();
	// bad args (absolute leaf)
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/", "/boot") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo((const char*)NULL, "test") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo((const char*)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();

	// 3. SetTo(const BDirectory*, const char*, bool)
	// existing dir (root dir), no leaf, no normalization
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, NULL) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir, no leaf, no normalization
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, NULL) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir (root dir), no leaf, normalization forced
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, NULL, true) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir, no leaf, normalization forced
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, NULL, true) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir (root dir), leaf, no normalization
	NextSubTest();
	pathName = "/";
	leafName = "boot";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir, leaf, no normalization
	NextSubTest();
	pathName = "/boot";
	leafName = "home/Desktop";
	absolutePathName = "/boot/home/Desktop";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// existing dir, leaf, auto normalization
	NextSubTest();
	pathName = "/boot";
	leafName = "home/..";
	absolutePathName = "/boot";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, leafName) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(absolutePathName) == path.Path() );
	path.Unset();
	dir.Unset();
	// bad args (absolute leaf)
	NextSubTest();
	pathName = "/";
	leafName = "/boot";
	CPPUNIT_ASSERT( dir.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&dir, leafName) == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	dir.Unset();
	// bad args (uninitialized dir)
	NextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.SetTo(&dir, "test") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	dir.Unset();
	// bad args (badly initialized dir)
	NextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/this/dir/doesn't/exists") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.SetTo(&dir, "test") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	dir.Unset();
// R5: crashs, when passing a NULL BDirectory
#if !TEST_R5
	// bad args (NULL dir)
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo((const BDirectory*)NULL, "test") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	dir.Unset();
	// bad args (NULL dir)
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo((const BDirectory*)NULL, NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	dir.Unset();
#endif

	// 4. SetTo(const BEntry*)
	// existing entry (root dir)
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// existing entry
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// abstract entry
	NextSubTest();
	pathName = "/boot/shouldn't exist";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// bad args (uninitialized BEntry)
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.SetTo(&entry) == B_NO_INIT );
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	entry.Unset();
	// bad args (badly initialized BEntry)
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/this/doesn't/exist") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( equals(path.SetTo(&entry), B_NO_INIT, B_ENTRY_NOT_FOUND) );
	CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_ENTRY_NOT_FOUND) );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	entry.Unset();
	// bad args (NULL BEntry)
	NextSubTest();
	CPPUNIT_ASSERT( equals(path.SetTo((const BEntry*)NULL), B_NO_INIT,
									  B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE) );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	entry.Unset();

	// 5. SetTo(const entry_ref*)
	// existing entry (root dir)
	NextSubTest();
	pathName = "/";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// existing entry
	NextSubTest();
	pathName = "/boot";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// abstract entry
	NextSubTest();
	pathName = "/boot/shouldn't exist";
	CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( path.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(pathName) == path.Path() );
	path.Unset();
	entry.Unset();
	// bad args (NULL entry_ref)
	NextSubTest();
	CPPUNIT_ASSERT( equals(path.SetTo((const entry_ref*)NULL), B_NO_INIT,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE) );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	entry.Unset();
}

// AppendTest
void
PathTest::AppendTest()
{
	BPath path;
	// uninitialized BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.Append("test") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// dir hierarchy, from existing to non-existing
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( string("/") == path.Path() );
	CPPUNIT_ASSERT( path.Append("boot") == B_OK );
	CPPUNIT_ASSERT( string("/boot") == path.Path() );
	CPPUNIT_ASSERT( path.Append("home/Desktop") == B_OK );
	CPPUNIT_ASSERT( string("/boot/home/Desktop") == path.Path() );
	CPPUNIT_ASSERT( path.Append("non/existing") == B_OK );
	CPPUNIT_ASSERT( string("/boot/home/Desktop/non/existing") == path.Path() );
	// trigger normalization
	CPPUNIT_ASSERT( path.Append("at/least/not//now") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// force normalization
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( string("/boot") == path.Path() );
	CPPUNIT_ASSERT( path.Append("home/non-existing", true) == B_OK );
	CPPUNIT_ASSERT( string("/boot/home/non-existing") == path.Path() );
	CPPUNIT_ASSERT( path.Append("not/now", true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( path.Path() == NULL );
	path.Unset();
	// bad/strange args
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( path.Append(NULL) == B_OK );
	CPPUNIT_ASSERT( string("/boot") == path.Path() );
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( path.Append("/tmp") == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.InitCheck() == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( path.Append("") == B_OK );
	CPPUNIT_ASSERT( string("/boot") == path.Path() );
	path.Unset();
}

// LeafTest
void
PathTest::LeafTest()
{
	BPath path;
	// uninitialized BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.Leaf() == NULL );
	path.Unset();
	// root dir
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( string("") == path.Leaf() );
	path.Unset();
	// existing dirs
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( string("boot") == path.Leaf() );
	CPPUNIT_ASSERT( path.SetTo("/boot/home") == B_OK );
	CPPUNIT_ASSERT( string("home") == path.Leaf() );
	CPPUNIT_ASSERT( path.SetTo("/boot/home/Desktop") == B_OK );
	CPPUNIT_ASSERT( string("Desktop") == path.Leaf() );
	path.Unset();
	// non-existing dirs
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/non-existing") == B_OK );
	CPPUNIT_ASSERT( string("non-existing") == path.Leaf() );
	CPPUNIT_ASSERT( path.SetTo("/non/existing/dir") == B_OK );
	CPPUNIT_ASSERT( string("dir") == path.Leaf() );
	path.Unset();
}

// ParentTest
void
PathTest::ParentTest()
{
	BPath path;
	BPath parent;
// R5: crashs, when GetParent() is called on uninitialized BPath
#if !TEST_R5
	// uninitialized BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_NO_INIT );
	path.Unset();
	parent.Unset();
#endif
	// root dir
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_ENTRY_NOT_FOUND );
	path.Unset();
	parent.Unset();
	// existing dirs
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_OK );
	CPPUNIT_ASSERT( string("/") == parent.Path() );
	CPPUNIT_ASSERT( path.SetTo("/boot/home") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_OK );
	CPPUNIT_ASSERT( string("/boot") == parent.Path() );
	CPPUNIT_ASSERT( path.SetTo("/boot/home/Desktop") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_OK );
	CPPUNIT_ASSERT( string("/boot/home") == parent.Path() );
	path.Unset();
	parent.Unset();
	// non-existing dirs
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/non-existing") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_OK );
	CPPUNIT_ASSERT( string("/") == parent.Path() );
	CPPUNIT_ASSERT( path.SetTo("/non/existing/dir") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&parent) == B_OK );
	CPPUNIT_ASSERT( string("/non/existing") == parent.Path() );
	path.Unset();
	parent.Unset();
	// destructive parenting
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/non/existing/dir") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(&path) == B_OK );
	CPPUNIT_ASSERT( string("/non/existing") == path.Path() );
	CPPUNIT_ASSERT( path.GetParent(&path) == B_OK );
	CPPUNIT_ASSERT( string("/non") == path.Path() );
	CPPUNIT_ASSERT( path.GetParent(&path) == B_OK );
	CPPUNIT_ASSERT( string("/") == path.Path() );
	CPPUNIT_ASSERT( path.GetParent(&path) == B_ENTRY_NOT_FOUND );
	path.Unset();
	parent.Unset();
// R5: crashs, when passing a NULL BPath
#if !TEST_R5
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/non/existing/dir") == B_OK );
	CPPUNIT_ASSERT( path.GetParent(NULL) == B_BAD_VALUE );
	path.Unset();
	parent.Unset();
#endif
}

// ComparisonTest
void
PathTest::ComparisonTest()
{
	BPath path;
	// 1. ==/!= const BPath &
	BPath path2;
	// uninitialized BPaths
// R5: uninitialized paths are unequal
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
#if !TEST_R5
	CPPUNIT_ASSERT( (path == path2) == true );
	CPPUNIT_ASSERT( (path != path2) == false );
#endif
	path.Unset();
	path2.Unset();
	// uninitialized argument BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( (path == path2) == false );
	CPPUNIT_ASSERT( (path != path2) == true );
	path.Unset();
	path2.Unset();
	// uninitialized this BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path2.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( (path == path2) == false );
	CPPUNIT_ASSERT( (path != path2) == true );
	path.Unset();
	path2.Unset();
	// various paths
	NextSubTest();
	const char *paths[] = { "/", "/boot", "/boot/home", "/boot/home/Desktop" };
	int32 pathCount = sizeof(paths) / sizeof(const char*);
	for (int32 i = 0; i < pathCount; i++) {
		for (int32 k = 0; k < pathCount; k++) {
			CPPUNIT_ASSERT( path.SetTo(paths[i]) == B_OK );
			CPPUNIT_ASSERT( path2.SetTo(paths[k]) == B_OK );
			CPPUNIT_ASSERT( (path == path2) == (i == k) );
			CPPUNIT_ASSERT( (path != path2) == (i != k) );
		}
	}
	path.Unset();
	path2.Unset();

	// 2. ==/!= const char *
	const char *pathName;
	// uninitialized BPath
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	pathName = "/";
	CPPUNIT_ASSERT( (path == pathName) == false );
	CPPUNIT_ASSERT( (path != pathName) == true );
	path.Unset();
	// various paths
	NextSubTest();
	for (int32 i = 0; i < pathCount; i++) {
		for (int32 k = 0; k < pathCount; k++) {
			CPPUNIT_ASSERT( path.SetTo(paths[i]) == B_OK );
			pathName = paths[k];
			CPPUNIT_ASSERT( (path == pathName) == (i == k) );
			CPPUNIT_ASSERT( (path != pathName) == (i != k) );
		}
	}
	path.Unset();
	// bad args (NULL const char*)
// R5: initialized path equals NULL argument!
// R5: uninitialized path does not equal NULL argument!
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/") == B_OK );
	pathName = NULL;
#if !TEST_R5
	CPPUNIT_ASSERT( (path == pathName) == false );
	CPPUNIT_ASSERT( (path != pathName) == true );
#endif
	path.Unset();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
#if !TEST_R5
	CPPUNIT_ASSERT( (path == pathName) == true );
	CPPUNIT_ASSERT( (path != pathName) == false );
#endif
	path.Unset();
}

// AssignmentTest
void
PathTest::AssignmentTest()
{
	// 1. copy constructor
	// uninitialized
	NextSubTest();
	{
		BPath path;
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		BPath path2(path);
		CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
	}
	// initialized
	NextSubTest();
	{
		BPath path("/boot/home/Desktop");
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		BPath path2(path);
		CPPUNIT_ASSERT( path2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( path == path2 );
	}

	// 2. assignment operator, const BPath &
	// uninitialized
	NextSubTest();
	{
		BPath path;
		BPath path2;
		path2 = path;
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
	}
	NextSubTest();
	{
		BPath path;
		BPath path2("/");
		CPPUNIT_ASSERT( path2.InitCheck() == B_OK );
		path2 = path;
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
	}
	// initialized
	NextSubTest();
	{
		BPath path("/boot/home");
		CPPUNIT_ASSERT( path.InitCheck() == B_OK );
		BPath path2;
		path2 = path;
		CPPUNIT_ASSERT( path2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( path == path2 );
	}

	// 2. assignment operator, const char *
	// initialized
	NextSubTest();
	{
		const char *pathName = "/boot/home";
		BPath path2;
		path2 = pathName;
		CPPUNIT_ASSERT( path2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( path2 == pathName );
	}
	// bad args
	NextSubTest();
	{
		const char *pathName = NULL;
		BPath path2;
		path2 = pathName;
		CPPUNIT_ASSERT( path2.InitCheck() == B_NO_INIT );
	}
}

// FlattenableTest
void
PathTest::FlattenableTest()
{
	BPath path;
	// 1. trivial methods (IsFixedSize(), TypeCode(), AllowsTypeCode)
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( path.IsFixedSize() == false );
	CPPUNIT_ASSERT( path.TypeCode() == B_REF_TYPE );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_REF_TYPE) == true );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_STRING_TYPE) == false );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_FLOAT_TYPE) == false );
	path.Unset();	
	// initialized
	NextSubTest();
	CPPUNIT_ASSERT( path.SetTo("/boot/home") == B_OK );
	CPPUNIT_ASSERT( path.IsFixedSize() == false );
	CPPUNIT_ASSERT( path.TypeCode() == B_REF_TYPE );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_REF_TYPE) == true );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_STRING_TYPE) == false );
	CPPUNIT_ASSERT( path.AllowsTypeCode(B_FLOAT_TYPE) == false );
	path.Unset();	

	// 2. non-trivial methods
	char buffer[1024];
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	ssize_t size = path.FlattenedSize();
	CPPUNIT_ASSERT( size == sizeof(dev_t) + sizeof(ino_t) );
	CPPUNIT_ASSERT( path.Flatten(buffer, sizeof(buffer)) == B_OK );
	CPPUNIT_ASSERT( path.Unflatten(B_REF_TYPE, buffer, size) == B_OK );
	CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
	path.Unset();
	// some flatten/unflatten tests
	NextSubTest();
	const char *paths[] = { "/", "/boot", "/boot/home", "/boot/home/Desktop",
							"/boot/home/non-existing" };
	int32 pathCount = sizeof(paths) / sizeof(const char*);
	for (int32 i = 0; i < pathCount; i++) {
		const char *pathName = paths[i];
		// init the path and get an equivalent entry ref
		CPPUNIT_ASSERT( path.SetTo(pathName) == B_OK );
		BEntry entry;
		CPPUNIT_ASSERT( entry.SetTo(pathName) == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		// flatten the path
		struct flattened_ref { dev_t device; ino_t directory; char name[1]; };
		size = path.FlattenedSize();
		ssize_t expectedSize
			= sizeof(dev_t) + sizeof(ino_t) + strlen(ref.name) + 1;
		CPPUNIT_ASSERT( size ==  expectedSize);
		CPPUNIT_ASSERT( path.Flatten(buffer, sizeof(buffer)) == B_OK );
		// check the flattened data
		const flattened_ref &fref = *(flattened_ref*)buffer;
		CPPUNIT_ASSERT( ref.device == fref.device );
		CPPUNIT_ASSERT( ref.directory == fref.directory );
		CPPUNIT_ASSERT( strcmp(ref.name, fref.name) == 0 );
		// unflatten the path 
		path.Unset();
		CPPUNIT_ASSERT( path.InitCheck() == B_NO_INIT );
		CPPUNIT_ASSERT( path.Unflatten(B_REF_TYPE, buffer, size) == B_OK );
		CPPUNIT_ASSERT( path == pathName );
		path.Unset();
	}
	// bad args
	NextSubTest();
// R5: crashs, when passing a NULL buffer
// R5: doesn't check the buffer size
	CPPUNIT_ASSERT( path.SetTo("/boot/home") == B_OK );
#if !TEST_R5
	CPPUNIT_ASSERT( path.Flatten(NULL, sizeof(buffer)) == B_BAD_VALUE );
	CPPUNIT_ASSERT( path.Flatten(buffer, path.FlattenedSize() - 2)
					== B_BAD_VALUE );
#endif
	path.Unset();
}

