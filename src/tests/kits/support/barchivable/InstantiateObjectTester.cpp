//------------------------------------------------------------------------------
//	InstantiateObjectTester.cpp
//
/**
	Testing of instantiate_object(BMessage* archive, image_id* id)
	@note	No cases are currently defined for NULL 'id' parameter, since NULL
			is a valid value for it.  Perhaps there should be to ensure that the
			instantiate_object is, in fact, dealing with that case correctly.
			There are also no tests against instantiate_object(BMessage*) as it
			simply calls instantiate_object(BMessage*, image_id*) with NULL for
			the image_id parameter.
 */
//------------------------------------------------------------------------------

#include "InstantiateObjectTester.h"

// Standard Includes -----------------------------------------------------------
#include <errno.h>
#include <stdexcept>
#include <iostream>

// System Includes -------------------------------------------------------------
#include <Roster.h>
#include <Entry.h>
#include <Path.h>

// Project Includes ------------------------------------------------------------
#include <cppunit/Exception.h>
#include <TestShell.h>

// Local Includes --------------------------------------------------------------
#include "remoteobjectdef/RemoteTestObject.h"
#include "LocalTestObject.h"

using namespace std;

// Local Defines ---------------------------------------------------------------
#define FORMAT_AND_THROW(MSG, ERR)	\
	FormatAndThrow(__LINE__, __FILE__, MSG, ERR)

// Globals ---------------------------------------------------------------------
const char* gInvalidClassName	= "TInvalidClassName";
const char* gInvalidSig			= "application/x-vnd.InvalidSignature";
const char* gLocalClassName		= "TIOTest";
const char* gLocalSig			= "application/x-vnd.LocalSignature";
const char* gRemoteClassName	= "TRemoteTestObject";
const char* gRemoteSig			= "application/x-vnd.RemoteObjectDef";
const char* gValidSig			= gRemoteSig;
#if !TEST_R5
const char* gRemoteLib			= "/lib/libsupporttest_RemoteTestObject.so";
#else
const char* gRemoteLib			= "/lib/libsupporttest_RemoteTestObject_r5.so";
#endif

void FormatAndThrow(int line, const char* file, const char* msg, int err);

//------------------------------------------------------------------------------
TInstantiateObjectTester::TInstantiateObjectTester(string name)
	:	BTestCase(name), fAddonId(B_ERROR)
{
	;
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Invalid archive
	@param archive	NULL
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE.
					errno is set to B_BAD_VALUE.
 */
void TInstantiateObjectTester::Case1()
{
	errno = B_OK;
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(NULL, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			No class name
	@param archive	Valid BMessage pointer without string field "class"
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE.
					errno is set to B_OK.
 */
void TInstantiateObjectTester::Case2()
{
	errno = B_OK;
	BMessage Archive;
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//	Invalid class name tests
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Invalid class name
	@param archive	Valid BMessage pointer, with string field labeled "class"
					containing an invalid class name
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE.
					errno is set to B_BAD_VALUE.
 */
void TInstantiateObjectTester::Case3()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gInvalidClassName);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Invalid class name and signature
	@param archive	Valid BMessage pointer, with string fields labeled "class"
					and "add_on", containing invalid class name and signature,
					respectively
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE.
					errno is set to B_LAUNCH_FAILED_APP_NOT_FOUND.
 */
void TInstantiateObjectTester::Case4()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gInvalidClassName);
	Archive.AddString("add_on", gInvalidSig);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_LAUNCH_FAILED_APP_NOT_FOUND);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Invalid class name, valid signature
	@param archive	Valid BMessage pointer with string fields labeled "class"
					and "add_on", containing invalid class name and valid
					signature, respectively
	@param id		Valid image_id pointer
	@requires		RemoteObjectDef add-on must be built and accessible
	@results		Returns NULL.
					*id is > 0 (add-on was loaded)
					errno is set to B_BAD_VALUE.
 */
void TInstantiateObjectTester::Case5()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gInvalidClassName);
	Archive.AddString("add_on", gValidSig);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	// The system implementation returns the image_id of the last addon searched
	// Implies the addon is not unloaded.  How to verify this behaviour?  Should
	// the addon be unloaded if it doesn't contain our function?  Addons do,
	// after all, eat into our allowable memory.

	// Verified that addon is *not* unloaded in the Be implementation.  If Case8
	// runs after this case without explicitely unloaded the addon here, it
	// fails because it depends on the addon image not being available within
	// the team.
	CPPUNIT_ASSERT(id > 0);
	unload_add_on(id);
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	Valid class name tests
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of class defined in local image
	@param archive	Valid BMessage pointer with string field "class" containing
					name of locally defined class which can be instantiated via
					archiving mechanism
	@param id		Valid image_id pointer
	@requires		locally defined class which can be instantiated via
					archiving mechanism
	@results		Returns valid TIOTest instance.
					*id is set to B_BAD_VALUE (no image was loaded).
					errno is set to B_OK.
 */
//	No sig
//		Local app -- local class
void TInstantiateObjectTester::Case6()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gLocalClassName);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test != NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of class defined in add-on explicitely loaded
					by this team
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely defined class which can be instantiated via
					archiving mechanism
	@param id		Valid image_id pointer
	@requires		RemoteObjectDef add-on must be built and accessible
	@results		Returns valid TRemoteTestObject instance.
					*id is set to B_BAD_VALUE (no image was loaded).
					errno is set to B_OK.
 */
void TInstantiateObjectTester::Case7()
{
	errno = B_OK;
	LoadAddon();

	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	image_id id = B_OK;
	TRemoteTestObject* Test = (TRemoteTestObject*)instantiate_object(&Archive,
																	 &id);
	CPPUNIT_ASSERT(Test != NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_OK);

	UnloadAddon();
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of remotely-defined class, without required
					signature of the defining add-on
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely-defined class; no "add-on" field
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE (no image loaded).
					errno is set to B_BAD_VALUE.
 */
void TInstantiateObjectTester::Case8()
{debugger(__PRETTY_FUNCTION__);
	errno = B_OK;
	BMessage Archive;
	CPPUNIT_ASSERT(Archive.AddString("class", gRemoteClassName) == B_OK);
	image_id id = B_OK;
	TRemoteTestObject* Test = (TRemoteTestObject*)instantiate_object(&Archive,
																	 &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive naming locally defined class with invalid
					signature
	@param archive	Valid BMessage pointer with string field "class" containing
					name of locally defined class and string field "add_on"
					containing invalid signature
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE (no image loaded).
					errno is set to B_LAUNCH_FAILED_APP_NOT_FOUND.
 */
void TInstantiateObjectTester::Case9()
{
	errno = B_OK;
	BMessage Archive;
	CPPUNIT_ASSERT(Archive.AddString("class", gLocalClassName) == B_OK);
	CPPUNIT_ASSERT(Archive.AddString("add_on", gInvalidSig) == B_OK);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_LAUNCH_FAILED_APP_NOT_FOUND);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of class defined in add-on explicitely loaded
					by this team, but with an invalid signature
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely-defined class and string field "add_on"
					containing invalid signature
	@param id		Valid image_id pointer
	@requires		RemoteObjectDef add-on must be built and accessible
	@results		Returns NULL.
					*id is set to B_BAD_VALUE (no image loaded).
					errno is set to B_LAUNCH_FAILED_APP_NOT_FOUND.
 */
void TInstantiateObjectTester::Case10()
{
	errno = B_OK;
	LoadAddon();

	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gInvalidSig);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_LAUNCH_FAILED_APP_NOT_FOUND);

	UnloadAddon();
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of remotely-defined class, with invalid
					signature
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely-defined class and string field add-on
					containing invalid signature
	@param id		Valid image_id pointer
	@results		Returns NULL.
					*id is set to B_BAD_VALUE.
					errno is set to B_LAUNCH_FAILED_APP_NOT_FOUND
 */
void TInstantiateObjectTester::Case11()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gInvalidSig);
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test == NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_LAUNCH_FAILED_APP_NOT_FOUND);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of locally-defined class with correct
					signature
	@param archive	Valid BMessage pointer with string field "class" containing
					name of locally-defined class and string field "add_on"
					containing signature of current team
	@param id		Valid image_id pointer
	@requires		locally defined class which can be instantiated via
					archiving mechanism
	@results		Returns valid TIOTest instance.
					*id is set to B_BAD_VALUE (no image loaded).
					errno is set to B_OK.
	@note			This test is not currently used; GetLocalSignature() doesn't
					seem to work without a BApplication instance constructed.
					See GetLocalSignature() for more info.
 */
void TInstantiateObjectTester::Case12()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gLocalClassName);
	Archive.AddString("add_on", GetLocalSignature().c_str());
	image_id id = B_OK;
	TIOTest* Test = (TIOTest*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test != NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of class defined in add-on explicitely loaded
					by this team with signature of add-on
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely-defined class and string field "add_on"
					containing signature of loaded add-on
	@param id		Valid image_id pointer
	@requires		RemoteObjectDef add-on must be built and accessible
	@results		Returns valid instance of TRemoteTestObject.
					*id is set to B_BAD_VALUE (image load not necessary).
					errno is set to B_OK.
 */
void TInstantiateObjectTester::Case13()
{
	errno = B_OK;
	LoadAddon();

	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gRemoteSig);
	image_id id = B_OK;
	TRemoteTestObject* Test = (TRemoteTestObject*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test != NULL);
	CPPUNIT_ASSERT(id == B_BAD_VALUE);
	CPPUNIT_ASSERT(errno == B_OK);

	UnloadAddon();
}
//------------------------------------------------------------------------------
/**
	instantiate_object(BMessage* archive, image_id* id)
	@case			Valid archive of remotely-defined class with correct
					signature
	@param archive	Valid BMessage pointer with string field "class" containing
					name of remotely-defined class and string field "add_on"
					containing signature of defining add-on
	@param id		Valid image_id pointer
	@requires		RemoteObjectDef must be built and accessible
	@results		Returns valid instance of TRemoteTestObject.
					*id > 0 (image was loaded).
					errno is set to B_OK.
 */
void TInstantiateObjectTester::Case14()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gRemoteSig);
	image_id id = B_OK;
	TRemoteTestObject* Test = (TRemoteTestObject*)instantiate_object(&Archive, &id);
	CPPUNIT_ASSERT(Test != NULL);
	CPPUNIT_ASSERT(id > 0);
	unload_add_on(id);
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
CppUnit::Test* TInstantiateObjectTester::Suite()
{
	CppUnit::TestSuite* SuiteOfTests = new CppUnit::TestSuite;

//	SuiteOfTests->addTest(
//		new CppUnit::TestCaller<TInstantiateObjectTester>("BArchivable::instantiate_object() Test",
//			&TInstantiateObjectTester::RunTests));

	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case1);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case2);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case3);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case4);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case5);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case6);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case8);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case7);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case9);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case10);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case11);
//	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case12);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case13);
	ADD_TEST(SuiteOfTests, TInstantiateObjectTester, Case14);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------
void TInstantiateObjectTester::LoadAddon()
{
	if (fAddonId > 0)
		return;

	// We're not testing the roster, so I'm going to just
	// find the add-on manually.
	std::string libPath = std::string(BTestShell::GlobalTestDir()) + gRemoteLib;
	cout << "dir == '" << libPath << "'" << endl;
	fAddonId = load_add_on(libPath.c_str());

	RES(fAddonId);
	if (fAddonId <= 0)
	{
		FORMAT_AND_THROW(" failed to load addon: ", fAddonId);
	}
}
//------------------------------------------------------------------------------
void TInstantiateObjectTester::UnloadAddon()
{
	if (fAddonId > 0)
	{
		status_t err = unload_add_on(fAddonId);
		fAddonId = B_ERROR;
		if (err)
		{
			FORMAT_AND_THROW(" failed to unload addon: ", err);
		}
	}
}
//------------------------------------------------------------------------------
std::string TInstantiateObjectTester::GetLocalSignature()
{
	BRoster Roster;
	app_info ai;
	team_id team;

	// Get the team_id of this app
	thread_id tid = find_thread(NULL);
	thread_info ti;
	status_t err = get_thread_info(tid, &ti);
	if (err)
	{
		FORMAT_AND_THROW(" failed to get thread_info: ", err);
	}

	// Get the app_info via the team_id
	team = ti.team;
	team_info info;
	err = get_team_info(team, &info);
	if (err)
	{
		FORMAT_AND_THROW(" failed to get team_info: ", err);
	}

	team = info.team;

	// It seems that this call to GetRunningAppInfo() is not working because we
	// don't have an instance of BApplication somewhere -- the roster, therefore,
	// doesn't know about us.
	err = Roster.GetRunningAppInfo(team, &ai);
	if (err)
	{
		FORMAT_AND_THROW(" failed to get app_info: ", err);
	}

	// Return the signature from the app_info
	return ai.signature;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void FormatAndThrow(int line, const char *file, const char *msg, int err)
{
	std::string s("line: ");
	s += IntToStr(line);
	s += " ";
	s += file;
	s += msg;
	s += strerror(err);
	s += "(";
	s += IntToStr(err);
	s += ")";
	CppUnit::Exception re(s.c_str());
	throw re;
}
//------------------------------------------------------------------------------

void
TInstantiateObjectTester::RunTests() {
	NextSubTest();
	Case1();
	NextSubTest();
	Case2();
	NextSubTest();
	Case3();
	NextSubTest();
	Case4();
	NextSubTest();
	Case5();
	NextSubTest();
	Case6();
	NextSubTest();
	Case7();
	NextSubTest();
	Case8();
	NextSubTest();
	Case9();
	NextSubTest();
	Case10();
	NextSubTest();
	Case11();
	NextSubTest();
	Case12();
	NextSubTest();
	Case13();
	NextSubTest();
	Case14();
}

/*
 * $Log $
 *
 * $Id  $
 *
 */


