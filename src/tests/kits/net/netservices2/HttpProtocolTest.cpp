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

#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpStream.h>
#include <NetServicesDefs.h>
#include <Url.h>

using BPrivate::Network::BHttpFields;
using BPrivate::Network::BHttpMethod;
using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpRequestStream;
using BPrivate::Network::BNetworkRequestError;


HttpProtocolTest::HttpProtocolTest()
{

}


void
HttpProtocolTest::HttpFieldsTest()
{
	using namespace std::literals;

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
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid name");
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
	const BHttpFields defaultFields = {
		{"Host"sv, "haiku-os.org"sv},
		{"Accept"sv, "*/*"sv},
		{"Set-Cookie"sv, "qwerty=494793ddkl; Domain=haiku-os.co.uk"sv},
		{"Set-Cookie"sv, "afbzyi=0kdnke0lyv; Domain=haiku-os.co.uk"sv},
		{},	// Empty; should be ignored by the constructor
		{"Accept-Encoding"sv, "gzip"sv}
	};

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
		// test order of adding fields
		fields.AddField("Set-Cookie"sv, "vfxdrm=9lpqrsvxm; Domain=haiku-os.co.uk"sv);
		// query for Set-Cookie
		auto it = fields.FindField("Set-Cookie"sv);
		CPPUNIT_ASSERT(it != fields.end());
		CPPUNIT_ASSERT((*it).Name() == "Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL(defaultFields[2].Value(), (*it).Value());
		it++;
		CPPUNIT_ASSERT(it != fields.end());
		CPPUNIT_ASSERT((*it).Name() == "Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL(defaultFields[3].Value(), (*it).Value());
		it++;
		CPPUNIT_ASSERT(it != fields.end());
		CPPUNIT_ASSERT((*it).Name() == "Set-Cookie"sv);
		it++;
		CPPUNIT_ASSERT(it != fields.end());
		CPPUNIT_ASSERT_EQUAL(defaultFields[4].Value(), (*it).Value());
		// Remove the Accept-Encoding entry by iterator
		fields.RemoveField(it);
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), defaultFields.CountFields());
		// Remove the Set-Cookie entries by name
		fields.RemoveField("Set-Cookie"sv);
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), 2);
		// Test MakeEmpty
		fields.MakeEmpty();
		CPPUNIT_ASSERT_EQUAL(fields.CountFields(), 0);
	}

	// Iterate through the fields using a constant iterator
	{
		const BHttpFields fields = {
			{"key1"sv, "value1"sv},
			{"key2"sv, "value2"sv},
			{"key3"sv, "value3"sv},
			{"key4"sv, "value4"sv}
		};

		auto count = 0L;
		for (const auto& field: fields) {
			count++;
			auto key = BString("key");
			auto value = BString("value");
			key << count;
			value << count;
			CPPUNIT_ASSERT_EQUAL(key, field.Name());
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
	} catch (...) {
		CPPUNIT_FAIL("Unexpected exception type when creating an empty method");
	}

	// Method with invalid characters (arabic translation of GET)
	try {
		auto method = BHttpMethod("جلب");
		CPPUNIT_FAIL("Creating a method with invalid characters was succesful unexpectedly");
	} catch (BHttpMethod::InvalidMethod&) {
		// success
	} catch (...) {
		CPPUNIT_FAIL("Unexpected exception type when creating a method with invalid characters");
	}
}


constexpr std::string_view kHaikuGetRequestText =
	"GET / HTTP/1.1\r\n"
	"Host: www.haiku-os.org\r\n"
	"Accept: *\r\n"
	"Accept-Encoding: gzip\r\n"
	"Connection: close\r\n\r\n";


void
HttpProtocolTest::HttpRequestTest()
{
	// Basic test
	BHttpRequest request;
	CPPUNIT_ASSERT(request.IsEmpty());
	auto url = BUrl("https://www.haiku-os.org");
	request.SetUrl(url);
	CPPUNIT_ASSERT(request.Url() == url);

	// Validate header serialization
	BString header = request.HeaderToString();
	CPPUNIT_ASSERT(header.Compare(kHaikuGetRequestText.data(), kHaikuGetRequestText.size()) == 0);
}


class RequestStreamTestIO : public BDataIO
{
public:
	RequestStreamTestIO(const std::string_view expectedOutput)
		: fExpectedOutput(expectedOutput)
	{
		
	}

	// Accept maximum of 8 bytes at a time.
	ssize_t Write(const void* buffer, size_t size) {
		ssize_t bytesWritten = (size < 8) ? size : 8;
		CPPUNIT_ASSERT_MESSAGE("RequestStreamTestIO: bytes written larger than expected output",
			fExpectedOutput.size() >= (fPos + bytesWritten));
		CPPUNIT_ASSERT(fExpectedOutput.substr(fPos, bytesWritten) == std::string_view(static_cast<const char*>(buffer), bytesWritten));
		fPos += bytesWritten;
		return bytesWritten;
	};

private:
	const std::string_view fExpectedOutput;
	ssize_t fPos = 0;
};


void
HttpProtocolTest::HttpRequestStreamTest()
{
	// Set up basic GET for https://www.haiku-os.org/
	BHttpRequest request;
	auto url = BUrl("https://www.haiku-os.org");
	request.SetUrl(url);

	// Test streaming the request
	BHttpRequestStream requestStream(request);
	RequestStreamTestIO testIO(kHaikuGetRequestText.data());
	bool finished = false;
	ssize_t expectedBytesWritten = 8;
	ssize_t expectedTotalBytesWritten = 8;
	const ssize_t expectedTotalSize = kHaikuGetRequestText.size();
	if (expectedTotalSize < 8) {
		expectedBytesWritten = expectedTotalSize;
		expectedTotalBytesWritten = expectedTotalSize;
	}
	while (!finished) {
		auto [currentBytesWritten, totalBytesWritten, totalSize, complete] = requestStream.Transfer(&testIO);
		CPPUNIT_ASSERT_EQUAL(expectedBytesWritten, currentBytesWritten);
		CPPUNIT_ASSERT_EQUAL(expectedTotalBytesWritten, totalBytesWritten);
		CPPUNIT_ASSERT_EQUAL(expectedTotalSize, totalSize);
		if (expectedTotalBytesWritten == expectedTotalSize) {
			// loop should be finished
			CPPUNIT_ASSERT_EQUAL(true, complete);
			finished = true;
		} else {
			// prepare for next loop
			if (totalSize - totalBytesWritten < 8) {
				expectedBytesWritten = totalSize - totalBytesWritten;
			}
			expectedTotalBytesWritten += expectedBytesWritten;
			CPPUNIT_ASSERT_EQUAL(false, complete);
		}
	}
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
		"HttpProtocolTest::HttpRequestStreamTest", &HttpProtocolTest::HttpRequestStreamTest));

	parent.addTest("HttpProtocolTest", &suite);
}



// HttpIntegrationTest


HttpIntegrationTest::HttpIntegrationTest(TestServerMode mode)
	: fTestServer(mode)
{

}


void
HttpIntegrationTest::setUp()
{
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Starting up test server",
		B_OK,
		fTestServer.Start());
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
		testCaller->addThread("HostAndNetworkFailTest", &HttpIntegrationTest::HostAndNetworkFailTest);

		suite.addTest(testCaller);
		parent.addTest("HttpIntegrationTest", &suite);
	}

	// Https
	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpsIntegrationTest");

		HttpIntegrationTest* httpsIntegrationTest = new HttpIntegrationTest(TestServerMode::Https);
		BThreadedTestCaller<HttpIntegrationTest>* testCaller
			= new BThreadedTestCaller<HttpIntegrationTest>("HttpsTest::", httpsIntegrationTest);

		// HTTP
		testCaller->addThread("HostAndNetworkFailTest", &HttpIntegrationTest::HostAndNetworkFailTest);

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
		} catch (...) {
			CPPUNIT_FAIL("Unknown exception raised when getting invalid hostname");
		}
	}

	// Test connection error fail
	{
		// FIXME: find a better way to get an unused local port, instead of hardcoding one
		auto request = BHttpRequest(BUrl("http://localhost:59445/"));
		auto result = fSession.Execute(std::move(request));
		try {
			result.Status();
			CPPUNIT_FAIL("Expecting exception when trying to connect to invalid hostname");
		} catch (const BNetworkRequestError& e) {
			CPPUNIT_ASSERT_EQUAL(BNetworkRequestError::NetworkError, e.Type());
		} catch (...) {
			CPPUNIT_FAIL("Unknown exception raised when getting invalid hostname");
		}
	}

	// Succesful connection (fails as canceled right now)
	{
		auto request = BHttpRequest(BUrl("https://www.haiku-os.org/"));
		auto result = fSession.Execute(std::move(request));
		try {
			result.Status();
			CPPUNIT_FAIL("Expecting exception");
		} catch (const BNetworkRequestError& e) {
			CPPUNIT_ASSERT_EQUAL(BNetworkRequestError::Canceled, e.Type());
		} catch (...) {
			CPPUNIT_FAIL("Unknown exception raised when executing request");
		}
	}
}
