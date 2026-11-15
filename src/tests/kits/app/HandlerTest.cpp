/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <string>

#include <Message.h>
#include <OS.h>
#include <Handler.h>
#include <Looper.h>
#include <MessageFilter.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class HandlerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HandlerTest);
	CPPUNIT_TEST(NullName_Constructor_NameIsNull);
	CPPUNIT_TEST(ValidName_Constructor_NameMatches);
	CPPUNIT_TEST(ArchiveWithName_Constructor_NameMatches);
	CPPUNIT_TEST(ArchiveNoName_Constructor_NameIsNull);
	CPPUNIT_TEST(NullArchive_Constructor_NameIsNull);
	CPPUNIT_TEST(NullDataDeepFalse_Archive_ReturnsBadValue);
	CPPUNIT_TEST(NullDataDeepTrue_Archive_ReturnsBadValue);
	CPPUNIT_TEST(DeepFalse_Archive_ArchiveHasName);
	CPPUNIT_TEST(DeepTrue_Archive_ArchiveHasName);
	CPPUNIT_TEST(NullData_Instantiate_ReturnsNull);
	CPPUNIT_TEST(ValidData_Instantiate_ReturnsHandler);
	CPPUNIT_TEST(ArchiveNoName_Instantiate_ReturnsHandler);
	CPPUNIT_TEST(Null_SetName_NameIsNull);
	CPPUNIT_TEST(Valid_SetName_NameMatches);
	CPPUNIT_TEST(MeaninglessData_Perform_ReturnsNotFound);
	CPPUNIT_TEST_SUITE_END();

public:
	/**
		BHandler(const char* name)
		@case			Construct with NULL name
		@param	name	NULL
		@results		BHandler::Name() should return NULL
	 */
	void NullName_Constructor_NameIsNull()
	{
		BHandler Handler((const char*)NULL);
		CPPUNIT_ASSERT(Handler.Name() == NULL);
	}

	/**
		BHandler(const char* name)
		@case			Construct with valid name
		@param	name	valid string
		@results		BHandler::Name() returns name
	 */
	void ValidName_Constructor_NameMatches()
	{
		BHandler Handler("name");
		CPPUNIT_ASSERT(std::string("name") == Handler.Name());
	}

	/**
		BHandler(BMessage* archive)
		@case			archive is valid and has field "_name"
		@param	archive	valid BMessage pointer
		@results		BHandler::Name() returns _name
	 */
	void ArchiveWithName_Constructor_NameMatches()
	{
		BMessage Archive;
		Archive.AddString("_name", "the name");
		BHandler Handler(&Archive);
		CPPUNIT_ASSERT(std::string("the name") == Handler.Name());
	}

	/**
		BHandler(BMessage* archive)
		@case			archive is valid, but has no field "_name"
		@param	archive	valid BMessage pointer
		@results		BHandler::Name() returns NULL
	 */
	void ArchiveNoName_Constructor_NameIsNull()
	{
		BMessage Archive;
		BHandler Handler(&Archive);
		CPPUNIT_ASSERT(Handler.Name() == NULL);
	}

	/**
		BHandler(BMessage* archive)
		@case			archive is NULL
		@param	archive	NULL
		@results		BHandler::Name() returns NULL
		@note			This test is not enabled against the original implementation
						as it doesn't check for a NULL parameter and seg faults.
	 */
	void NullArchive_Constructor_NameIsNull()
	{
	#ifndef TEST_R5
		BHandler Handler((BMessage*)NULL);
		CPPUNIT_ASSERT(Handler.Name() == NULL);
	#endif
	}

	/**
		Archive(BMessage* data, bool deep = true)
		@case			data is NULL, deep is false
		@param	data	NULL
		@param	deep	false
		@results		Returns B_BAD_VALUE
		@note			This test is not enabled against the original implementation
						as it doesn't check for NULL parameters and seg faults
	 */
	void NullDataDeepFalse_Archive_ReturnsBadValue()
	{
	#ifndef TEST_R5
		BHandler Handler;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, Handler.Archive(NULL, false));
	#endif
	}

	/**
		Archive(BMessage* data, bool deep = true)
		@case			data is NULL, deep is true
		@param	data	NULL
		@param	deep	true
		@results		Returns B_BAD_VALUE
		@note			This test is not enabled against the original implementation
						as it doesn't check for NULL parameters and seg faults
	 */
	void NullDataDeepTrue_Archive_ReturnsBadValue()
	{
	#ifndef TEST_R5
		BHandler Handler;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, Handler.Archive(NULL));
	#endif
	}

	/**
		Archive(BMessage* data, bool deep = true)
		@case			data is valid, deep is false
		@param	data	valid BMessage pointer
		@param	deep	false
		@results		Returns B_OK
						Resultant archive has string field labeled "_name"
						Field "_name" contains the string "a name"
						Resultant archive has string field labeled "class"
						Field "class" contains the string "BHandler"
	 */
	void DeepFalse_Archive_ArchiveHasName()
	{
		BMessage Archive;
		BHandler Handler("a name");
		CPPUNIT_ASSERT_EQUAL(B_OK, Handler.Archive(&Archive, false));

		const char* data;
		CPPUNIT_ASSERT_EQUAL(B_OK, Archive.FindString("_name", &data));
		CPPUNIT_ASSERT(std::string("a name") == data);
		CPPUNIT_ASSERT_EQUAL(B_OK, Archive.FindString("class", &data));
		CPPUNIT_ASSERT(std::string("BHandler") == data);
	}

	/**
		Archive(BMessage *data, bool deep = true)
		@case			data is valid, deep is true
		@param	data	valid BMessage pointer
		@param	deep	true
		@results		Returns B_OK
						Resultant archive has string field labeled "_name"
						Field "_name" contains the string "a name"
						Resultant archive has string field labeled "class"
						Field "class" contains the string "BHandler"
	 */
	void DeepTrue_Archive_ArchiveHasName()
	{
		BMessage Archive;
		BHandler Handler("another name");
		CPPUNIT_ASSERT_EQUAL(B_OK, Handler.Archive(&Archive));

		const char* data;
		CPPUNIT_ASSERT_EQUAL(B_OK, Archive.FindString("_name", &data));
		CPPUNIT_ASSERT(std::string("another name") == data);
		CPPUNIT_ASSERT_EQUAL(B_OK, Archive.FindString("class", &data));
		CPPUNIT_ASSERT(std::string("BHandler") == data);
	}

	/**
		Instantiate(BMessage* data)
		@case			data is NULL
		@param	data	NULL
		@results
		@note			This test is not enabled against the original implementation
						as it doesn't check for NULL parameters and seg faults
	 */
	void NullData_Instantiate_ReturnsNull()
	{
	#ifndef TEST_R5
		CPPUNIT_ASSERT(BHandler::Instantiate(NULL) == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, errno);
	#endif
	}

	/**
		Instantiate(BMessage* data)
		@case			data is valid, has field "_name"
		@param	data	Valid BMessage pointer with string field "class" containing
						string "BHandler" and with string field "_name" containing
						string "a name"
		@results		BHandler::Name() returns "a name"
	 */
	void ValidData_Instantiate_ReturnsHandler()
	{
		BMessage Archive;
		Archive.AddString("class", "BHandler");
		Archive.AddString("_name", "a name");

		BHandler* Handler =
			dynamic_cast<BHandler*>(BHandler::Instantiate(&Archive));
		CPPUNIT_ASSERT(Handler != NULL);
		CPPUNIT_ASSERT(std::string("a name") == Handler->Name());
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	/**
		Instantiate(BMessage *data)
		@case			data is valid, has no field "_name"
		@param	data	valid BMessage pointer with string field "class" containing
						string "BHandler"
		@results		BHandler::Name() returns NULL
	 */
	void ArchiveNoName_Instantiate_ReturnsHandler()
	{
		BMessage Archive;
		Archive.AddString("class", "BHandler");

		BHandler* Handler =
			dynamic_cast<BHandler*>(BHandler::Instantiate(&Archive));
		CPPUNIT_ASSERT(Handler != NULL);
		CPPUNIT_ASSERT(Handler->Name() == NULL);
		CPPUNIT_ASSERT_EQUAL(B_OK, errno);
	}

	/**
		SetName(const char* name)
		Name()
		@case			name is NULL
		@param	name	NULL
		@results		BHandler::Name() returns NULL

	 */
	void Null_SetName_NameIsNull()
	{
		BHandler Handler("a name");
		CPPUNIT_ASSERT(std::string("a name") == Handler.Name());

		Handler.SetName(NULL);
		CPPUNIT_ASSERT(Handler.Name() == NULL);
	}

	/**
		SetName(const char *name)
		Name()
		@case			name is valid
		@param	name	Valid string pointer
		@results		BHandler::Name returns name
	 */
	void Valid_SetName_NameMatches()
	{
		BHandler Handler("a name");
		CPPUNIT_ASSERT(std::string("a name") == Handler.Name());

		Handler.SetName("another name");
		CPPUNIT_ASSERT(std::string("another name") == Handler.Name());
	}

	/**
		Perform(perform_code d, void *arg)
		@case		feed meaningless data, should return B_NAME_NOT_FOUND
		@param	d	N/A
		@param	arg	NULL
		@results	Returns B_NAME_NOT_FOUND
	 */
	void MeaninglessData_Perform_ReturnsNotFound()
	{
		BHandler Handler;
		CPPUNIT_ASSERT_EQUAL(B_NAME_NOT_FOUND, Handler.Perform(0, NULL));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HandlerTest, getTestSuiteName());
