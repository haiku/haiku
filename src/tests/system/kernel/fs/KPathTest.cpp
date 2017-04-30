/*
 * Copyright 2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KPathTest.h"

#include <stdlib.h>
#include <string.h>

#include <fs/KPath.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


typedef void* mutex;


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


// #pragma mark - DEBUG only


extern "C" void*
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	return malloc(size);
}


extern "C" status_t
_mutex_lock(mutex* lock, void* locker)
{
	return B_OK;
}


extern "C" status_t
_mutex_trylock(mutex* lock)
{
	return B_OK;
}


extern "C" void
_mutex_unlock(mutex* lock)
{
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
	CPPUNIT_ASSERT_MESSAGE("1. ", status == B_OK);

	status = path.SetTo("a/b/c");
	CPPUNIT_ASSERT_MESSAGE("2. ", status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);

	status = path.SetPath("abc/def");
	CPPUNIT_ASSERT_MESSAGE("3. ", status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "abc/def") == 0);
	CPPUNIT_ASSERT(path.Length() == 7);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);

	status = path.SetTo("a/b/c", false, 10);
	CPPUNIT_ASSERT_MESSAGE("4. ", status == B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);
	CPPUNIT_ASSERT(path.BufferSize() == 10);

	status = path.SetPath("sorry/i'm/too/long");
	CPPUNIT_ASSERT(status == B_BUFFER_OVERFLOW);
	CPPUNIT_ASSERT(strcmp(path.Path(), "a/b/c") == 0);
	CPPUNIT_ASSERT(path.Length() == 5);

	status = path.SetTo(NULL, KPath::DEFAULT, SIZE_MAX);
	CPPUNIT_ASSERT(status == B_NO_MEMORY);
}


void
KPathTest::TestLazyAlloc()
{
	KPath path(NULL, KPath::LAZY_ALLOC);
	CPPUNIT_ASSERT(path.Path() == NULL);
	CPPUNIT_ASSERT(path.Length() == 0);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);
	CPPUNIT_ASSERT(path.InitCheck() == B_OK);

	path.SetPath("/b");
	CPPUNIT_ASSERT(path.Path() != NULL);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/b") == 0);
	CPPUNIT_ASSERT(path.Length() == 2);
	CPPUNIT_ASSERT(path.BufferSize() == B_PATH_NAME_LENGTH);

	KPath second("yo", KPath::LAZY_ALLOC);
	CPPUNIT_ASSERT(second.Path() != NULL);
	CPPUNIT_ASSERT(strcmp(second.Path(), "yo") == 0);
	CPPUNIT_ASSERT(second.Length() == 2);
	CPPUNIT_ASSERT(second.BufferSize() == B_PATH_NAME_LENGTH);

	status_t status = path.SetTo(NULL, KPath::LAZY_ALLOC, SIZE_MAX);
	CPPUNIT_ASSERT(status == B_OK);
	status = path.SetPath("test");
	CPPUNIT_ASSERT(status == B_NO_MEMORY);
	CPPUNIT_ASSERT(path.InitCheck() == B_NO_MEMORY);
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
	CPPUNIT_ASSERT(status == B_OK);

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
	CPPUNIT_ASSERT(two.InitCheck() == B_NO_INIT);

	two.SetTo(NULL, KPath::LAZY_ALLOC);
	CPPUNIT_ASSERT(two.InitCheck() == B_OK);
	CPPUNIT_ASSERT(two.Path() == NULL);
	CPPUNIT_ASSERT(two.Length() == 0);
	one.Adopt(two);

	CPPUNIT_ASSERT(two.InitCheck() == B_OK);
	CPPUNIT_ASSERT(one.Path() == NULL);
	CPPUNIT_ASSERT(one.Length() == 0);
	one.SetPath("test");
	CPPUNIT_ASSERT(one.Path() != NULL);
	CPPUNIT_ASSERT(strcmp(one.Path(), "test") == 0);
	CPPUNIT_ASSERT(one.Length() == 4);
}


void
KPathTest::TestLockBuffer()
{
	KPath path;
	CPPUNIT_ASSERT(path.Path() != NULL);
	CPPUNIT_ASSERT(path.Length() == 0);
	char* buffer = path.LockBuffer();
	CPPUNIT_ASSERT(path.Path() == buffer);
	strcpy(buffer, "test");
	CPPUNIT_ASSERT(path.Length() == 0);
	path.UnlockBuffer();
	CPPUNIT_ASSERT(path.Length() == 4);

	KPath second(NULL, KPath::LAZY_ALLOC);
	CPPUNIT_ASSERT(second.Path() == NULL);
	CPPUNIT_ASSERT(second.Length() == 0);
	buffer = second.LockBuffer();
	CPPUNIT_ASSERT(second.Path() == NULL);
	CPPUNIT_ASSERT(buffer == NULL);

	KPath third(NULL, KPath::LAZY_ALLOC);
	CPPUNIT_ASSERT(third.Path() == NULL);
	buffer = third.LockBuffer(true);
	CPPUNIT_ASSERT(third.Path() != NULL);
	CPPUNIT_ASSERT(buffer != NULL);
	strcpy(buffer, "test");
	CPPUNIT_ASSERT(third.Length() == 0);
	third.UnlockBuffer();
	CPPUNIT_ASSERT(third.Length() == 4);
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
	CPPUNIT_ASSERT(path.InitCheck() == B_NO_INIT);
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

	status = path.SetTo("test/../in", KPath::NORMALIZE);
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
		"KPathTest::TestLazyAlloc", &KPathTest::TestLazyAlloc));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestLeaf", &KPathTest::TestLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestReplaceLeaf", &KPathTest::TestReplaceLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestRemoveLeaf", &KPathTest::TestRemoveLeaf));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestAdopt", &KPathTest::TestAdopt));
	suite.addTest(new CppUnit::TestCaller<KPathTest>(
		"KPathTest::TestLockBuffer", &KPathTest::TestLockBuffer));
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
