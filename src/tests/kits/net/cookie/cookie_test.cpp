#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <iostream>

#include <NetworkKit.h>
#include <SupportKit.h>

#ifdef ASSERT
#undef ASSERT
#endif

#define REPORT(i, assert, line) cout << "ASSERT() failed at line " << line \
									<< ": " <<  #assert << " (test #" << i << ")" << endl
#define ASSERT(index, assertion) { if (!(assertion)) { REPORT(index, assertion, __LINE__ ); } }


using std::cout;
using std::endl;


typedef struct {
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
} ExplodeTest;


ExplodeTest kTestExplode[] =
	//     Cookie string      URL
	//     ------------- -------------
	//		   Valid     Name     Value	       Domain         Path      Secure  HttpOnly Session  Expiration
	//       --------- -------- --------- ----------------- ---------  -------- -------- -------  ----------
	{
		// Normal cookies
		{ "name=value", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		{ "name=value; domain=example.com; path=/; secure", "http://www.example.com/path/path",
			{  true,    "name",  "value",   "example.com",   "/"    ,   true,    false,   true,   BDateTime() } },
		{ "name=value; httponly; secure", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   true,    true,    true,   BDateTime() } },
		{ "name=value; expires=Wed, 20 Feb 2013 20:00:00 UTC", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   false,   false,   false,
				BDateTime(BDate(2012, 2, 20), BTime(20, 0, 0, 0)) } },
		// Valid cookie with bad form
		{ "name=  ;  domain   =example.com  ;path=/;  secure = yup  ; blahblah ;)", "http://www.example.com/path/path",
			{  true,    "name",  "",   "example.com",   "/"    ,   true,    false,   true,   BDateTime() } },
		// Invalid path, default path should be used instead
		{ "name=value; path=invalid", "http://www.example.com/path/path",
			{  true,    "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		// Setting for other subdomain (invalid)
		{ "name=value; domain=subdomain.example.com", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		// Various invalid cookies
		{ "name", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } },
		{ "; domain=example.com", "http://www.example.com/path/path",
			{  false,   "name",  "value", "www.example.com", "/path",   false,   false,   true,   BDateTime() } }
	};


void explodeImplodeTest()
{
	uint32 testIndex;
	BNetworkCookie cookie;

	for (testIndex = 0; testIndex < (sizeof(kTestExplode) / sizeof(ExplodeTest)); testIndex++)
	{
		BUrl url(kTestExplode[testIndex].url);
		cookie.ParseCookieStringFromUrl(kTestExplode[testIndex].cookieString, url);

		ASSERT(testIndex, kTestExplode[testIndex].expected.valid == cookie.IsValid());

		if (kTestExplode[testIndex].expected.valid) {
			ASSERT(testIndex, BString(kTestExplode[testIndex].expected.name) == cookie.Name());
			ASSERT(testIndex, BString(kTestExplode[testIndex].expected.value) == cookie.Value());
			ASSERT(testIndex, BString(kTestExplode[testIndex].expected.domain) == cookie.Domain());
			ASSERT(testIndex, BString(kTestExplode[testIndex].expected.path) == cookie.Path());
			ASSERT(testIndex, kTestExplode[testIndex].expected.secure == cookie.Secure());
			ASSERT(testIndex, kTestExplode[testIndex].expected.httponly == cookie.HttpOnly());
			ASSERT(testIndex, kTestExplode[testIndex].expected.session == cookie.IsSessionCookie());

			if (!cookie.IsSessionCookie())
				ASSERT(testIndex, kTestExplode[testIndex].expected.expire.Time_t() == cookie.ExpirationDate());
		}
	}
}


void stressTest(int32 domainNumber, int32 totalCookies, char** flat, ssize_t* size)
{
	char **domains = new char*[domainNumber];

	cout << "Creating random domains" << endl;
	srand(time(NULL));
	for (int32 i = 0; i < domainNumber; i++) {
		int16 charNum = (rand() % 16) + 1;

		domains[i] = new char[charNum + 5];

		// Random domain
		for (int32 c = 0; c < charNum; c++)
			domains[i][c] = (rand() % 26) + 'a';

		domains[i][charNum] = '.';

		// Random tld
		for (int32 c = 0; c < 3; c++)
			domains[i][charNum+1+c] = (rand() % 26) + 'a';

		domains[i][charNum+4] = 0;
	}

	BNetworkCookieJar j;
	BStopWatch* watch = new BStopWatch("Cookie insertion");
	for (int32 i = 0; i < totalCookies; i++) {
		BNetworkCookie c;
		int16 domain = (rand() % domainNumber);
		BString name("Foo");
		name << i;

		c.SetName(name);
		c.SetValue("Bar");
		c.SetDomain(domains[domain]);
		c.SetPath("/");

		j.AddCookie(c);
	}
	delete watch;

	BNetworkCookie* c;
	int16 domain = (rand() % domainNumber);
	BString host("http://");
	host << domains[domain] << "/";

	watch = new BStopWatch("Cookie filtering");
	BUrl url(host);
	int32 count = 0;
	for (BNetworkCookieJar::UrlIterator it(j.GetUrlIterator(url)); (c = it.Next()); ) {
	//for (BNetworkCookieJar::Iterator it(j.GetIterator()); c = it.Next(); ) {
		count++;
	}
	delete watch;
	cout << "Count for " << host << ": " << count << endl;


	cout << "Flat view of cookie jar is " << j.FlattenedSize() << " bytes large." << endl;
	*flat = new char[j.FlattenedSize()];
	*size = j.FlattenedSize();

	if (j.Flatten(*flat, j.FlattenedSize()) == B_OK)
		cout << "Flatten() success!" << endl;
	else
		cout << "Flatten() error!" << endl;

	delete[] domains;
}


int
main(int, char**)
{
	cout << "Running explodeImplodeTest:" << endl;
	explodeImplodeTest();
	cout << endl << endl;

	cout << "Running stressTest:" << endl;
	char* flatJar;
	ssize_t size;
	stressTest(10000, 40000, &flatJar, &size);

	BNetworkCookieJar j;
	j.Unflatten(B_ANY_TYPE, flatJar, size);

	int32 count = 0;
	BNetworkCookie* c;
	for (BNetworkCookieJar::Iterator it(j.GetIterator()); (c = it.Next()); )
		count++;
	cout << "Count : " << count << endl;

	delete[] flatJar;

	return EXIT_SUCCESS;
}
