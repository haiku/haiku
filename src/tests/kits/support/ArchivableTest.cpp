/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <Archivable.h>
#include <Debug.h>
#include <Entry.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>

#include <TestSuiteAddon.h>
#include <TestUtils.h>
#include <cppunit/Exception.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "remoteobjectdef/RemoteTestObject.h"


const char* gInvalidClassName = "TInvalidClassName";
const char* gInvalidSig = "application/x-vnd.InvalidSignature";
const char* gLocalClassName = "TIOTest";
const char* gLocalSig = "application/x-vnd.LocalSignature";
const char* gRemoteClassName = "TRemoteTestObject";
const char* gRemoteSig = "application/x-vnd.RemoteObjectDef";
const char* gValidSig = gRemoteSig;

#ifndef TEST_R5
const char* gRemoteLib = "/lib/libsupporttest_RemoteTestObject.so";
#else
const char* gRemoteLib = "/lib/libsupporttest_RemoteTestObject_r5.so";
#endif


static void FormatAndThrow(int line, const char* file, const char* msg, int err) 
{
	std::string s("line: ");
	char lineStr[32];
	sprintf(lineStr, "%d", line);
	s += lineStr;
	s += " ";
	s += file;
	s += msg;
	s += strerror(err);
	s += "(";
	sprintf(lineStr, "%d", err);
	s += lineStr;
	s += ")";
	CppUnit::Exception re(s.c_str());
	throw re;
}


#define FORMAT_AND_THROW(MSG, ERR) FormatAndThrow(__LINE__, __FILE__, MSG, ERR)


class TIOTest : public BArchivable {
public:
	TIOTest(int32 i)
		: fData(i)
	{
	}

	TIOTest(BMessage* archive)
	{
		if (archive->FindInt32("TIOTest::data", &fData) != B_OK)
			fData = 0;
	}

	int32 GetData() const { return fData; }

	status_t Archive(BMessage* archive, bool deep = true) const
	{
		status_t err = archive->AddString("class", "TIOTest");
		if (!err)
			err = archive->AddInt32("TIOTest::data", fData);

		return err;
	}

	static TIOTest* Instantiate(BMessage* archive)
	{
		if (validate_instantiation(archive, "TIOTest"))
			return new TIOTest(archive);

		return NULL;
	}

private:
	int32 fData;
};


class ArchivableTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ArchivableTest);
	//CPPUNIT_TEST(Perform_ZeroCodeNullArg_ReturnsError);
	CPPUNIT_TEST(Archive_NullArchiveShallow_ReturnsBadValue);
	CPPUNIT_TEST(Archive_ValidArchiveShallow_ReturnsOk);
	CPPUNIT_TEST(Archive_NullArchiveDeep_ReturnsBadValue);
	CPPUNIT_TEST(Archive_ValidArchiveDeep_ReturnsOk);
	CPPUNIT_TEST_SUITE_END();

public:
	void Perform_ZeroCodeNullArg_ReturnsError()
	{
		BArchivable archive;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, archive.Perform(0, NULL));
	}

	void Archive_NullArchiveShallow_ReturnsBadValue()
	{
		BArchivable archive;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, archive.Archive(NULL, false));
	}

	void Archive_ValidArchiveShallow_ReturnsOk()
	{
		BMessage storage;
		BArchivable archive;
		CPPUNIT_ASSERT_EQUAL(B_OK, archive.Archive(&storage, false));
		const char* name;
		CPPUNIT_ASSERT_EQUAL(B_OK, storage.FindString("class", &name));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(name, "BArchivable"));
	}

	void Archive_NullArchiveDeep_ReturnsBadValue()
	{
		BArchivable archive;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, archive.Archive(NULL, true));
	}

	void Archive_ValidArchiveDeep_ReturnsOk()
	{
		BMessage storage;
		BArchivable archive;
		CPPUNIT_ASSERT_EQUAL(B_OK, archive.Archive(&storage, true));
		const char* name;
		CPPUNIT_ASSERT_EQUAL(B_OK, storage.FindString("class", &name));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(name, "BArchivable"));
	}
};


class FindInstantiationFuncTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(FindInstantiationFuncTest);
	CPPUNIT_TEST(FindInstantiationFunc_NullArgs_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_InvalidClassNullSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_NullClassInvalidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_InvalidClassAndSig_ReturnsNull);
	//CPPUNIT_TEST(FindInstantiationFunc_LocalClassNullSig_ReturnsValidFunc);
	//CPPUNIT_TEST(FindInstantiationFunc_RemoteClassNullSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_LocalClassInvalidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_RemoteClassInvalidSig_ReturnsNull);
	//CPPUNIT_TEST(FindInstantiationFunc_LocalClassValidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_RemoteClassValidSig_ReturnsNull);
#ifndef TEST_R5
	CPPUNIT_TEST(FindInstantiationFunc_Message_NullArchive_ReturnsNull);
#endif
	CPPUNIT_TEST(FindInstantiationFunc_Message_InvalidClassNullSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_Message_NullClassInvalidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_Message_InvalidClassAndSig_ReturnsNull);
	//CPPUNIT_TEST(FindInstantiationFunc_Message_LocalClassNullSig_ReturnsValidFunc);
	CPPUNIT_TEST(FindInstantiationFunc_Message_RemoteClassNullSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_Message_LocalClassInvalidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_Message_RemoteClassInvalidSig_ReturnsNull);
	//CPPUNIT_TEST(FindInstantiationFunc_Message_LocalClassValidSig_ReturnsNull);
	CPPUNIT_TEST(FindInstantiationFunc_Message_RemoteClassValidSig_ReturnsNull);
	CPPUNIT_TEST_SUITE_END();

public:
	void FindInstantiationFunc_NullArgs_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(NULL, NULL);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_InvalidClassNullSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gInvalidClassName, NULL);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_NullClassInvalidSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(NULL, gInvalidSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_InvalidClassAndSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gInvalidClassName, gInvalidSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_LocalClassNullSig_ReturnsValidFunc()
	{
		instantiation_func f = find_instantiation_func(gLocalClassName, NULL);
		CPPUNIT_ASSERT(f != NULL);

		BMessage archive;
		archive.AddString("class", gLocalClassName);
		TIOTest* test = dynamic_cast<TIOTest*>(f(&archive));
		CPPUNIT_ASSERT(test != NULL);
	}

	void FindInstantiationFunc_RemoteClassNullSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gRemoteClassName, NULL);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_LocalClassInvalidSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gLocalClassName, gInvalidSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_RemoteClassInvalidSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gRemoteClassName, gInvalidSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	/**
	 * \note
	 * This test is not currently used; can't obtain the local
	 * signature without a BApplication object (gLocalSig is a
	 * placeholder).
	 */
	void FindInstantiationFunc_LocalClassValidSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gLocalClassName, gLocalSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_RemoteClassValidSig_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func(gRemoteClassName, gRemoteSig);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_NullArchive_ReturnsNull()
	{
		instantiation_func f = find_instantiation_func((BMessage*)NULL);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_InvalidClassNullSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gInvalidClassName);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_NullClassInvalidSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("add_on", gInvalidSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_InvalidClassAndSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gInvalidClassName);
		archive.AddString("add_on", gInvalidSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_LocalClassNullSig_ReturnsValidFunc()
	{
		BMessage archive;
		archive.AddString("class", gLocalClassName);

		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f != NULL);

		TIOTest* test = dynamic_cast<TIOTest*>(f(&archive));
		CPPUNIT_ASSERT(test != NULL);
	}

	void FindInstantiationFunc_Message_RemoteClassNullSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_LocalClassInvalidSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gLocalClassName);
		archive.AddString("add_on", gInvalidSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_RemoteClassInvalidSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gInvalidSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	/**
	 * \note
	 * This test is not currently used; can't obtain the local
	 * signature without a BApplication object (gLocalSig is a
	 * placeholder).
	 */
	void FindInstantiationFunc_Message_LocalClassValidSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gLocalClassName);
		archive.AddString("add_on", gLocalSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}

	void FindInstantiationFunc_Message_RemoteClassValidSig_ReturnsNull()
	{
		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gRemoteSig);
		instantiation_func f = find_instantiation_func(&archive);
		CPPUNIT_ASSERT(f == NULL);
	}
};


class InstantiateObjectTest : public CppUnit::TestFixture {
public:
	InstantiateObjectTest() : fAddonId(B_ERROR) {}

	CPPUNIT_TEST_SUITE(InstantiateObjectTest);
	CPPUNIT_TEST(InstantiateObject_NullArchive_ReturnsNullAndBadValue);
	CPPUNIT_TEST(InstantiateObject_NoClassName_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_InvalidClassName_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_InvalidClassAndSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_InvalidClassValidSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_LocalClass_ReturnsInstance);
	CPPUNIT_TEST(InstantiateObject_LoadedRemoteClass_ReturnsInstance);
	CPPUNIT_TEST(InstantiateObject_RemoteClassNoSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_LocalClassInvalidSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_LoadedRemoteClassInvalidSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_RemoteClassInvalidSignature_ReturnsNull);
	CPPUNIT_TEST(InstantiateObject_LoadedRemoteClassValidSignature_ReturnsInstance);
	CPPUNIT_TEST(InstantiateObject_RemoteClassValidSignature_ReturnsInstance);
	CPPUNIT_TEST_SUITE_END();

	void InstantiateObject_NullArchive_ReturnsNullAndBadValue()
	{
		errno = B_OK;
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(NULL, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	}

	void InstantiateObject_NoClassName_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	void InstantiateObject_InvalidClassName_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gInvalidClassName);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	void InstantiateObject_InvalidClassAndSignature_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gInvalidClassName);
		archive.AddString("add_on", gInvalidSig);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_LAUNCH_FAILED_APP_NOT_FOUND, errno);
	}

	void InstantiateObject_InvalidClassValidSignature_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gInvalidClassName);
		archive.AddString("add_on", gValidSig);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT(id > 0);
		unload_add_on(id);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	}

	void InstantiateObject_LocalClass_ReturnsInstance()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gLocalClassName);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test != NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	void InstantiateObject_LoadedRemoteClass_ReturnsInstance()
	{
		errno = B_OK;
		LoadAddon();

		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		image_id id = B_OK;
		TRemoteTestObject* test = (TRemoteTestObject*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test != NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);

		UnloadAddon();
	}

	void InstantiateObject_RemoteClassNoSignature_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		CPPUNIT_ASSERT(archive.AddString("class", gRemoteClassName) == B_OK);
		image_id id = B_OK;
		TRemoteTestObject* test = (TRemoteTestObject*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	}

	void InstantiateObject_LocalClassInvalidSignature_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		CPPUNIT_ASSERT(archive.AddString("class", gLocalClassName) == B_OK);
		CPPUNIT_ASSERT(archive.AddString("add_on", gInvalidSig) == B_OK);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_LAUNCH_FAILED_APP_NOT_FOUND, errno);
	}

	void InstantiateObject_LoadedRemoteClassInvalidSignature_ReturnsNull()
	{
		errno = B_OK;
		LoadAddon();

		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gInvalidSig);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_LAUNCH_FAILED_APP_NOT_FOUND, errno);

		UnloadAddon();
	}

	void InstantiateObject_RemoteClassInvalidSignature_ReturnsNull()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gInvalidSig);
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_LAUNCH_FAILED_APP_NOT_FOUND, errno);
	}

	void InstantiateObject_LocalClassValidSignature_ReturnsInstance()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gLocalClassName);
		archive.AddString("add_on", GetLocalSignature().c_str());
		image_id id = B_OK;
		TIOTest* test = (TIOTest*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test != NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	void InstantiateObject_LoadedRemoteClassValidSignature_ReturnsInstance()
	{
		errno = B_OK;
		LoadAddon();

		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gRemoteSig);
		image_id id = B_OK;
		TRemoteTestObject* test = (TRemoteTestObject*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test != NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);

		UnloadAddon();
	}

	void InstantiateObject_RemoteClassValidSignature_ReturnsInstance()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", gRemoteClassName);
		archive.AddString("add_on", gRemoteSig);
		image_id id = B_OK;
		TRemoteTestObject* test = (TRemoteTestObject*)instantiate_object(&archive, &id);
		CPPUNIT_ASSERT(test != NULL);
		CPPUNIT_ASSERT(id > 0);
		unload_add_on(id);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

private:
	void LoadAddon()
	{
		if (fAddonId > 0)
			return;

		std::string libPath("lib/");
		libPath += gRemoteLib;
		fAddonId = load_add_on(libPath.c_str());

		if (fAddonId <= 0)
			FORMAT_AND_THROW(" failed to load addon: ", fAddonId);
	}

	void UnloadAddon()
	{
		if (fAddonId > 0) {
			status_t err = unload_add_on(fAddonId);
			fAddonId = B_ERROR;
			if (err)
				FORMAT_AND_THROW(" failed to unload addon: ", err);
		}
	}

	std::string GetLocalSignature()
	{
		BRoster roster;
		app_info ai;

		thread_id tid = find_thread(NULL);
		thread_info ti;
		status_t err = get_thread_info(tid, &ti);
		if (err)
			FORMAT_AND_THROW(" failed to get thread_info: ", err);

		team_info info;
		err = get_team_info(ti.team, &info);
		if (err)
			FORMAT_AND_THROW(" failed to get team_info: ", err);

		err = roster.GetRunningAppInfo(info.team, &ai);
		if (err)
			FORMAT_AND_THROW(" failed to get app_info: ", err);

		return ai.signature;
	}

private:
	image_id fAddonId;
};


class ValidateInstantiationTest : public CppUnit::TestFixture {
public:
	CPPUNIT_TEST_SUITE(ValidateInstantiationTest);
#ifndef TEST_R5
	CPPUNIT_TEST(ValidateInstantiation_NullParams_ReturnsFalse);
	CPPUNIT_TEST(ValidateInstantiation_NullArchive_ReturnsFalse);
#endif
	CPPUNIT_TEST(ValidateInstantiation_NullClassName_ReturnsFalse);
	CPPUNIT_TEST(ValidateInstantiation_NoClassField_ReturnsFalse);
	CPPUNIT_TEST(ValidateInstantiation_MismatchedClassField_ReturnsFalse);
	CPPUNIT_TEST(ValidateInstantiation_ValidParams_ReturnsTrue);
	CPPUNIT_TEST_SUITE_END();

	void ValidateInstantiation_NullParams_ReturnsFalse()
	{
		errno = B_OK;
		CPPUNIT_ASSERT_EQUAL(false, validate_instantiation(NULL, NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	}

	void ValidateInstantiation_NullClassName_ReturnsFalse()
	{
		errno = B_OK;
		BMessage archive;
		CPPUNIT_ASSERT_EQUAL(false, validate_instantiation(&archive, NULL));
		CPPUNIT_ASSERT_EQUAL(B_MISMATCHED_VALUES, errno);
	}

	void ValidateInstantiation_NullArchive_ReturnsFalse()
	{
		errno = B_OK;
		CPPUNIT_ASSERT_EQUAL(false, validate_instantiation(NULL, "FooBar"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	}

	void ValidateInstantiation_NoClassField_ReturnsFalse()
	{
		errno = B_OK;
		BMessage archive;
		CPPUNIT_ASSERT_EQUAL(false, validate_instantiation(&archive, "FooBar"));
		CPPUNIT_ASSERT_EQUAL(B_MISMATCHED_VALUES, errno);
	}

	void ValidateInstantiation_MismatchedClassField_ReturnsFalse()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", "FooBar");
		CPPUNIT_ASSERT_EQUAL(false, validate_instantiation(&archive, "BarFoo"));
		CPPUNIT_ASSERT_EQUAL(B_MISMATCHED_VALUES, errno);
	}

	void ValidateInstantiation_ValidParams_ReturnsTrue()
	{
		errno = B_OK;
		BMessage archive;
		archive.AddString("class", "FooBar");
		CPPUNIT_ASSERT_EQUAL(true, validate_instantiation(&archive, "FooBar"));
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ArchivableTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FindInstantiationFuncTest, getTestSuiteName());
// CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(InstantiateObjectTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidateInstantiationTest, getTestSuiteName());
