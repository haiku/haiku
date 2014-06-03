/*
 * Copyright 2010, Christophe Huriaux
 * Copyright 2014, Haiku, inc.
 * Distributed under the terms of the MIT licence
 */


#include "UrlTest.h"


#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <NetworkKit.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


UrlTest::UrlTest()
{
}


UrlTest::~UrlTest()
{
}


// Test if rebuild url are the same length of parsed one
void UrlTest::LengthTest()
{
	int8 testIndex;
	BUrl testUrl;

	const char* kTestLength[] =
	{
		"http://user:pass@www.foo.com:80/path?query#fragment",
		"http://user:pass@www.foo.com:80/path?query#",
		"http://user:pass@www.foo.com:80/path?query",
		"http://user:pass@www.foo.com:80/path?",
		"http://user:pass@www.foo.com:80/path",
		"http://user:pass@www.foo.com:80/",
		"http://user:pass@www.foo.com",
		"http://user:pass@",
		"http://www.foo.com",
		"http://",
		"http:"
	};

	for (testIndex = 0; kTestLength[testIndex] != NULL; testIndex++)
	{
		NextSubTest();
		testUrl.SetUrlString(kTestLength[testIndex]);

		CPPUNIT_ASSERT(strlen(kTestLength[testIndex]) == strlen(testUrl.UrlString()));
	}
}

typedef struct
{
	const char* url;

	struct
	{
		const char* protocol;
		const char* userName;
		const char* password;
		const char* host;
		int16		port;
		const char* path;
		const char* request;
		const char* fragment;
	} expected;
} ExplodeTest;


const ExplodeTest	kTestExplode[] =
	//  Url
	//       Protocol     User  Password  Hostname  Port     Path    Request      Fragment
	//       -------- --------- --------- --------- ---- ---------- ---------- ------------
	{
		{ "http://user:pass@host:80/path?query#fragment",
			{ "http",   "user",   "pass",   "host",	 80,   "/path",	  "query",   "fragment" } },
		{ "http://www.host.tld/path?query#fragment",
			{ "http",   "",        "", "www.host.tld",0,   "/path",	  "query",   "fragment" } }
	};

void UrlTest::ExplodeImplodeTest()
{
	uint8 testIndex;
	BUrl testUrl;

	for (testIndex = 0; testIndex < (sizeof(kTestExplode) / sizeof(ExplodeTest)); testIndex++)
	{
		NextSubTest();
		testUrl.SetUrlString(kTestExplode[testIndex].url);

		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].url)
			== BString(testUrl.UrlString()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.protocol)
			== BString(testUrl.Protocol()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.userName)
			== BString(testUrl.UserName()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.password)
			== BString(testUrl.Password()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.host)
			== BString(testUrl.Host()));
		CPPUNIT_ASSERT(kTestExplode[testIndex].expected.port == testUrl.Port());
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.path)
			== BString(testUrl.Path()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.request)
			== BString(testUrl.Request()));
		CPPUNIT_ASSERT(BString(kTestExplode[testIndex].expected.fragment)
			== BString(testUrl.Fragment()));
	}
}


void
UrlTest::PathOnly()
{
	BUrl test = "lol";
	CPPUNIT_ASSERT(test.HasPath());
	CPPUNIT_ASSERT(test.Path() == "lol");
	CPPUNIT_ASSERT(test.UrlString() == "lol");
}


/* static */ void
UrlTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("UrlTest");

	suite.addTest(new CppUnit::TestCaller<UrlTest>(
		"UrlTest::LengthTest", &UrlTest::LengthTest));
	suite.addTest(new CppUnit::TestCaller<UrlTest>(
		"UrlTest::ExplodeImplodeTest", &UrlTest::ExplodeImplodeTest));
	suite.addTest(new CppUnit::TestCaller<UrlTest>("UrlTest::PathOnly",
		&UrlTest::PathOnly));

	parent.addTest("UrlTest", &suite);
}
