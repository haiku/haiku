/*
 * Copyright 2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KPathTest.h"

#include <string.h>

#include <fs/KPath.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


// Kernel stubs


extern "C" team_id
team_get_kernel_team_id(void)
{
	return 0;
}


extern "C" team_id
team_get_current_team_id(void)
{
	return 0;
}


extern "C" status_t
vfs_normalize_path(const char* path, char* buffer, size_t bufferSize,
	bool traverseLink, bool kernel)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


KPathTest::KPathTest()
{
}


KPathTest::~KPathTest()
{
}


void
KPathTest::TestSetToAndPath()
{
	KPath path;
	status_t status = path.InitCheck();
//	CPPUNIT_ASSERT(status == B_NO_INIT);

	status = path.SetTo("a/b/c");
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);

	status = path.SetPath("abc/def");
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "abc/def") == 0);
	CPPUNIT_ASSERT(path.Length() == 7);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);

	status = path.SetTo("a/b/c", false, 10);
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);
	CPPUNIT_ASSERT(path.BufferSize() == 10);

	status = path.SetPath("sorry/i'm/too/long");
	CPPUNIT_ASSERT(status == B_BUFFER_OVERFLOW);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);
}


void
KPathTest::TestLeaf()
{
	KPath path("a");
	CPPUNIT_ASSERT(strcmp(path.Path(), "a") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "a") == 0);

	path.SetTo("");
	CPPUNIT_ASSERT(strcmp(path.Path(), "") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "") == 0);

	path.SetTo("/");
	CPPUNIT_ASSERT(strcmp(path.Path(), "/") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "") == 0);

	path.SetTo("a/b");
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "b") == 0);

	path.SetTo("a/b/");
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "b") == 0);

	path.SetTo("/a/b//c");
	CPPUNIT_ASSERT(strcmp(path.Path(), "/a/b//c") == 0);
	CPPUNIT_ASSERT(strcmp(path.Leaf(), "c") == 0);
}


void
KPathTest::TestReplaceLeaf()
{
	KPath path;
	status_t status = path.ReplaceLeaf("x");
//	CPPUNIT_ASSERT(status == B_NO_INIT);

	path.SetTo("/a/b/c");
	CPPUNIT_ASSERT(path.Length() == 6);

	status = path.ReplaceLeaf(NULL);
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(path.Length() == 4);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/a/b") == 0);

	status = path.ReplaceLeaf("");
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(path.Length() == 2);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/a") == 0);

	status = path.ReplaceLeaf("c");
	CPPUNIT_ASSERT(status == B_OK);
	CPPUNIT_ASSERT(path.Length() == 2);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/c") == 0);
}


void
KPathTest::TestRemoveLeaf()
{
	KPath path;
	bool removed = path.RemoveLeaf();
	CPPUNIT_ASSERT(!removed);

	path.SetTo("a//b/c");
	removed = path.RemoveLeaf();
	CPPUNIT_ASSERT_MESSAGE("1. removed", removed);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a//b") == 0);
	CPPUNIT_ASSERT(path.Length() == 4);

	removed = path.RemoveLeaf();
	CPPUNIT_ASSERT_MESSAGE("2. removed", removed);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a") == 0);
	CPPUNIT_ASSERT(path.Length() == 1);

	removed = path.RemoveLeaf();
	CPPUNIT_ASSERT_MESSAGE("3. !removed", !removed);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a") == 0);
	CPPUNIT_ASSERT(path.Length() == 1);

	path.SetTo("/a");
	removed = path.RemoveLeaf();
	CPPUNIT_ASSERT_MESSAGE("4. removed", removed);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/") == 0);
	CPPUNIT_ASSERT(path.Length() == 1);

	removed = path.RemoveLeaf();
	CPPUNIT_ASSERT_MESSAGE("5. !removed", !removed);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/") == 0);
	CPPUNIT_ASSERT(path.Length() == 1);
}


void
KPathTest::TestAdopt()
{
	KPath one("first", false, 10);
	CPPUNIT_ASSERT(one.InitCheck() == B_OK);
	CPPUNIT_ASSERT(one.BufferSize() == 10);
	CPPUNIT_ASSERT(one.Length() == 5);
	KPath two("second", false, 20);
	CPPUNIT_ASSERT(two.InitCheck() == B_OK);
	CPPUNIT_ASSERT(two.BufferSize() == 20);

	one.Adopt(two);
	CPPUNIT_ASSERT(one.InitCheck() == B_OK);
	CPPUNIT_ASSERT(one.BufferSize() == 20);
	CPPUNIT_ASSERT(one.Length() == 6);
	CPPUNIT_ASSERT(strcmp(one.Path(), "second") == 0);
	CPPUNIT_ASSERT(two.Length() == 0);
	CPPUNIT_ASSERT(two.BufferSize() == 0);
//	CPPUNIT_ASSERT(two.InitCheck() == B_NO_INIT);
}


void
KPathTest::TestDetachBuffer()
{
	KPath path("test");
	CPPUNIT_ASSERT(path.InitCheck() == B_OK);

	char* buffer = path.DetachBuffer();
	CPPUNIT_ASSERT(buffer != NULL);
	CPPUNIT_ASSERT(strcmp(buffer, "test") == 0);

	CPPUNIT_ASSERT(path.Path() == NULL);
//	CPPUNIT_ASSERT(path.InitCheck() == B_NO_INIT);
}


void
KPathTest::TestNormalize()
{
	// Outside the kernel, we only test the error case.
	KPath path("test/../out");
	CPPUNIT_ASSERT(path.InitCheck() == B_OK);

	status_t status = path.Normalize(true);
	CPPUNIT_ASSERT(status == B_NOT_SUPPORTED);
	CPPUNIT_ASSERT(path.Path() != NULL);
	CPPUNIT_ASSERT(path.Path()[0] == '\0');
	CPPUNIT_ASSERT(path.Path() == path.Leaf());
}


void
KPathTest::TestAssign()
{
	KPath one("first", false, 10);
	CPPUNIT_ASSERT(one.Length() == 5);
	KPath two("second", false, 20);

	two = one;
	CPPUNIT_ASSERT(strcmp(two.Path(), one.Path()) == 0);
	CPPUNIT_ASSERT(two.Path() != one.Path());
	CPPUNIT_ASSERT(two.BufferSize() == one.BufferSize());
	CPPUNIT_ASSERT(two.Length() == one.Length());

	one = "/whatever";
	CPPUNIT_ASSERT(one.Length() == 9);
	CPPUNIT_ASSERT(one.BufferSize() == two.BufferSize());
	CPPUNIT_ASSERT(strcmp(one.Path(), "/whatever") == 0);
}


void
KPathTest::TestEquals()
{
	KPath a("one");
	KPath b("two");
	CPPUNIT_ASSERT_MESSAGE("1.", !(a == b));

	b = a;
	CPPUNIT_ASSERT_MESSAGE("2.", a == b);

	b = "ones";
	CPPUNIT_ASSERT_MESSAGE("3.", !(a == b));
}


void
KPathTest::TestNotEquals()
{
	KPath a("one");
	KPath b("two");
	CPPUNIT_ASSERT_MESSAGE("1.", a != b);

	b = a;
	CPPUNIT_ASSERT_MESSAGE("2.", !(a != b));

	b = "ones";
	CPPUNIT_ASSERT_MESSAGE("3.", a != b);
}


/*static*/ void
KPathTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("KPathTest");

	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestSetToAndPath", &KPathTest::TestSetToAndPath));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestLeaf", &KPathTest::TestLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestReplaceLeaf", &KPathTest::TestReplaceLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestRemoveLeaf", &KPathTest::TestRemoveLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestAdopt", &KPathTest::TestAdopt));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestDetachBuffer", &KPathTest::TestDetachBuffer));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestNormalize", &KPathTest::TestNormalize));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestAssign", &KPathTest::TestAssign));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestEquals", &KPathTest::TestEquals));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestNotEquals", &KPathTest::TestNotEquals));

	parent.addTest("KPathTest", &suite);
}
