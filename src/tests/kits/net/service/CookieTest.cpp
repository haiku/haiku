/*
 * Copyright 2014, Haiku, inc.
 * Distributed under the terms of the MIT licence
 */


#include "CookieTest.h"


#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <NetworkKit.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


CookieTest::CookieTest()
{
}


CookieTest::~CookieTest()
{
}


// #pragma mark - Positive functionality tests


void
CookieTest::SimpleTest()
{
	BNetworkCookieJar jar;
	char buffer[256];
	BUrl url("http://www.chipchapin.com/WebTools/cookietest.php");

	time_t t = time(NULL) + 6400; // Cookies expire in 1h45
	struct tm* now = gmtime(&t);
	status_t result;

	// Add various cookies
	result = jar.AddCookie("testcookie1=present;", url);
	CPPUNIT_ASSERT(result == B_OK);

	strftime(buffer, sizeof(buffer),
		"testcookie2=present; expires=%A, %d-%b-%Y %H:%M:%S", now);
	result = jar.AddCookie(buffer, url);
	CPPUNIT_ASSERT(result == B_OK);

	strftime(buffer, sizeof(buffer),
		"testcookie3=present; expires=%A, %d-%b-%Y %H:%M:%S; path=/", now);
	result = jar.AddCookie(buffer, url);
	CPPUNIT_ASSERT(result == B_OK);

	t += 3200; // expire in 2h40
	now = gmtime(&t);
	strftime(buffer, sizeof(buffer),
		"testcookie4=present; path=/; domain=www.chipchapin.com; "
		"expires=%A, %d-%b-%Y %H:%M:%S", now);
	result = jar.AddCookie(buffer, url);
	CPPUNIT_ASSERT(result == B_OK);

	// Now check they were all properly added
	BNetworkCookieJar::Iterator it = jar.GetIterator();

	int count = 0;
	while (it.HasNext()) {
		count ++;
		it.Next();
	}

	CPPUNIT_ASSERT_EQUAL(4, count);
}


void
CookieTest::StandardTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/001.php");
	result = jar.AddCookie("001=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/001-1.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "001");

	CPPUNIT_ASSERT(cookie != NULL);

	CPPUNIT_ASSERT(cookie->HasValue());
	CPPUNIT_ASSERT_EQUAL(BString("1"), cookie->Value());
	CPPUNIT_ASSERT(cookie->IsSessionCookie());
	CPPUNIT_ASSERT(cookie->ShouldDeleteAtExit());

	CPPUNIT_ASSERT(!cookie->HasExpirationDate());
	CPPUNIT_ASSERT(!cookie->Secure());
	CPPUNIT_ASSERT(!cookie->HttpOnly());
}


void
CookieTest::ExpireTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/003.php");
	result = jar.AddCookie("003=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/003-1.php");
	result = jar.AddCookie("003=1; expires=Thu, 01-Jan-1970 00:00:10 GMT", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/003-2.php");
	// The cookie should be expired.
	CPPUNIT_ASSERT(_GetCookie(jar, url, "003") == NULL);
}


void
CookieTest::PathTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/004/004.php");
	result = jar.AddCookie("004=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	// Page in the same path can access the cookie
	url.SetUrlString("http://testsuites.opera.com/cookies/004/004-1.php");
	CPPUNIT_ASSERT(_GetCookie(jar, url, "004") != NULL);

	// Page in parent directory cannot access the cookie
	url.SetUrlString("http://testsuites.opera.com/cookies/004-2.php");
	CPPUNIT_ASSERT(_GetCookie(jar, url, "004") == NULL);
}


void
CookieTest::MaxSizeTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/006.php");
	BString cookieString("006=");
	for (int i = 0; i < 128; i++) {
		cookieString << "00xxxxxxxxxxxxxx16xxxxxxxxxxxxxx";
	}
	result = jar.AddCookie(cookieString, url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/006-1.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "006");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value().Length() == 4096);
}


void
CookieTest::MaxNumberTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/007.php");
	BString cookieString;

	for (int i = 1; i <= 20; i++)
	{
		cookieString.SetToFormat("007-%d=1", i);
		result = jar.AddCookie(cookieString, url);
		CPPUNIT_ASSERT(result == B_OK);
	}

	url.SetUrlString("http://testsuites.opera.com/cookies/007-1.php");
	for (int i = 1; i <= 20; i++)
	{
		cookieString.SetToFormat("007-%d", i);
		const BNetworkCookie* cookie = _GetCookie(jar, url, cookieString.String());
		CPPUNIT_ASSERT(cookie != NULL);
		CPPUNIT_ASSERT(cookie->Value() == "1");
	}
}


void
CookieTest::UpdateTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/008.php");
	result = jar.AddCookie("008=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/008-1.php");
	result = jar.AddCookie("008=2", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/008-2.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "008");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "2");
}


void
CookieTest::HttpOnlyTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/010.php");
	result = jar.AddCookie("010=1; Httponly; Max-age=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/010-1.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "010");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "1");
	CPPUNIT_ASSERT(cookie->HttpOnly());
	CPPUNIT_ASSERT(cookie->HasExpirationDate());
}


void
CookieTest::EncodingTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/011.php");
	result = jar.AddCookie("011=UTF-8 \303\246\303\270\303\245 \346\227\245\346\234\254;", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/011-1.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "011");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT_EQUAL(
		BString("UTF-8 \303\246\303\270\303\245 \346\227\245\346\234\254"),
		cookie->Value());
}


void
CookieTest::DomainTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/012.php");
	result = jar.AddCookie("012-1=1; Domain=\"opera.com\"", url);
	CPPUNIT_ASSERT(result == B_OK);
	result = jar.AddCookie("012-1=1; Domain=\"example.com\"", url);
	CPPUNIT_ASSERT(result != B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/012-1.php");
	const BNetworkCookie* cookie = _GetCookie(jar, url, "012-1");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "1");

	cookie = _GetCookie(jar, url, "012-2");
	CPPUNIT_ASSERT(cookie == NULL);
}


void
CookieTest::PersistantTest()
{
	BNetworkCookieJar jar;
	status_t result;
	char buffer[256];

	time_t t = time(NULL) + 100000;
	struct tm* now = gmtime(&t);

	NextSubTest();

	BUrl url("http://testsuites.opera.com/cookies/013.php");

	strftime(buffer, sizeof(buffer),
		"013-1=1; Expires=%a, %d-%b-%Y %H:%M:%S", now);
	result = jar.AddCookie(buffer, url);
	CPPUNIT_ASSERT(result == B_OK);

	result = jar.AddCookie("013-2=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	NextSubTest();

	url.SetUrlString("http://testsuites.opera.com/cookies/013-1.php");

	const BNetworkCookie* cookie = _GetCookie(jar, url, "013-1");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "1");
	CPPUNIT_ASSERT(cookie->HasExpirationDate());

	cookie = _GetCookie(jar, url, "013-2");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "1");
	CPPUNIT_ASSERT(!cookie->HasExpirationDate());

	// Simulate exit
	jar.PurgeForExit();

	NextSubTest();

	cookie = _GetCookie(jar, url, "013-1");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == "1");
	CPPUNIT_ASSERT(cookie->HasExpirationDate());

	cookie = _GetCookie(jar, url, "013-2");
	CPPUNIT_ASSERT(cookie == NULL);
}


void
CookieTest::OverwriteTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/015/015.php");
	result = jar.AddCookie("015-01=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/015-1.php");
	result = jar.AddCookie("015-01=1", url);
	result = jar.AddCookie("015-02=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/015/015-2.php");
	result = jar.AddCookie("015-01=1", url);
	result = jar.AddCookie("015-02=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/015/015-3.php");
	BNetworkCookieJar::UrlIterator it = jar.GetUrlIterator(url);
	int count = 0;

	while (it.HasNext()) {
		it.Next();
		count++;
	}

	CPPUNIT_ASSERT_EQUAL(4, count);
}


void
CookieTest::OrderTest()
{
	BNetworkCookieJar jar;
	status_t result;

	BUrl url("http://testsuites.opera.com/cookies/016.php");
	result = jar.AddCookie("016-01=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/016/016-1.php");
	result = jar.AddCookie("016-02=1", url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/016/016-2.php");
	BNetworkCookieJar::UrlIterator it = jar.GetUrlIterator(url);
	int count = 0;

	// Check that the cookie with the most specific path is sent first
	while (it.HasNext()) {
		const BNetworkCookie* cookie = it.Next();
		switch(count)
		{
			case 0:
				CPPUNIT_ASSERT_EQUAL(BString("016-02"), cookie->Name());
				break;
			case 1:
				CPPUNIT_ASSERT_EQUAL(BString("016-01"), cookie->Name());
				break;
		}
		count++;
	}

	CPPUNIT_ASSERT_EQUAL(2, count);
}


// #pragma mark - Error handling and extended tests


void
CookieTest::ExpireParsingTest()
{
	BUrl url("http://testsuites.opera.com/cookies/301.php");
	BNetworkCookie cookie;
	status_t result;

	BString bigData("301-16=1; Expires=\"");
	for (int i = 0; i < 1500; i++)
		bigData << "abcdefghijklmnopqrstuvwxyz";
	bigData << "\";";

	struct Test {
		const char* cookieString;
		bool canParse;
		bool sessionOnly;
		bool expired;
	};

	Test tests[] = {
		{ "301-01=1; Expires=\"notAValidValue\";",
			true, true, false }, // Obviously invalid date
		{ "301-02=1; Expires=\"Wed, 08-Nov-2035 01:04:33\";",
			true, false, false }, // Valid date
		{ "301-03=1; Expires=\"Tue, 19-Jan-2038 03:14:06\";",
			true, false, false }, // Valid date, after year 2038 time_t overflow
		{ "301-04=1; Expires=\"Fri, 13-Dec-1901 20:45:51\";",
			true, false, true }, // Valid date, in the past
		{ "301-05=1; Expires=\"Thu, 33-Nov-2035 01:04:33\";",
			true, true, false }, // Invalid day
		{ "301-06=1; Expires=\"Wed, 08-Siz-2035 01:04:33\";",
			true, true, false }, // Invalid month
		{ "301-07=1; Expires=\"Wed, 08-Nov-9035 01:04:33\";",
			true, false, false }, // Very far in the future
			// NOTE: Opera testsuite considers it should be a session cookie.
		{ "301-08=1; Expires=\"Wed, 08-Nov-2035 75:04:33\";",
			true, true, false }, // Invalid hour
		{ "301-09=1; Expires=\"Wed, 08-Nov-2035 01:75:33\";",
			true, true, false }, // Invalid minute
		{ "301-10=1; Expires=\"Wed, 08-Nov-2035 01:04:75\";",
			true, true, false }, // Invalid second
		{ "301-11=1; Expires=\"XXX, 08-Nov-2035 01:04:33\";",
			true, true, false }, // Invalid weekday
		{ "301-12=1; Expires=\"Thu, XX-Nov-2035 01:04:33\";",
			true, true, false }, // Non-digit day
		{ "301-13=1; Expires=\"Thu, 08-Nov-XXXX 01:04:33\";",
			true, true, false }, // Non-digit year
		{ "301-14=1; Expires=\"Thu, 08-Nov-2035 XX:XX:33\";",
			true, true, false }, // Non-digit hour and minute
		{ "301-15=1; Expires=\"Thu, 08-Nov-2035 01:04:XX\";",
			true, true, false }, // Non-digit second
		{ bigData.String(),
			true, true, false }, // Very long invalid string
			// NOTE: Opera doesn't register the cookie at all.
		{ "301-17=1; Expires=\"Thu, 99999-Nov-2035 01:04:33\";",
			true, true, false }, // Day with many digits
		{ "301-18=1; Expires=\"Thu, 25-Nov-99999 01:04:33\";",
			true, true, false }, // Year with many digits
			// NOTE: Opera tests 301-17 twice and misses this test.
		{ "301-19=1; Expires=\"Thu, 25-Nov-2035 99999:04:33\";",
			true, true, false }, // Hour with many digits
		{ "301-20=1; Expires=\"Thu, 25-Nov-2035 01:99999:33\";",
			true, true, false }, // Minute with many digits
		{ "301-21=1; Expires=\"Thu, 25-Nov-2035 01:04:99999\";",
			true, true, false }, // Second with many digits
		{ "301-22=1; Expires=\"99999999999999999999\";",
			true, true, false }, // Huge number
		{ "301-23=1; Expires=\"Fri, 13-Dec-101 20:45:51\";",
			true, false, true }, // Very far in the past
		{ "301-24=1; EXPiReS=\"Wed, 08-Nov-2035 01:04:33\";",
			true, false, false }, // Case insensitive key parsing
		{ "301-25=1; Expires=Wed, 08-Nov-2035 01:04:33\";",
			true, true, false }, // Missing opening quote
			// NOTE: unlike Opera, we accept badly quoted values for cookie
			// attributes. This allows to handle unquoted values from the early
			// cookie spec properly. However, a date with a quote inside it
			// should not be accepted, so this will be a session cookie.
		{ "301-26=1; Expires=\"Wed, 08-Nov-2035 01:04:33;",
			true, true, false }, // Missing closing quote
		{ "301-27=1; Expires:\"Wed, 08-Nov-2035 01:04:33\";",
			true, true, false }, // Uses : instead of =
			// NOTE: unlike Opera, we allow this as a cookie with a strange
			// name and no value
		{ "301-28=1; Expires;=\"Wed, 08-Nov-2035 01:04:33\";",
			true, true, false }, // Extra ; after name
		{ "301-29=1; Expired=\"Wed, 08-Nov-2035 01:04:33\";",
			true, true, false }, // Expired instead of Expires
		{ "301-30=1; Expires=\"Wed; 08-Nov-2035 01:04:33\";",
			true, true, false }, // ; in value
		{ "301-31=1; Expires=\"Wed,\\r\\n 08-Nov-2035 01:04:33\";",
			true, true, false }, // escaped newline in value
			// NOTE: Only here for completeness. This is what the Opera
			// testsuite sends in test 31, but I suspect they were trying to
			// test the following case.
		{ "301-31b=1; Expires=\"Wed,\r\n 08-Nov-2035 01:04:33\";",
			true, false, false }, // newline in value
			// NOTE: This can't really happen when we get cookies from HTTP
			// headers. It could happen when the cookie is set from meta html
			// tags or from JS.
	};

	for (unsigned int i = 0; i < sizeof(tests) / sizeof(Test); i++)
	{
		NextSubTest();

		result = cookie.ParseCookieString(tests[i].cookieString, url);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Cookie can be parsed",
			tests[i].canParse, result == B_OK);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Cookie is session only",
			tests[i].sessionOnly, cookie.IsSessionCookie());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Cookie has expired",
			tests[i].expired, cookie.ShouldDeleteNow());
	}
}


void
CookieTest::PathMatchingTest()
{
	const BUrl url("http://testsuites.opera.com/cookies/302/302.php");
	const BUrl url1("http://testsuites.opera.com/cookies/302-5.php");
	const BUrl url2("http://testsuites.opera.com/cookies/302/302-3.php");
	const BUrl url3("http://testsuites.opera.com/cookies/302/sub/302-4.php");
	const BUrl url4("http://testsuites.opera.com/cookies/302-2/302-6.php");
	BNetworkCookie cookie;
	status_t result;

	BString bigData("302-24=1; Path=\"/cookies/302/");
	for (int i = 0; i < 1500; i++)
		bigData << "abcdefghijklmnopqrstuvwxyz";
	bigData << "\";";

	struct Test {
		const char* cookieString;
		bool canSet;
		bool url1;
		bool url2;
		bool url3;
		bool url4;
	};

	const Test tests[] = {
		{ "302-01=1; Path=\"/\"",
			true, true, true, true, true },
		{ "302-02=1; Path=\"/cookies\"",
			true, true, true, true, true },
		{ "302-03=1; Path=\"/cookies/\"",
			true, true, true, true, true },
		{ "302-04=1; Path=\"/cookies/302\"",
			true, false, true, true, true },
		{ "302-05=1; Path=\"/cookies/302/\"",
			true, false, true, true, false },
		{ "302-06=1; Path=\"/cookies/302/.\"",
			false, false, false, false, false },
		{ "302-07=1; Path=\"/cookies/302/../302\"",
			false, false, false, false, false },
		{ "302-08=1; Path=\"/cookies/302/../302-2\"",
			false, false, false, false, false },
		{ "302-09=1; Path=\"/side\"",
			false, false, false, false, false },
		{ "302-10=1; Path=\"/cookies/302-2\"",
			true, false, false, false, true },
		{ "302-11=1; Path=\"/cookies/302-2/\"",
			true, false, false, false, true },
		{ "302-12=1; Path=\"sub\"",
			false, false, false, false, false },
		{ "302-13=1; Path=\"sub/\"",
			false, false, false, false, false },
		{ "302-14=1; Path=\".\"",
			false, false, false, false, false },
		{ "302-15=1; Path=\"/cookies/302/sub\"",
			true, false, false, true, false },
		{ "302-16=1; Path=\"/cookies/302/sub/\"",
			true, false, false, true, false },
		{ "302-17=1; Path=\"/cookies/302/sub/..\"",
			false, false, false, false, false },
		{ "302-18=1; Path=/",
			true, true, true, true, true },
		{ "302-19=1; Path=/ /",
			false, false, false, false, false },
		{ "302-20=1; Path=\"/",
			false, false, false, false, false },
		{ "302-21=1; Path=/\"",
			false, false, false, false, false },
		{ "302-22=1; Path=\\/",
			false, false, false, false, false },
		{ "302-23=1; Path=\n/",
			false, false, false, false, false },
		{ bigData,
			false, false, false, false, false },
	};

	for (unsigned int i = 0; i < sizeof(tests) / sizeof(Test); i++)
	{
		NextSubTest();

		result = cookie.ParseCookieString(tests[i].cookieString, url);

		CPPUNIT_ASSERT_EQUAL_MESSAGE("Allowed to set cookie",
			tests[i].canSet, result == B_OK);
		CPPUNIT_ASSERT_EQUAL(tests[i].url1, cookie.IsValidForUrl(url1));
		CPPUNIT_ASSERT_EQUAL(tests[i].url2, cookie.IsValidForUrl(url2));
		CPPUNIT_ASSERT_EQUAL(tests[i].url3, cookie.IsValidForUrl(url3));
		CPPUNIT_ASSERT_EQUAL(tests[i].url4, cookie.IsValidForUrl(url4));
	}
}


void
CookieTest::DomainMatchingTest()
{
	const BUrl setter("http://testsuites.opera.com/cookies/304.php");
	const BUrl getter("http://testsuites.opera.com/cookies/304-1.php");

	BString bigData("304-12=1; Domain=\"");
	for (int i = 0; i < 1500; i++)
		bigData << "abcdefghijklmnopqrstuvwxyz";
	bigData << "\";";

	struct Test {
		const char* cookieString;
		bool shouldMatch;
	};

	const Test tests[] = {
		{ "304-01=1; Domain=\"opera.com\"", true },
		{ "304-02=1; Domain=\".opera.com\"", true },
		{ "304-03=1; Domain=\".com\"", false },
		{ "304-04=1; Domain=\"pera.com\"", false },
		{ "304-05=1; Domain=\"?pera.com\"", false },
		{ "304-06=1; Domain=\"*.opera.com\"", false },
		{ "304-07=1; Domain=\"300.300.300.300\"", false },
		{ "304-08=1; Domain=\"123456123456123456\"", false },
		{ "304-09=1; Domain=\"/opera.com\"", false },
		{ "304-10=1; Domain=\"opera.com/\"", false },
		{ "304-11=1; Domain=\"example.com\"", false },
		{ bigData, false },
		{ "304-13=1; Domain=\"opera com\"", false },
		{ "304-14=1; Domain=opera.com\"", false },
		{ "304-15=1; Domain=\"opera.com", false },
		{ "304-16=1; Domain=opera.com", true },
		{ "304-17=1; Domain=\"*.com\"", false },
		{ "304-18=1; Domain=\"*opera.com\"", false },
		{ "304-19=1; Domain=\"*pera.com\"", false },
		{ "304-20=1; Domain=\"\"", false },
		{ "304-21=1; Domain=\"\346\227\245\346\234\254\"", false },
		{ "304-22=1; Domain=\"OPERA.COM\"", true },
		{ "304-23=1; Domain=\"195.189.143.182\"", false },
	};

	for (unsigned int i = 0; i < sizeof(tests) / sizeof(Test); i++)
	{
		NextSubTest();

		BNetworkCookie cookie(tests[i].cookieString, setter);
		CPPUNIT_ASSERT_EQUAL(tests[i].shouldMatch,
			cookie.IsValidForUrl(getter));
	}
}


void
CookieTest::MaxAgeParsingTest()
{
	const BUrl setter("http://testsuites.opera.com/cookies/305.php");

	BString bigData("305-12=1; Max-Age=\"");
	for (int i = 0; i < 1500; i++)
		bigData << "abcdefghijklmnopqrstuvwxyz";
	bigData << "\";";

	struct Test {
		const char* cookieString;
		bool expired;
	};

	const Test tests[] = {
		{ "305-01=1; Max-Age=\"notAValidValue\"", true },
		{ "305-02=1; Max-Age=\"Wed, 08-Nov-2035 01:04:33\"", true },
		{ "305-03=1; Max-Age=\"0\"", true },
		{ "305-04=1; Max-Age=\"1\"", false },
		{ "305-05=1; Max-Age=\"10000\"", false },
		{ "305-06=1; Max-Age=\"-1\"", true },
		{ "305-07=1; Max-Age=\"0.5\"", true },
		{ "305-08=1; Max-Age=\"9999999999999999999999999\"", false },
		{ "305-09=1; Max-Age=\"-9999999999999999999999999\"", true },
		{ "305-10=1; Max-Age=\"+10000\"", false },
		{ "305-11=1; Max-Age=\"Fri, 13-Dec-1901 20:45:52\"", true },
		{ bigData, true },
		{ "305-13=1; Max-Age=\"0+10000\"", true },
		{ "305-14=1; Max-Age=\"10000+0\"", true },
		{ "305-15=1; Max-Age=10000\"", true },
		{ "305-16=1; Max-Age=\"10000", true },
		{ "305-17=1; Max-Age=10000", false },
		{ "305-18=1; Max-Age=100\"00", true },
	};

	for (unsigned int i = 0; i < sizeof(tests) / sizeof(Test); i++)
	{
		NextSubTest();

		BNetworkCookie cookie(tests[i].cookieString, setter);
		CPPUNIT_ASSERT_EQUAL(tests[i].expired, cookie.ShouldDeleteNow());
	}
}


void
CookieTest::ExplodeTest()
{
	struct Test {
		const char* cookieString;
		const char*	url;
		struct
		{
			bool		valid;
			const char* name;
			const char* value;
			const char* domain;
			const char* path;
			bool 		secure;
			bool 		httponly;
			bool		session;
			BDateTime	expire;
		} expected;
	};

	Test tests[] = {
	//     Cookie string      URL
	//     ------------- -------------
	//		   Valid     Name     Value	       Domain         Path      Secure  HttpOnly Session  Expiration
	//       --------- -------- --------- ----------------- ---------  -------- -------- -------  ----------
		// Normal cookies
		{ "name=value", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		{ "name=value; domain=example.com; path=/; secure", "http://www.example.com/path/path",
			{  true,    "name",  "value",   "example.com",   "/"    ,   true,    false,   true,   BDateTime() } },
		{ "name=value; httponly; secure", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   true,    true,    true,   BDateTime() } },
		{ "name=value; expires=Wed, 20-Feb-2013 20:00:00 UTC", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   false,   false,   false,
				BDateTime(BDate(2013, 2, 20), BTime(20, 0, 0, 0)) } },
		// Valid cookie with bad form
		{ "name=  ;  domain   =example.com  ;path=/;  secure = yup  ; blahblah ;)", "http://www.example.com/path/path",
			{  true,    "name",  "",   "example.com",   "/"    ,   true,    false,   true,   BDateTime() } },
		// Invalid path
		{ "name=value; path=invalid", "http://www.example.com/path/path",
			{  false,    "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		// Setting for other subdomain (invalid)
		{ "name=value; domain=subdomain.example.com", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		// Various invalid cookies
		{ "name", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		{ "; domain=example.com", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } }
	};
	
	BNetworkCookie cookie;

	for (uint32 i = 0; i < (sizeof(tests) / sizeof(Test)); i++) {
		NextSubTest();

		BUrl url(tests[i].url);
		cookie.ParseCookieString(tests[i].cookieString, url);

		CPPUNIT_ASSERT(tests[i].expected.valid == cookie.IsValid());

		if (!tests[i].expected.valid)
			continue;

		CPPUNIT_ASSERT_EQUAL(BString(tests[i].expected.name), cookie.Name());
		CPPUNIT_ASSERT_EQUAL(BString(tests[i].expected.value), cookie.Value());
		CPPUNIT_ASSERT_EQUAL(BString(tests[i].expected.domain),
			cookie.Domain());
		CPPUNIT_ASSERT_EQUAL(BString(tests[i].expected.path), cookie.Path());
		CPPUNIT_ASSERT(tests[i].expected.secure == cookie.Secure());
		CPPUNIT_ASSERT(tests[i].expected.httponly == cookie.HttpOnly());
		CPPUNIT_ASSERT(tests[i].expected.session == cookie.IsSessionCookie());

		if (!cookie.IsSessionCookie())
			CPPUNIT_ASSERT_EQUAL(tests[i].expected.expire.Time_t(),
				cookie.ExpirationDate());
	}
}


/* static */ void
CookieTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("CookieTest");

	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::SimpleTest",
		&CookieTest::SimpleTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::StandardTest", &CookieTest::StandardTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::ExpireTest",
		&CookieTest::ExpireTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::PathTest",
		&CookieTest::PathTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::MaxSizeTest",
		&CookieTest::MaxSizeTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::MaxNumberTest", &CookieTest::MaxNumberTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::UpdateTest",
		&CookieTest::UpdateTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::HttpOnlyTest", &CookieTest::HttpOnlyTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::EncodingTest", &CookieTest::EncodingTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>("CookieTest::DomainTest",
		&CookieTest::DomainTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::PersistantTest", &CookieTest::PersistantTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::OverwriteTest", &CookieTest::OverwriteTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::OrderTest", &CookieTest::OrderTest));

	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::ExpireParsingTest", &CookieTest::ExpireParsingTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::PathMatchingTest", &CookieTest::PathMatchingTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::DomainMatchingTest", &CookieTest::DomainMatchingTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::MaxAgeParsingTest", &CookieTest::MaxAgeParsingTest));
	suite.addTest(new CppUnit::TestCaller<CookieTest>(
		"CookieTest::ExplodeTest", &CookieTest::ExplodeTest));

	parent.addTest("CookieTest", &suite);
}


const BNetworkCookie*
CookieTest::_GetCookie(BNetworkCookieJar& jar, const BUrl& url,
	const char* name)
{
	BNetworkCookieJar::UrlIterator it = jar.GetUrlIterator(url);
	const BNetworkCookie* result = NULL;

	while (it.HasNext()) {
		const BNetworkCookie* cookie = it.Next();
		if (cookie->Name() == name) {
			// Make sure the cookie is found only once.
			CPPUNIT_ASSERT(result == NULL);

			result = cookie;
		}
	}

	return result;
}
