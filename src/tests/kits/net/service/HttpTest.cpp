/*
 * Copyright 2010, Christophe Huriaux
 * Copyright 2014, Haiku, inc.
 * Distributed under the terms of the MIT licence
 */


#include "HttpTest.h"


#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <NetworkKit.h>
#include <HttpRequest.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


HttpTest::HttpTest()
	: fBaseUrl("http://httpbin.org/")
{
}


HttpTest::~HttpTest()
{
}


void
HttpTest::GetTest()
{
	BUrl testUrl(fBaseUrl, "/user-agent");
	BUrlContext c;
	BHttpRequest t(testUrl);

	t.SetContext(&c);

	CPPUNIT_ASSERT(t.Run());

	while(t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT(t.Status() == B_OK);

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(6, r.Headers().CountHeaders());
		// FIXME This is too strict and not very useful.
	CPPUNIT_ASSERT_EQUAL(42, r.Length());
		// Fixed size as we know the response format.
	CPPUNIT_ASSERT(!c.GetCookieJar().GetIterator().HasNext());
		// This page should not set cookies
}


void
HttpTest::UploadTest()
{
	BUrl testUrl(fBaseUrl, "/post");
	BUrlContext c;
	BHttpRequest t(testUrl);

	t.SetContext(&c);

	BHttpForm f;
	f.AddString("hello", "world");
	CPPUNIT_ASSERT(f.AddFile("_uploadfile", BPath("/system/data/licenses/MIT"))
		== B_OK);

	t.SetPostFields(f);

	CPPUNIT_ASSERT(t.Run());

	while(t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT(t.Status() == B_OK);

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(460, r.Length());
		// Fixed size as we know the response format.
}


void
HttpTest::AuthBasicTest()
{
	BUrl testUrl(fBaseUrl, "/basic-auth/walter/secret");
	_AuthTest(testUrl);
}


void
HttpTest::AuthDigestTest()
{
	BUrl testUrl(fBaseUrl, "/digest-auth/auth/walter/secret");
	_AuthTest(testUrl);
}


void
HttpTest::_AuthTest(BUrl& testUrl)
{
	BUrlContext c;
	BHttpRequest t(testUrl);

	t.SetContext(&c);

	t.SetUserName("walter");
	t.SetPassword("secret");

	CPPUNIT_ASSERT(t.Run());

	while(t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT(t.Status() == B_OK);

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(6, r.Headers().CountHeaders());
		// FIXME This is too strict and not very useful.
	CPPUNIT_ASSERT_EQUAL(47, r.Length());
		// Fixed size as we know the response format.
}


/* static */ void
HttpTest::AddTests(BTestSuite& parent)
{
	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpTest");

		suite.addTest(new CppUnit::TestCaller<HttpTest>(
			"HttpTest::GetTest", &HttpTest::GetTest));
		suite.addTest(new CppUnit::TestCaller<HttpTest>(
			"HttpTest::UploadTest", &HttpTest::UploadTest));
		suite.addTest(new CppUnit::TestCaller<HttpTest>(
			"HttpTest::AuthBasicTest", &HttpTest::AuthBasicTest));
		suite.addTest(new CppUnit::TestCaller<HttpTest>(
			"HttpTest::AuthDigestTest", &HttpTest::AuthDigestTest));

		parent.addTest("HttpTest", &suite);
	}

	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpsTest");

		suite.addTest(new CppUnit::TestCaller<HttpsTest>(
			"HttpsTest::GetTest", &HttpsTest::GetTest));
		suite.addTest(new CppUnit::TestCaller<HttpsTest>(
			"HttpsTest::UploadTest", &HttpsTest::UploadTest));
		suite.addTest(new CppUnit::TestCaller<HttpsTest>(
			"HttpsTest::AuthBasicTest", &HttpsTest::AuthBasicTest));
		suite.addTest(new CppUnit::TestCaller<HttpsTest>(
			"HttpsTest::AuthDigestTest", &HttpsTest::AuthDigestTest));

		parent.addTest("HttpsTest", &suite);
	}
}


// # pragma mark - HTTPS


HttpsTest::HttpsTest()
	: HttpTest()
{
	fBaseUrl.SetProtocol("https");
}
