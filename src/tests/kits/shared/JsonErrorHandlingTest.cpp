/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include <Json.h>
#include <JsonEventListener.h>

#include <AutoDeleter.h>
#include <DataIO.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "JsonSamples.h"


using namespace BPrivate;


class ErrorCapturingListener : public BJsonEventListener {
public:
	ErrorCapturingListener()
	{
		fErrorStatus = B_OK;
		fFirstEventTypeAfterError = B_JSON_NULL; // least likely
		fEventCountAfterError = 0;
	}
	virtual				~ErrorCapturingListener() {}

	void				Complete() {}
	bool				HasEventsAfterError() { return fEventCountAfterError > 0; }
	json_event_type		FirstEventTypeAfterError() { return fFirstEventTypeAfterError; }
	BString				GetErrorMessage() { return fErrorMessage; }
	int32				GetErrorLine() { return fErrorLine; }
	status_t			ErrorStatus() { return fErrorStatus; }

	virtual void HandleError(status_t status, int32 line, const char* message)
	{
		fErrorStatus = status;
		fErrorLine = line;

		if (message != NULL)
			fErrorMessage = BString(message);
		else
			fErrorMessage = BString();
	}

	virtual bool Handle(const BJsonEvent& event)
	{
		if (fErrorStatus != B_OK) {
			if (fEventCountAfterError == 0)
				fFirstEventTypeAfterError = event.EventType();

			fEventCountAfterError++;
		}
		return true; // keep going.
	}

private:
	status_t		fErrorStatus;
	int32			fErrorLine;
	BString			fErrorMessage;
	json_event_type	fFirstEventTypeAfterError;
	int32			fEventCountAfterError;
};


/*! This DataIO concrete implementation is designed to open and then to fail
	in order to simulate what might happen if there were an IO problem when
	parsing some JSON.
*/
class FailingDataIO : public BDataIO {
public:
						FailingDataIO() {}
	virtual				~FailingDataIO() {}
	virtual	status_t	Flush() { return B_IO_ERROR; }
	virtual	ssize_t		Write(const void* buffer, size_t size) { return B_IO_ERROR; }
	virtual	ssize_t		Read(void* buffer, size_t size) { return B_IO_ERROR; }
};


class JsonErrorHandlingTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JsonErrorHandlingTest);
	CPPUNIT_TEST(TestIOIssue);
	CPPUNIT_TEST(TestBadNumber);
	CPPUNIT_TEST(TestBadStringEscape);
	CPPUNIT_TEST(TestStringUnterminated);
	CPPUNIT_TEST(TestObjectWithPrematureSeparator);
	CPPUNIT_TEST(TestCompletelyUnknown);
	CPPUNIT_TEST_SUITE_END();

	void _TestParseWithErrorMessage(BDataIO* inputData, int32 line, status_t expectedStatus,
			const char* expectedMessage)
	{
		ErrorCapturingListener listener;

		BPrivate::BJson::Parse(inputData, &listener);

		CPPUNIT_ASSERT(!listener.HasEventsAfterError());
		CPPUNIT_ASSERT_EQUAL(expectedStatus, listener.ErrorStatus());
		CPPUNIT_ASSERT_EQUAL(line, listener.GetErrorLine());
		CPPUNIT_ASSERT(0 == strcmp(expectedMessage, listener.GetErrorMessage().String()));
	}

	void _TestParseWithErrorMessage(const char* input, int32 line, status_t expectedStatus,
			const char* expectedMessage)
	{
		BMemoryIO inputData(input, strlen(input));
		_TestParseWithErrorMessage(&inputData, line, expectedStatus, expectedMessage);
	}

	void _TestParseWithUnexpectedCharacter(const char* input, int32 line, status_t expectedStatus,
			char expectedChar)
	{
		BString expectedMessage;
		expectedMessage.SetToFormat("unexpected character [%" B_PRIu8 "] (%c) when parsing element",
			static_cast<uint8>(expectedChar), expectedChar);

		_TestParseWithErrorMessage(input, line, expectedStatus, expectedMessage.String());
	}

	void _TestParseWithBadStringEscape(const char* input, int32 line, status_t expectedStatus,
			char expectedBadEscapeChar)
	{
		BString expectedMessage;
		expectedMessage.SetToFormat("unexpected escaped character [%c] in string parsing",
			expectedBadEscapeChar);

		_TestParseWithErrorMessage(input, line, expectedStatus, expectedMessage.String());
	}

public:
	void TestIOIssue()
	{
		FailingDataIO inputData;
		_TestParseWithErrorMessage(&inputData, -1, B_IO_ERROR, "io related read error");
	}

	void TestBadNumber()
	{
		_TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_NUMBER,
			1, B_BAD_DATA, "malformed number");
	}

	void TestBadStringEscape()
	{
		_TestParseWithBadStringEscape(JSON_SAMPLE_BROKEN_BAD_STRING_ESCAPE,
			1, B_BAD_DATA, 'v');
	}

	void TestStringUnterminated()
	{
		_TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_UNTERMINATED_STRING,
			1, B_BAD_DATA, "unexpected end of input");
	}

	void TestObjectWithPrematureSeparator()
	{
		_TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_OBJECT_PREMATURE_SEPARATOR,
			1, B_BAD_DATA, "unexpected item separator when parsing start of object");
	}

	void TestCompletelyUnknown()
	{
		_TestParseWithUnexpectedCharacter(JSON_SAMPLE_BROKEN_COMPLETELY_UNKNOWN,
			1, B_BAD_DATA, 'z');
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(JsonErrorHandlingTest, getTestSuiteName());
