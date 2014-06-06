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
	BNetworkCookie* cookie = _GetCookie(jar, url, "001");

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

	static const char* bigdata = "XXXXXXXX10XXXXXXXX20XXXXXXXX30XXXXXXXX40"
		"XXXXXXXX50XXXXXXXX60XXXXXXXX70XXXXXXXX80XXXXXXXX90XXXXXXX100"
		"XXXXXXX110XXXXXXX120XXXXXXX130XXXXXXX140XXXXXXX150XXXXXXX160"
		"XXXXXXX170XXXXXXX180XXXXXXX190XXXXXXX200XXXXXXX210XXXXXXX220"
		"XXXXXXX230XXXXXXX240XXXXXXX250XXXXXXX260XXXXXXX270XXXXXXX280"
		"XXXXXXX290XXXXXXX300XXXXXXX310XXXXXXX320XXXXXXX330XXXXXXX340"
		"XXXXXXX350XXXXXXX360XXXXXXX370XXXXXXX380XXXXXXX390XXXXXXX400"
		"XXXXXXX410XXXXXXX420XXXXXXX430XXXXXXX440XXXXXXX450XXXXXXX460"
		"XXXXXXX470XXXXXXX480XXXXXXX490XXXXXXX500XXXXXXX510XXXXXXX520"
		"XXXXXXX530XXXXXXX540XXXXXXX550XXXXXXX560XXXXXXX570XXXXXXX580"
		"XXXXXXX590XXXXXXX600XXXXXXX610XXXXXXX620XXXXXXX630XXXXXXX640"
		"XXXXXXX650XXXXXXX660XXXXXXX670XXXXXXX680XXXXXXX690XXXXXXX700"
		"XXXXXXX710XXXXXXX720XXXXXXX730XXXXXXX740XXXXXXX750XXXXXXX760"
		"XXXXXXX770XXXXXXX780XXXXXXX790XXXXXXX800XXXXXXX810XXXXXXX820"
		"XXXXXXX830XXXXXXX840XXXXXXX850XXXXXXX860XXXXXXX870XXXXXXX880"
		"XXXXXXX890XXXXXXX900XXXXXXX910XXXXXXX920XXXXXXX930XXXXXXX940"
		"XXXXXXX950XXXXXXX960XXXXXXX970XXXXXXX980XXXXXXX990XXXXXX1000"
		"XXXXXX1010XXXXXX1020XXXXXX1030XXXXXX1040XXXXXX1050XXXXXX1060"
		"XXXXXX1070XXXXXX1080XXXXXX1090XXXXXX1100XXXXXX1110XXXXXX1120"
		"XXXXXX1130XXXXXX1140XXXXXX1150XXXXXX1160XXXXXX1170XXXXXX1180"
		"XXXXXX1190XXXXXX1200XXXXXX1210XXXXXX1220XXXXXX1230XXXXXX1240"
		"XXXXXX1250XXXXXX1260XXXXXX1270XXXXXX1280XXXXXX1290XXXXXX1300"
		"XXXXXX1310XXXXXX1320XXXXXX1330XXXXXX1340XXXXXX1350XXXXXX1360"
		"XXXXXX1370XXXXXX1380XXXXXX1390XXXXXX1400XXXXXX1410XXXXXX1420"
		"XXXXXX1430XXXXXX1440XXXXXX1450XXXXXX1460XXXXXX1470XXXXXX1480"
		"XXXXXX1490XXXXXX1500XXXXXX1510XXXXXX1520XXXXXX1530XXXXXX1540"
		"XXXXXX1550XXXXXX1560XXXXXX1570XXXXXX1580XXXXXX1590XXXXXX1600"
		"XXXXXX1610XXXXXX1620XXXXXX1630XXXXXX1640XXXXXX1650XXXXXX1660"
		"XXXXXX1670XXXXXX1680XXXXXX1690XXXXXX1700XXXXXX1710XXXXXX1720"
		"XXXXXX1730XXXXXX1740XXXXXX1750XXXXXX1760XXXXXX1770XXXXXX1780"
		"XXXXXX1790XXXXXX1800XXXXXX1810XXXXXX1820XXXXXX1830XXXXXX1840"
		"XXXXXX1850XXXXXX1860XXXXXX1870XXXXXX1880XXXXXX1890XXXXXX1900"
		"XXXXXX1910XXXXXX1920XXXXXX1930XXXXXX1940XXXXXX1950XXXXXX1960"
		"XXXXXX1970XXXXXX1980XXXXXX1990XXXXXX2000XXXXXX2010XXXXXX2020"
		"XXXXXX2030XXXXXX2040XXXXXX2050XXXXXX2060XXXXXX2070XXXXXX2080"
		"XXXXXX2090XXXXXX2100XXXXXX2110XXXXXX2120XXXXXX2130XXXXXX2140"
		"XXXXXX2150XXXXXX2160XXXXXX2170XXXXXX2180XXXXXX2190XXXXXX2200"
		"XXXXXX2210XXXXXX2220XXXXXX2230XXXXXX2240XXXXXX2250XXXXXX2260"
		"XXXXXX2270XXXXXX2280XXXXXX2290XXXXXX2300XXXXXX2310XXXXXX2320"
		"XXXXXX2330XXXXXX2340XXXXXX2350XXXXXX2360XXXXXX2370XXXXXX2380"
		"XXXXXX2390XXXXXX2400XXXXXX2410XXXXXX2420XXXXXX2430XXXXXX2440"
		"XXXXXX2450XXXXXX2460XXXXXX2470XXXXXX2480XXXXXX2490XXXXXX2500"
		"XXXXXX2510XXXXXX2520XXXXXX2530XXXXXX2540XXXXXX2550XXXXXX2560"
		"XXXXXX2570XXXXXX2580XXXXXX2590XXXXXX2600XXXXXX2610XXXXXX2620"
		"XXXXXX2630XXXXXX2640XXXXXX2650XXXXXX2660XXXXXX2670XXXXXX2680"
		"XXXXXX2690XXXXXX2700XXXXXX2710XXXXXX2720XXXXXX2730XXXXXX2740"
		"XXXXXX2750XXXXXX2760XXXXXX2770XXXXXX2780XXXXXX2790XXXXXX2800"
		"XXXXXX2810XXXXXX2820XXXXXX2830XXXXXX2840XXXXXX2850XXXXXX2860"
		"XXXXXX2870XXXXXX2880XXXXXX2890XXXXXX2900XXXXXX2910XXXXXX2920"
		"XXXXXX2930XXXXXX2940XXXXXX2950XXXXXX2960XXXXXX2970XXXXXX2980"
		"XXXXXX2990XXXXXX3000XXXXXX3010XXXXXX3020XXXXXX3030XXXXXX3040"
		"XXXXXX3050XXXXXX3060XXXXXX3070XXXXXX3080XXXXXX3090XXXXXX3100"
		"XXXXXX3110XXXXXX3120XXXXXX3130XXXXXX3140XXXXXX3150XXXXXX3160"
		"XXXXXX3170XXXXXX3180XXXXXX3190XXXXXX3200XXXXXX3210XXXXXX3220"
		"XXXXXX3230XXXXXX3240XXXXXX3250XXXXXX3260XXXXXX3270XXXXXX3280"
		"XXXXXX3290XXXXXX3300XXXXXX3310XXXXXX3320XXXXXX3330XXXXXX3340"
		"XXXXXX3350XXXXXX3360XXXXXX3370XXXXXX3380XXXXXX3390XXXXXX3400"
		"XXXXXX3410XXXXXX3420XXXXXX3430XXXXXX3440XXXXXX3450XXXXXX3460"
		"XXXXXX3470XXXXXX3480XXXXXX3490XXXXXX3500XXXXXX3510XXXXXX3520"
		"XXXXXX3530XXXXXX3540XXXXXX3550XXXXXX3560XXXXXX3570XXXXXX3580"
		"XXXXXX3590XXXXXX3600XXXXXX3610XXXXXX3620XXXXXX3630XXXXXX3640"
		"XXXXXX3650XXXXXX3660XXXXXX3670XXXXXX3680XXXXXX3690XXXXXX3700"
		"XXXXXX3710XXXXXX3720XXXXXX3730XXXXXX3740XXXXXX3750XXXXXX3760"
		"XXXXXX3770XXXXXX3780XXXXXX3790XXXXXX3800XXXXXX3810XXXXXX3820"
		"XXXXXX3830XXXXXX3840XXXXXX3850XXXXXX3860XXXXXX3870XXXXXX3880"
		"XXXXXX3890XXXXXX3900XXXXXX3910XXXXXX3920XXXXXX3930XXXXXX3940"
		"XXXXXX3950XXXXXX3960XXXXXX3970XXXXXX3980XXXXXX3990XXXXXX4000"
		"XXXXXX4010XXXXXX4020XXXXXX4030XXXXXX4040XXXXXX4050XXXXXX4060"
		"XXXXXX4070XXXXXX4080XXXXXX4090XX";

	BUrl url("http://testsuites.opera.com/cookies/006.php");
	BString cookieString("006=");
	cookieString << bigdata;
	result = jar.AddCookie(cookieString, url);
	CPPUNIT_ASSERT(result == B_OK);

	url.SetUrlString("http://testsuites.opera.com/cookies/006-1.php");
	BNetworkCookie* cookie = _GetCookie(jar, url, "006");
	CPPUNIT_ASSERT(cookie != NULL);
	CPPUNIT_ASSERT(cookie->Value() == bigdata);
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
		BNetworkCookie* cookie = _GetCookie(jar, url, cookieString.String());
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
	BNetworkCookie* cookie = _GetCookie(jar, url, "008");
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
	BNetworkCookie* cookie = _GetCookie(jar, url, "010");
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
	BNetworkCookie* cookie = _GetCookie(jar, url, "011");
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
	BNetworkCookie* cookie = _GetCookie(jar, url, "012-1");
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

	BNetworkCookie* cookie = _GetCookie(jar, url, "013-1");
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
		BNetworkCookie* cookie = it.Next();
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

	parent.addTest("CookieTest", &suite);
}


BNetworkCookie*
CookieTest::_GetCookie(BNetworkCookieJar& jar, const BUrl& url,
	const char* name)
{
	BNetworkCookieJar::UrlIterator it = jar.GetUrlIterator(url);
	BNetworkCookie* result = NULL;

	while (it.HasNext()) {
		BNetworkCookie* cookie = it.Next();
		if (cookie->Name() == name) {
			// Make sure the cookie is found only once.
			CPPUNIT_ASSERT(result == NULL);

			result = cookie;
		}
	}

	return result;
}
