/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#include "JsonErrorHandlingTest.h"

#include <AutoDeleter.h>

#include <Json.h>
#include <JsonEventListener.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "JsonSamples.h"


using namespace BPrivate;

class ErrorCapturingListener : public BJsonEventListener {
public:
								ErrorCapturingListener();
		virtual					~ErrorCapturingListener();

			bool				Handle(const BJsonEvent& event);
			void				HandleError(status_t status, int32 line,
									const char* message);
			void				Complete() {};

			status_t			ErrorStatus();
			int32				GetErrorLine();
			BString				GetErrorMessage();
			bool				HasEventsAfterError();
			json_event_type		FirstEventTypeAfterError();
private:
			status_t			fErrorStatus;
			int32				fErrorLine;
			BString				fErrorMessage;
			json_event_type		fFirstEventTypeAfterError;
			int32				fEventCountAfterError;

};


/*! This DataIO concrete implementation is designed to open and then to fail
    in order to simulate what might happen if there were an IO problem when
    parsing some JSON.
*/

class FailingDataIO : public BDataIO {
public:
									FailingDataIO();
		virtual						~FailingDataIO();

		ssize_t						Read(void* buffer, size_t size);
		ssize_t						Write(const void* buffer, size_t size);

		status_t					Flush();
};


// #pragma mark - FailingDataIO


FailingDataIO::FailingDataIO()
{
}


FailingDataIO::~FailingDataIO()
{
}


ssize_t
FailingDataIO::Read(void* buffer, size_t size)
{
	return B_IO_ERROR;
}


ssize_t
FailingDataIO::Write(const void* buffer, size_t size)
{
	fprintf(stdout, "attempt to write");
	return B_IO_ERROR;
}


status_t
FailingDataIO::Flush()
{
	return B_IO_ERROR;
}


// #pragma mark - ErrorCapturingListener


ErrorCapturingListener::ErrorCapturingListener()
{
	fErrorStatus = B_OK;
	fFirstEventTypeAfterError = B_JSON_NULL; // least likely
	fEventCountAfterError = 0;
}


ErrorCapturingListener::~ErrorCapturingListener()
{
}


bool
ErrorCapturingListener::Handle(const BJsonEvent& event)
{
	if (fErrorStatus != B_OK) {
		if (fEventCountAfterError == 0)
			fFirstEventTypeAfterError = event.EventType();

		fEventCountAfterError++;
	}
	return true; // keep going.
}


void
ErrorCapturingListener::HandleError(status_t status, int32 line,
	const char *message)
{
	fErrorStatus = status;
	fErrorLine = line;

	if (message != NULL)
		fErrorMessage = BString(message);
	else
		fErrorMessage = BString();
}


status_t
ErrorCapturingListener::ErrorStatus()
{
	return fErrorStatus;
}


int32
ErrorCapturingListener::GetErrorLine()
{
	return fErrorLine;
}


BString
ErrorCapturingListener::GetErrorMessage()
{
	return fErrorMessage;
}


json_event_type
ErrorCapturingListener::FirstEventTypeAfterError()
{
	return fFirstEventTypeAfterError;
}


bool
ErrorCapturingListener::HasEventsAfterError()
{
	return fEventCountAfterError > 0;
}


JsonErrorHandlingTest::JsonErrorHandlingTest()
{
}


JsonErrorHandlingTest::~JsonErrorHandlingTest()
{
}


void
JsonErrorHandlingTest::TestParseWithBadStringEscape(const char* input,
	int32 line, status_t expectedStatus, char expectedBadEscapeChar)
{
	BString expectedMessage;
	expectedMessage.SetToFormat("unexpected escaped character [%c] "
		"in string parsing", expectedBadEscapeChar);

	TestParseWithErrorMessage(input, line, expectedStatus,
		expectedMessage.String());
}


void
JsonErrorHandlingTest::TestParseWithUnterminatedElement(const char* input,
	int32 line, status_t expectedStatus)
{
	BString expectedMessage;
	expectedMessage.SetToFormat("unterminated element");

	TestParseWithErrorMessage(input, line, expectedStatus,
		expectedMessage.String());
}


void
JsonErrorHandlingTest::TestParseWithUnexpectedCharacter(const char* input,
	int32 line, status_t expectedStatus, char expectedChar)
{
	BString expectedMessage;
	expectedMessage.SetToFormat("unexpected character [%" B_PRIu8
		"] (%c) when parsing element", static_cast<uint8>(expectedChar),
		expectedChar);

	TestParseWithErrorMessage(input, line, expectedStatus,
		expectedMessage.String());
}


void
JsonErrorHandlingTest::TestParseWithErrorMessage(const char* input, int32 line,
	status_t expectedStatus, const char* expectedMessage)
{
	fprintf(stderr, "in >%s<\n", input);
	BDataIO *inputData = new BMemoryIO(input, strlen(input));
	ObjectDeleter<BDataIO> inputDataDeleter(inputData);
	TestParseWithErrorMessage(inputData, line, expectedStatus, expectedMessage);
}


void
JsonErrorHandlingTest::TestParseWithErrorMessage(BDataIO* inputData, int32 line,
	status_t expectedStatus, const char* expectedMessage)
{
	ErrorCapturingListener* listener = new ErrorCapturingListener();
	ObjectDeleter<ErrorCapturingListener> listenerDeleter(listener);

// ----------------------
	BPrivate::BJson::Parse(inputData, listener);
// ----------------------

	fprintf(stderr, "expected error at line %" B_PRIi32 " - %s : %s\n",
		line,
		strerror(expectedStatus),
		expectedMessage);

	fprintf(stderr, "actual error at line %" B_PRIi32 " - %s : %s\n",
		listener->GetErrorLine(),
		strerror(listener->ErrorStatus()),
		listener->GetErrorMessage().String());

	if (listener->HasEventsAfterError()) {
		fprintf(stderr, "first event after error [%d]\n",
			listener->FirstEventTypeAfterError());
	}

	CPPUNIT_ASSERT(!listener->HasEventsAfterError());
	CPPUNIT_ASSERT_EQUAL(expectedStatus, listener->ErrorStatus());
	CPPUNIT_ASSERT_EQUAL(line, listener->GetErrorLine());
	CPPUNIT_ASSERT(0 == strcmp(expectedMessage,
		listener->GetErrorMessage().String()));
}


void
JsonErrorHandlingTest::TestCompletelyUnknown()
{
	TestParseWithUnexpectedCharacter(
		JSON_SAMPLE_BROKEN_COMPLETELY_UNKNOWN, 1, B_BAD_DATA, 'z');
}


void
JsonErrorHandlingTest::TestObjectWithPrematureSeparator()
{
	TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_OBJECT_PREMATURE_SEPARATOR, 1,
		B_BAD_DATA, "unexpected item separator when parsing start of object");
}


void
JsonErrorHandlingTest::TestStringUnterminated()
{
	TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_UNTERMINATED_STRING, 1,
		B_BAD_DATA, "unexpected end of input");
}


void
JsonErrorHandlingTest::TestBadStringEscape()
{
	TestParseWithBadStringEscape(
		JSON_SAMPLE_BROKEN_BAD_STRING_ESCAPE, 1, B_BAD_DATA, 'v');
}


void
JsonErrorHandlingTest::TestBadNumber()
{
	TestParseWithErrorMessage(JSON_SAMPLE_BROKEN_NUMBER, 1, B_BAD_DATA,
		"malformed number");
}


void
JsonErrorHandlingTest::TestIOIssue()
{
	BDataIO *inputData = new FailingDataIO();
	ObjectDeleter<BDataIO> inputDataDeleter(inputData);
	TestParseWithErrorMessage(inputData, -1, B_IO_ERROR,
		"io related read error");
}


/*static*/ void
JsonErrorHandlingTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"JsonErrorHandlingTest");

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestCompletelyUnknown",
		&JsonErrorHandlingTest::TestCompletelyUnknown));

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestObjectWithPrematureSeparator",
		&JsonErrorHandlingTest::TestObjectWithPrematureSeparator));

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestStringUnterminated",
		&JsonErrorHandlingTest::TestStringUnterminated));

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestBadStringEscape",
		&JsonErrorHandlingTest::TestBadStringEscape));

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestBadNumber",
		&JsonErrorHandlingTest::TestBadNumber));

	suite.addTest(new CppUnit::TestCaller<JsonErrorHandlingTest>(
		"JsonErrorHandlingTest::TestIOIssue",
		&JsonErrorHandlingTest::TestIOIssue));

	parent.addTest("JsonErrorHandlingTest", &suite);
}