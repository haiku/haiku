/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpProtocolTest.h"

#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <tools/cppunit/ThreadedTestCaller.h>

#include <DateTime.h>
#include <ExclusiveBorrow.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpTime.h>
#include <Looper.h>
#include <NetServicesDefs.h>
#include <Url.h>

using BPrivate::BDateTime;
using BPrivate::Network::BBorrow;
using BPrivate::Network::BExclusiveBorrow;
using BPrivate::Network::BHttpFields;
using BPrivate::Network::BHttpMethod;
using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpTime;
using BPrivate::Network::BHttpTimeFormat;
using BPrivate::Network::BNetworkRequestError;
using BPrivate::Network::format_http_time;
using BPrivate::Network::make_exclusive_borrow;
using BPrivate::Network::parse_http_time;

using namespace std::literals;

// Logger settings
constexpr bool LOG_ENABLED = true;
constexpr bool LOG_TO_CONSOLE = false;


HttpProtocolTest::HttpProtocolTest()
{
}


void
HttpProtocolTest::HttpFieldsTest()
{
	// Header field name validation (ignore value validation)
	{
		auto fields = BHttpFields();
		try {
			auto validFieldName = "Content-Encoding"sv;
			fields.AddField(validFieldName, "value"sv);
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when passing valid field name");
		}
		try {
			auto invalidFieldName = "Cóntênt_Éncõdìng";
			fields.AddField(invalidFieldName, "value"sv);
			CPPUNIT_FAIL("Creating a header with an invalid name did not raise an exception");
		} catch (const BHttpFields::InvalidInput& e) {
			// success
		}
	}
	// Header field value validation (ignore name validation)
	{
		auto fields = BHttpFields();
		try {
			auto validFieldValue = "VálìdF|êldValue"sv;
			fields.AddField("Field"sv, validFieldValue);
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when passing valid field value");
		}
		try {
			auto invalidFieldValue = "Invalid\tField\0Value";
			fields.AddField("Field"sv, invalidFieldValue);
			CPPUNIT_FAIL("Creating a header with an invalid value did not raise an exception");
		} catch (const BHttpFields::InvalidInput& e) {
			// success
		}
	}

	// Header line parsing validation
	{
		auto fields = BHttpFields();
		try {
			BString noWhiteSpace("Connection:close");
			fields.AddField(noWhiteSpace);
			BString extraWhiteSpace("Connection:     close\t\t  \t");
			fields.AddField(extraWhiteSpace);
			for (const auto& field: fields) {
				std::string_view name = field.Name();
				CPPUNIT_ASSERT_EQUAL("Connection"sv, name);
				CPPUNIT_ASSERT_EQUAL("close"sv, field.Value());
			}
		} catch (const BHttpFields::InvalidInput& e) {
			CPPUNIT_FAIL(e.input.String());
			CPPUNIT_FAIL("Unexpected exception when adding a header with an valid value");
		}

		try {
			BString noSeparator("Connection close");
			fields.AddField(noSeparator);
		} catch (const BHttpFields::InvalidInput& e) {
			// success
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid value");
		}

		try {
			BString noName = (":close");
			fields.AddField(noName);
		} catch (const BHttpFields::InvalidInput& e) {
			// success
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid value");
		}

		try {
			BString noValue = ("Connection     :");
			fields.AddField(noValue);
		} catch (const BHttpFields::InvalidInput& e) {
			// success
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid value");
		}
	}

	// Header field name case insensitive comparison
	{
		BHttpFields fields = BHttpFields();
		fields.AddField("content-type"sv, "value"sv);
		CPPUNIT_ASSERT(fields[0].Name() == "content-type"sv);
		CPPUNIT_ASSERT(fields[0].Name() == "Content-Type"sv);
		CPPUNIT_ASSERT(fields[0].Name() == "cOnTeNt-TyPe"sv);
		CPPUNIT_ASSERT(fields[0].Name() != "content_type"sv);
		CPPUNIT_ASSERT(fields[0].Name() == BString{"Content-Type"});
	}

	// Set up a generic set of headers for further use
	const BHttpFields defaultFields = {{"Host"sv, "haiku-os.org"sv}, {"Accept"sv, "*/*"sv},
		{"Set-Cookie"sv, "qwerty=494793ddkl; Domain=haiku-os.co.uk"sv},
		{"Set-Cookie"sv, "afbzyi=0kdnke0lyv; Domain=haiku-os.co.uk"sv},
		{}, // Empty; should be ignored by the constructor
		{"Accept-Encoding"sv, "gzip"sv}};

	// Validate std::initializer_list constructor
	CPPUNIT_ASSERT_EQUAL(5, defaultFields.CountFields());

	// Test copying and moving
	{
		BHttpFields copiedFields = defaultFields;
		CPPUNIT_ASSERT_EQUAL(copiedFields.CountFields(), defaultFields.CountFields());
		for (size_t i = 0; i < defaultFields.CountFields(); i++) {
			std::string_view copiedName = copiedFields[i].Name();
			CPPUNIT_ASSERT(defaultFields[i].Name() == copiedName);
			CPPUNIT_ASSERT_EQUAL(defaultFields[i].Value(), copiedFields[i].Value());
		}

		BHttpFields movedFields(std::move(copiedFields));
		CPPUNIT_ASSERT_EQUAL(movedFields.CountFields(), defaultFields.CountFields());
		for (size_t i = 0; i < movedFields.CountFields(); i++) {
			std::string_view defaultName = defaultFields[i].Name();
			CPPUNIT_ASSERT(movedFields[i].Name() == defaultName);
			CPPUNIT_ASSERT_EQUAL(movedFields[i].Value(), defaultFields[i].Value());
		}

		CPPUNIT_ASSERT_EQUAL(copiedFields.CountFields(), 0);
	}

	// Test query and modification tools
	{
		BHttpFields fields = defaultFields;
		// test order of adding fields (in order of construction)
		fields.AddField("Set-Cookie"sv, "vfxdrm=9lpqrsvxm; Domain=haiku-os.co.uk"sv);
		// query for Set-Cookie should find the first in the list
		auto it = fields.FindField("Set-Cookie"sv);
		CPPUNIT_ASSERT(it != fields.end());
		CPPUNIT_ASSERT((*it).Name() == "Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL(defaultFields[2].Value(), (*it).Value());

		// the last item should be the newly insterted one
		it = fields.end();
		it--;
		CPPUNIT_ASSERT(it != fields.begin());
		CPPUNIT_ASSERT((*it).Name() == "Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL("vfxdrm=9lpqrsvxm; Domain=haiku-os.co.uk"sv, (*it).Value());

		// the item before should be the Accept-Encoding one
		it--;
		CPPUNIT_ASSERT(it != fields.begin());
		CPPUNIT_ASSERT((*it).Name() == "Accept-Encoding"sv);

		// remove the Accept-Encoding entry by iterator
		fields.RemoveField(it);
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), defaultFields.CountFields());
		// remove the Set-Cookie entries by name
		fields.RemoveField("Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), 2);
		// test MakeEmpty
		fields.MakeEmpty();
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), 0);
	}

	// Iterate through the fields using a constant iterator
	{
		const BHttpFields fields = {{"key1"sv, "value1"sv}, {"key2"sv, "value2"sv},
			{"key3"sv, "value3"sv}, {"key4"sv, "value4"sv}};

		auto count = 0L;
		for (const auto& field: fields) {
			count++;
			auto key = BString("key");
			auto value = BString("value");
			key << count;
			value << count;
			CPPUNIT_ASSERT_EQUAL(std::string_view(key.String()), field.Name());
			CPPUNIT_ASSERT_EQUAL(value, BString(field.Value().data(), field.Value().length()));
		}
		CPPUNIT_ASSERT_EQUAL(count, 4);
	}
}


void
HttpProtocolTest::HttpMethodTest()
{
	using namespace std::literals;

	// Default methods
	{
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Get).Method(), "GET"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Head).Method(), "HEAD"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Post).Method(), "POST"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Put).Method(), "PUT"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Delete).Method(), "DELETE"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Connect).Method(), "CONNECT"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Options).Method(), "OPTIONS"sv);
		CPPUNIT_ASSERT_EQUAL(BHttpMethod(BHttpMethod::Trace).Method(), "TRACE"sv);
	}

	// Valid custom method
	{
		try {
			auto method = BHttpMethod("PATCH"sv);
			CPPUNIT_ASSERT_EQUAL(method.Method(), "PATCH"sv);
		} catch (...) {
			CPPUNIT_FAIL("Unexpected error when creating valid method");
		}
	}

	// Invalid empty method
	try {
		auto method = BHttpMethod("");
		CPPUNIT_FAIL("Creating an empty method was succesful unexpectedly");
	} catch (BHttpMethod::InvalidMethod&) {
		// success
	}

	// Method with invalid characters (arabic translation of GET)
	try {
		auto method = BHttpMethod("جلب");
		CPPUNIT_FAIL("Creating a method with invalid characters was succesful unexpectedly");
	} catch (BHttpMethod::InvalidMethod&) {
		// success
	}
}


constexpr std::string_view kExpectedRequestText = "GET / HTTP/1.1\r\n"
												  "Host: www.haiku-os.org\r\n"
												  "Accept-Encoding: gzip\r\n"
												  "Connection: close\r\n"
												  "Api-Key: 01234567890abcdef\r\n\r\n";


void
HttpProtocolTest::HttpRequestTest()
{
	// Basic test
	BHttpRequest request;
	CPPUNIT_ASSERT(request.IsEmpty());
	auto url = BUrl("https://www.haiku-os.org");
	request.SetUrl(url);
	CPPUNIT_ASSERT(request.Url() == url);

	// Add Invalid HTTP fields (should throw)
	try {
		BHttpFields invalidField = {{"Host"sv, "haiku-os.org"sv}};
		request.SetFields(invalidField);
		CPPUNIT_FAIL("Should not be able to add the invalid \"Host\" field to a request");
	} catch (BHttpFields::InvalidInput& e) {
		// Correct; do nothing
	}

	// Add valid HTTP field
	BHttpFields validField = {{"Api-Key"sv, "01234567890abcdef"}};
	request.SetFields(validField);

	// Validate header serialization
	BString header = request.HeaderToString();
	CPPUNIT_ASSERT(header.Compare(kExpectedRequestText.data(), kExpectedRequestText.size()) == 0);
}


void
HttpProtocolTest::HttpTimeTest()
{
	const std::vector<BString> kValidTimeStrings
		= {"Sun, 07 Dec 2003 16:01:00 GMT", "Sun, 07 Dec 2003 16:01:00",
			"Sunday, 07-Dec-03 16:01:00 GMT", "Sunday, 07-Dec-03 16:01:00 GMT",
			"Sunday, 07-Dec-2003 16:01:00", "Sunday, 07-Dec-2003 16:01:00 GMT",
			"Sunday, 07-Dec-2003 16:01:00 UTC", "Sun Dec  7 16:01:00 2003"};
	const BDateTime kExpectedDateTime = {BDate{2003, 12, 7}, BTime{16, 01, 0}};

	for (const auto& timeString: kValidTimeStrings) {
		CPPUNIT_ASSERT(kExpectedDateTime == parse_http_time(timeString));
	}

	const std::vector<BString> kInvalidTimeStrings = {
		"Sun, 07 Dec 2003", // Date only
		"Sun, 07 Dec 2003 16:01:00 BST", // Invalid timezone
		"On Sun, 07 Dec 2003 16:01:00 GMT", // Extra data in front of the string
	};

	for (const auto& timeString: kInvalidTimeStrings) {
		try {
			parse_http_time(timeString);
			BString errorMessage = "Expected exception with invalid timestring: ";
			errorMessage.Append(timeString);
			CPPUNIT_FAIL(errorMessage.String());
		} catch (const BHttpTime::InvalidInput& e) {
			// expected exception; continue
		}
	}

	// Validate format_http_time()
	CPPUNIT_ASSERT_EQUAL(
		BString("Sun, 07 Dec 2003 16:01:00 GMT"), format_http_time(kExpectedDateTime));
	CPPUNIT_ASSERT_EQUAL(BString("Sunday, 07-Dec-03 16:01:00 GMT"),
		format_http_time(kExpectedDateTime, BHttpTimeFormat::RFC850));
	CPPUNIT_ASSERT_EQUAL(BString("Sun Dec  7 16:01:00 2003"),
		format_http_time(kExpectedDateTime, BHttpTimeFormat::AscTime));
}


/* static */ void
HttpProtocolTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpProtocolTest");

	suite.addTest(new CppUnit::TestCaller<HttpProtocolTest>(
		"HttpProtocolTest::HttpFieldsTest", &HttpProtocolTest::HttpFieldsTest));
	suite.addTest(new CppUnit::TestCaller<HttpProtocolTest>(
		"HttpProtocolTest::HttpMethodTest", &HttpProtocolTest::HttpMethodTest));
	suite.addTest(new CppUnit::TestCaller<HttpProtocolTest>(
		"HttpProtocolTest::HttpRequestTest", &HttpProtocolTest::HttpRequestTest));
	suite.addTest(new CppUnit::TestCaller<HttpProtocolTest>(
		"HttpProtocolTest::HttpTimeTest", &HttpProtocolTest::HttpTimeTest));

	parent.addTest("HttpProtocolTest", &suite);
}


// Observer test

#include <iostream>
class ObserverHelper : public BLooper
{
public:
	ObserverHelper()
		:
		BLooper("ObserverHelper")
	{
	}

	void MessageReceived(BMessage* msg) override { messages.emplace_back(*msg); }

	std::vector<BMessage> messages;
};


// HttpIntegrationTest


HttpIntegrationTest::HttpIntegrationTest(TestServerMode mode)
	:
	fTestServer(mode)
{
	// increase number of concurrent connections to 4 (from 2)
	fSession.SetMaxConnectionsPerHost(4);

	if constexpr (LOG_ENABLED) {
		fLogger = new HttpDebugLogger();
		fLogger->SetConsoleLogging(LOG_TO_CONSOLE);
		if (mode == TestServerMode::Http)
			fLogger->SetFileLogging("http-messages.log");
		else
			fLogger->SetFileLogging("https-messages.log");
		fLogger->Run();
		fLoggerMessenger.SetTo(fLogger);
	}
}


void
HttpIntegrationTest::setUp()
{
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Starting up test server", B_OK, fTestServer.Start());
}


void
HttpIntegrationTest::tearDown()
{
	if (fLogger) {
		fLogger->Lock();
		fLogger->Quit();
	}
}


/* static */ void
HttpIntegrationTest::AddTests(BTestSuite& parent)
{
	// Http
	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpIntegrationTest");

		HttpIntegrationTest* httpIntegrationTest = new HttpIntegrationTest(TestServerMode::Http);
		BThreadedTestCaller<HttpIntegrationTest>* testCaller
			= new BThreadedTestCaller<HttpIntegrationTest>("HttpTest::", httpIntegrationTest);

		// HTTP
		testCaller->addThread(
			"HostAndNetworkFailTest", &HttpIntegrationTest::HostAndNetworkFailTest);
		testCaller->addThread("GetTest", &HttpIntegrationTest::GetTest);
		testCaller->addThread("GetWithBufferTest", &HttpIntegrationTest::GetWithBufferTest);
		testCaller->addThread("HeadTest", &HttpIntegrationTest::HeadTest);
		testCaller->addThread("NoContentTest", &HttpIntegrationTest::NoContentTest);
		testCaller->addThread("AutoRedirectTest", &HttpIntegrationTest::AutoRedirectTest);
		testCaller->addThread("BasicAuthTest", &HttpIntegrationTest::BasicAuthTest);
		testCaller->addThread("StopOnErrorTest", &HttpIntegrationTest::StopOnErrorTest);
		testCaller->addThread("RequestCancelTest", &HttpIntegrationTest::RequestCancelTest);
		testCaller->addThread("PostTest", &HttpIntegrationTest::PostTest);

		suite.addTest(testCaller);
		parent.addTest("HttpIntegrationTest", &suite);
	}

	// Https
	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpsIntegrationTest");

		HttpIntegrationTest* httpsIntegrationTest = new HttpIntegrationTest(TestServerMode::Https);
		BThreadedTestCaller<HttpIntegrationTest>* testCaller
			= new BThreadedTestCaller<HttpIntegrationTest>("HttpsTest::", httpsIntegrationTest);

		// HTTPS
		testCaller->addThread(
			"HostAndNetworkFailTest", &HttpIntegrationTest::HostAndNetworkFailTest);
		testCaller->addThread("GetTest", &HttpIntegrationTest::GetTest);
		testCaller->addThread("GetWithBufferTest", &HttpIntegrationTest::GetWithBufferTest);
		testCaller->addThread("HeadTest", &HttpIntegrationTest::HeadTest);
		testCaller->addThread("NoContentTest", &HttpIntegrationTest::NoContentTest);
		testCaller->addThread("AutoRedirectTest", &HttpIntegrationTest::AutoRedirectTest);
		// testCaller->addThread("BasicAuthTest", &HttpIntegrationTest::BasicAuthTest);
		// Skip BasicAuthTest for HTTPS: it seems like it does not close the socket properly,
		// raising a SSL EOF error.
		testCaller->addThread("StopOnErrorTest", &HttpIntegrationTest::StopOnErrorTest);
		testCaller->addThread("RequestCancelTest", &HttpIntegrationTest::RequestCancelTest);
		testCaller->addThread("PostTest", &HttpIntegrationTest::PostTest);

		suite.addTest(testCaller);
		parent.addTest("HttpsIntegrationTest", &suite);
	}
}


void
HttpIntegrationTest::HostAndNetworkFailTest()
{
	// Test hostname resolution fail
	{
		auto request = BHttpRequest(BUrl("http://doesnotexist/"));
		auto result = fSession.Execute(std::move(request));
		try {
			result.Status();
			CPPUNIT_FAIL("Expecting exception when trying to connect to invalid hostname");
		} catch (const BNetworkRequestError& e) {
			CPPUNIT_ASSERT_EQUAL(BNetworkRequestError::HostnameError, e.Type());
		}
	}

	// Test connection error fail
	{
		// FIXME: find a better way to get an unused local port, instead of hardcoding one
		auto request = BHttpRequest(BUrl("http://localhost:59445/"));
		auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
		try {
			result.Status();
			CPPUNIT_FAIL("Expecting exception when trying to connect to invalid hostname");
		} catch (const BNetworkRequestError& e) {
			CPPUNIT_ASSERT_EQUAL(BNetworkRequestError::NetworkError, e.Type());
		}
	}
}


static const BHttpFields kExpectedGetFields = {
	{"Server"sv, "Test HTTP Server for Haiku"sv},
	{"Date"sv, "Sun, 09 Feb 2020 19:32:42 GMT"sv},
	{"Content-Type"sv, "text/plain"sv},
	{"Content-Length"sv, "107"sv},
	{"Content-Encoding"sv, "gzip"sv},
};


constexpr std::string_view kExpectedGetBody = {"Path: /\r\n"
											   "\r\n"
											   "Headers:\r\n"
											   "--------\r\n"
											   "Host: 127.0.0.1:PORT\r\n"
											   "Accept-Encoding: gzip\r\n"
											   "Connection: close\r\n"};


void
HttpIntegrationTest::GetTest()
{
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/"));
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	try {
		auto receivedFields = result.Fields();

		CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of headers",
			kExpectedGetFields.CountFields(), receivedFields.CountFields());
		for (auto& field: receivedFields) {
			auto expectedField = kExpectedGetFields.FindField(field.Name());
			if (expectedField == kExpectedGetFields.end())
				CPPUNIT_FAIL("Could not find expected field in response headers");

			CPPUNIT_ASSERT_EQUAL(field.Value(), (*expectedField).Value());
		}
		auto receivedBody = result.Body().text;
		CPPUNIT_ASSERT(receivedBody.has_value());
		CPPUNIT_ASSERT_EQUAL(kExpectedGetBody, receivedBody.value().String());
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


void
HttpIntegrationTest::GetWithBufferTest()
{
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/"));
	auto body = make_exclusive_borrow<BMallocIO>();
	auto result = fSession.Execute(std::move(request), BBorrow<BDataIO>(body), fLoggerMessenger);
	try {
		result.Body();
		auto bodyString
			= std::string(reinterpret_cast<const char*>(body->Buffer()), body->BufferLength());
		CPPUNIT_ASSERT_EQUAL(kExpectedGetBody, bodyString);
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


void
HttpIntegrationTest::HeadTest()
{
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/"));
	request.SetMethod(BHttpMethod::Head);
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	try {
		auto receivedFields = result.Fields();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of headers",
			kExpectedGetFields.CountFields(), receivedFields.CountFields());
		for (auto& field: receivedFields) {
			auto expectedField = kExpectedGetFields.FindField(field.Name());
			if (expectedField == kExpectedGetFields.end())
				CPPUNIT_FAIL("Could not find expected field in response headers");

			CPPUNIT_ASSERT_EQUAL(field.Value(), (*expectedField).Value());
		}

		CPPUNIT_ASSERT(result.Body().text->Length() == 0);
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


static const BHttpFields kExpectedNoContentFields = {
	{"Server"sv, "Test HTTP Server for Haiku"sv},
	{"Date"sv, "Sun, 09 Feb 2020 19:32:42 GMT"sv},
};


void
HttpIntegrationTest::NoContentTest()
{
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/204"));
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	try {
		auto receivedStatus = result.Status();
		CPPUNIT_ASSERT_EQUAL(204, receivedStatus.code);

		auto receivedFields = result.Fields();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of headers",
			kExpectedNoContentFields.CountFields(), receivedFields.CountFields());
		for (auto& field: receivedFields) {
			auto expectedField = kExpectedNoContentFields.FindField(field.Name());
			if (expectedField == kExpectedNoContentFields.end())
				CPPUNIT_FAIL("Could not find expected field in response headers");

			CPPUNIT_ASSERT_EQUAL(field.Value(), (*expectedField).Value());
		}

		CPPUNIT_ASSERT(result.Body().text->Length() == 0);
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


void
HttpIntegrationTest::AutoRedirectTest()
{
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/302"));
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	try {
		auto receivedFields = result.Fields();

		CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of headers",
			kExpectedGetFields.CountFields(), receivedFields.CountFields());
		for (auto& field: receivedFields) {
			auto expectedField = kExpectedGetFields.FindField(field.Name());
			if (expectedField == kExpectedGetFields.end())
				CPPUNIT_FAIL("Could not find expected field in response headers");

			CPPUNIT_ASSERT_EQUAL(field.Value(), (*expectedField).Value());
		}
		auto receivedBody = result.Body().text;
		CPPUNIT_ASSERT(receivedBody.has_value());
		CPPUNIT_ASSERT_EQUAL(kExpectedGetBody, receivedBody.value().String());
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


void
HttpIntegrationTest::BasicAuthTest()
{
	// Basic Authentication
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/auth/basic/walter/secret"));
	request.SetAuthentication({"walter", "secret"});
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	CPPUNIT_ASSERT(result.Status().code == 200);

	// Basic Authentication with incorrect credentials
	try {
		request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/auth/basic/walter/secret"));
		request.SetAuthentication({"invaliduser", "invalidpassword"});
		result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
		CPPUNIT_ASSERT(result.Status().code == 401);
	} catch (const BPrivate::Network::BError& e) {
		CPPUNIT_FAIL(e.DebugMessage().String());
	}
}


void
HttpIntegrationTest::StopOnErrorTest()
{
	// Test the Stop on Error functionality
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/400"));
	request.SetStopOnError(true);
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	CPPUNIT_ASSERT(result.Status().code == 400);
	CPPUNIT_ASSERT(result.Fields().CountFields() == 0);
	CPPUNIT_ASSERT(result.Body().text->Length() == 0);
}


void
HttpIntegrationTest::RequestCancelTest()
{
	// Test the cancellation functionality
	// TODO: this test potentially fails if the case is executed before the cancellation is
	//       processed. In practise, the cancellation always comes first. When the server
	//       supports a wait parameter, then this test can be made more robust.
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/"));
	auto result = fSession.Execute(std::move(request), nullptr, fLoggerMessenger);
	fSession.Cancel(result);
	try {
		result.Body();
		CPPUNIT_FAIL("Expected exception because request was cancelled");
	} catch (const BNetworkRequestError& e) {
		CPPUNIT_ASSERT(e.Type() == BNetworkRequestError::Canceled);
	}
}


static const BString kPostText
	= "The MIT License\n"
	  "\n"
	  "Copyright (c) <year> <copyright holders>\n"
	  "\n"
	  "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
	  "of this software and associated documentation files (the \"Software\"), to deal\n"
	  "in the Software without restriction, including without limitation the rights\n"
	  "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
	  "copies of the Software, and to permit persons to whom the Software is\n"
	  "furnished to do so, subject to the following conditions:\n"
	  "\n"
	  "The above copyright notice and this permission notice shall be included in\n"
	  "all copies or substantial portions of the Software.\n"
	  "\n"
	  "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
	  "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
	  "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
	  "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
	  "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
	  "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
	  "THE SOFTWARE.\n"
	  "\n";


static BString kExpectedPostBody = BString().SetToFormat("Path: /post\r\n"
														 "\r\n"
														 "Headers:\r\n"
														 "--------\r\n"
														 "Host: 127.0.0.1:PORT\r\n"
														 "Accept-Encoding: gzip\r\n"
														 "Connection: close\r\n"
														 "Content-Type: text/plain\r\n"
														 "Content-Length: 1083\r\n"
														 "\r\n"
														 "Request body:\r\n"
														 "-------------\r\n"
														 "%s\r\n",
	kPostText.String());


void
HttpIntegrationTest::PostTest()
{
	using namespace BPrivate::Network::UrlEvent;
	using namespace BPrivate::Network::UrlEventData;

	auto postBody = std::make_unique<BMallocIO>();
	postBody->Write(kPostText.String(), kPostText.Length());
	postBody->Seek(0, SEEK_SET);
	auto request = BHttpRequest(BUrl(fTestServer.BaseUrl(), "/post"));
	request.SetMethod(BHttpMethod::Post);
	request.SetRequestBody(std::move(postBody), "text/plain", kPostText.Length());

	auto observer = new ObserverHelper();
	observer->Run();

	auto result = fSession.Execute(std::move(request), nullptr, BMessenger(observer));

	CPPUNIT_ASSERT(result.Body().text.has_value());
	CPPUNIT_ASSERT_EQUAL(kExpectedPostBody.Length(), result.Body().text.value().Length());
	CPPUNIT_ASSERT(result.Body().text.value() == kExpectedPostBody);

	usleep(2000); // give some time to catch up on receiving all messages

	observer->Lock();
	while (observer->IsMessageWaiting()) {
		observer->Unlock();
		usleep(1000); // give some time to catch up on receiving all messages
		observer->Lock();
	}

	// Assert that the messages have the right contents.
	CPPUNIT_ASSERT_MESSAGE(
		"Expected at least 8 observer messages for this request.", observer->messages.size() >= 8);

	uint32 previousMessage = 0;
	for (const auto& message: observer->messages) {
		auto id = observer->messages[0].GetInt32(BPrivate::Network::UrlEventData::Id, -1);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("message Id does not match", result.Identity(), id);

		if (message.what == BPrivate::Network::UrlEvent::DebugMessage) {
			// ignore debug messages
			continue;
		}

		switch (previousMessage) {
			case 0:
				CPPUNIT_ASSERT_MESSAGE(
					"message should be HostNameResolved", HostNameResolved == message.what);
				break;

			case HostNameResolved:
				CPPUNIT_ASSERT_MESSAGE(
					"message should be ConnectionOpened", ConnectionOpened == message.what);
				break;

			case ConnectionOpened:
				CPPUNIT_ASSERT_MESSAGE(
					"message should be UploadProgress", UploadProgress == message.what);
				[[fallthrough]];

			case UploadProgress:
				switch (message.what) {
					case UploadProgress:
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::NumBytes data",
							message.HasInt64(NumBytes));
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::TotalBytes data",
							message.HasInt64(TotalBytes));
						CPPUNIT_ASSERT_MESSAGE("UrlEventData::TotalBytes size does not match",
							kPostText.Length() == message.GetInt64(TotalBytes, 0));
						break;
					case ResponseStarted:
						break;
					default:
						CPPUNIT_FAIL("Expected UploadProgress or ResponseStarted message");
				}
				break;

			case ResponseStarted:
				CPPUNIT_ASSERT_MESSAGE("message should be HttpStatus", HttpStatus == message.what);
				CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::HttpStatusCode data",
					message.HasInt16(HttpStatusCode));
				break;

			case HttpStatus:
				CPPUNIT_ASSERT_MESSAGE("message should be HttpFields", HttpFields == message.what);
				break;

			case HttpFields:
				CPPUNIT_ASSERT_MESSAGE(
					"message should be DownloadProgress", DownloadProgress == message.what);
				[[fallthrough]];

			case DownloadProgress:
			case BytesWritten:
				switch (message.what) {
					case DownloadProgress:
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::NumBytes data",
							message.HasInt64(NumBytes));
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::TotalBytes data",
							message.HasInt64(TotalBytes));
						break;
					case BytesWritten:
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::NumBytes data",
							message.HasInt64(NumBytes));
						break;
					case RequestCompleted:
						CPPUNIT_ASSERT_MESSAGE("message must have UrlEventData::Success data",
							message.HasBool(Success));
						CPPUNIT_ASSERT_MESSAGE(
							"UrlEventData::Success must be true", message.GetBool(Success));
						break;
					default:
						CPPUNIT_FAIL("Expected DownloadProgress, BytesWritten or HttpStatus "
									 "message");
				}
				break;

			default:
				CPPUNIT_FAIL("Unexpected message");
		}
		previousMessage = message.what;
	}

	observer->Quit();
}
