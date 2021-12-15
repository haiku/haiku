/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpProtocolTest.h"

#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <HttpHeaders.h>

using BPrivate::Network::BHttpHeader;
using BPrivate::Network::BHttpSession;


HttpProtocolTest::HttpProtocolTest()
{

}


void
HttpProtocolTest::HttpHeaderTest()
{
	using namespace std::literals;

	// Header field name validation (ignore value validation)
	{
		try {
			auto validFieldName = "Content-Encoding"sv;
			auto header = BHttpHeader{validFieldName, ""sv};
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when passing valid field name");
		}
		try {
			auto invalidFieldName = "Cóntênt_Éncõdìng";
			auto header = BHttpHeader{invalidFieldName, ""sv};
			CPPUNIT_FAIL("Creating a header with an invalid name did not raise an exception");
		} catch (const BHttpHeader::InvalidInput& e) {
			// success
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid name");
		}
	}
	// Header field value validation (ignore name validation)
	{
		try {
			auto validFieldValue = "VálìdF|êldValue"sv;
			auto header = BHttpHeader{""sv, validFieldValue};
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when passing valid field value");
		}
		try {
			auto invalidFieldValue = "Invalid\tField\0Value";
			auto header = BHttpHeader{""sv, invalidFieldValue};
			CPPUNIT_FAIL("Creating a header with an invalid value did not raise an exception");
		} catch (const BHttpHeader::InvalidInput& e) {
			// success
		} catch (...) {
			CPPUNIT_FAIL("Unexpected exception when creating a header with an invalid value");
		}
	}

	// Header field name case insensitive comparison
	{
		BHttpHeader header = BHttpHeader{"content-type"sv, ""sv};
		CPPUNIT_ASSERT(header.Name() == "content-type"sv);
		CPPUNIT_ASSERT(header.Name() == "Content-Type"sv);
		CPPUNIT_ASSERT(header.Name() == "cOnTeNt-TyPe"sv);
		CPPUNIT_ASSERT(header.Name() != "content_type"sv);
		CPPUNIT_ASSERT(header.Name() == BString{"Content-Type"});
	}
}


/* static */ void
HttpProtocolTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpProtocolTest");

	suite.addTest(new CppUnit::TestCaller<HttpProtocolTest>(
		"HttpProtocolTest::HttpHeaderTest", &HttpProtocolTest::HttpHeaderTest));

		// leak for now
	parent.addTest("HttpProtocolTest", &suite);
}
