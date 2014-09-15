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


static const int kHeaderCountInTrivialRequest = 7;
	// FIXME This is too strict and not very useful.


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
	BUrlContext* c = new BUrlContext();
	c->AcquireReference();
	BHttpRequest t(testUrl);

	t.SetContext(c);

	CPPUNIT_ASSERT(t.Run());

	while (t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, t.Status());

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(kHeaderCountInTrivialRequest,
		r.Headers().CountHeaders());
	CPPUNIT_ASSERT_EQUAL(42, r.Length());
		// Fixed size as we know the response format.
	CPPUNIT_ASSERT(!c->GetCookieJar().GetIterator().HasNext());
		// This page should not set cookies

	c->ReleaseReference();
}


void
HttpTest::ProxyTest()
{
	BUrl testUrl(fBaseUrl, "/user-agent");

	BUrlContext* c = new BUrlContext();
	c->AcquireReference();
	c->SetProxy("120.203.214.182", 83);

	BHttpRequest t(testUrl);
	t.SetContext(c);

	BUrlProtocolListener l;
	t.SetListener(&l);

	CPPUNIT_ASSERT(t.Run());

	while (t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, t.Status());

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());

printf("%s\n", r.StatusText().String());

	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(42, r.Length());
		// Fixed size as we know the response format.
	CPPUNIT_ASSERT(!c->GetCookieJar().GetIterator().HasNext());
		// This page should not set cookies

	c->ReleaseReference();
}


class PortTestListener: public BUrlProtocolListener
{
public:
	virtual			~PortTestListener() {};

			void	DataReceived(BUrlRequest*, const char* data, off_t,
						ssize_t size)
			{
				fResult.Append(data, size);
			}

	BString fResult;
};


void
HttpTest::PortTest()
{
	BUrl testUrl("http://portquiz.net:4242");
	BHttpRequest t(testUrl);

	// portquiz returns more easily parseable results when UA is Wget...
	t.SetUserAgent("Wget/1.15 (haiku testsuite)");

	PortTestListener listener;
	t.SetListener(&listener);

	CPPUNIT_ASSERT(t.Run());

	while (t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, t.Status());

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());

	CPPUNIT_ASSERT(listener.fResult.StartsWith("Port 4242 test successful!"));
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

	while (t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, t.Status());

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(466, r.Length());
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

	while (t.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, t.Status());

	const BHttpResult& r = dynamic_cast<const BHttpResult&>(t.Result());
	CPPUNIT_ASSERT_EQUAL(200, r.StatusCode());
	CPPUNIT_ASSERT_EQUAL(BString("OK"), r.StatusText());
	CPPUNIT_ASSERT_EQUAL(kHeaderCountInTrivialRequest,
		r.Headers().CountHeaders());
	CPPUNIT_ASSERT_EQUAL(48, r.Length());
		// Fixed size as we know the response format.
}


/* static */ template<class T> void
HttpTest::_AddCommonTests(BString prefix, CppUnit::TestSuite& suite)
{
	BString name;

	name = prefix;
	name << "GetTest";
	suite.addTest(new CppUnit::TestCaller<T>(name.String(), &T::GetTest));

	name = prefix;
	name << "UploadTest";
	suite.addTest(new CppUnit::TestCaller<T>(name.String(), &T::UploadTest));

	name = prefix;
	name << "AuthBasicTest";
	suite.addTest(new CppUnit::TestCaller<T>(name.String(), &T::AuthBasicTest));

	name = prefix;
	name << "AuthDigestTest";
	suite.addTest(new CppUnit::TestCaller<T>(name.String(), &T::AuthDigestTest));
}


/* static */ void
HttpTest::AddTests(BTestSuite& parent)
{
	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpTest");

		// HTTP + HTTPs
		_AddCommonTests<HttpTest>("HttpTest::", suite);

		// HTTP-only
		suite.addTest(new CppUnit::TestCaller<HttpTest>(
			"HttpTest::PortTest", &HttpTest::PortTest));

		suite.addTest(new CppUnit::TestCaller<HttpTest>("HttpTest::ProxyTest",
			&HttpTest::ProxyTest));

		parent.addTest("HttpTest", &suite);
	}

	{
		CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpsTest");

		// HTTP + HTTPs
		_AddCommonTests<HttpsTest>("HttpsTest::", suite);

		parent.addTest("HttpsTest", &suite);
	}
}


// # pragma mark - HTTPS


HttpsTest::HttpsTest()
	: HttpTest()
{
	fBaseUrl.SetProtocol("https");
}
