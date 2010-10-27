#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <NetworkKit.h>

#ifdef ASSERT
#undef ASSERT
#endif

#define REPORT(assert, line) cout << "ASSERT() failed at line " << line << ": " <<  #assert << endl
#define ASSERT(assertion) { if (!(assertion)) { REPORT(assertion, __LINE__ ); } }

using namespace std;

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


const char*			kTestLength[] =
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

// Test if rebuild url are the same length of parsed one
void lengthTest()
{
	int8 testIndex;
	BUrl testUrl;

	for (testIndex = 0; kTestLength[testIndex] != NULL; testIndex++)
	{
		testUrl.SetUrlString(kTestLength[testIndex]);

		cout << "`" << kTestLength[testIndex] << "' vs `" << testUrl.UrlString() << "'" << endl;
		ASSERT(strlen(kTestLength[testIndex]) == strlen(testUrl.UrlString()));
	}
}

void explodeImplodeTest()
{
	uint8 testIndex;
	BUrl testUrl;

	for (testIndex = 0; testIndex < (sizeof(kTestExplode) / sizeof(ExplodeTest)); testIndex++)
	{
		testUrl.SetUrlString(kTestExplode[testIndex].url);

		ASSERT(BString(kTestExplode[testIndex].url) == BString(testUrl.UrlString()));
		ASSERT(BString(kTestExplode[testIndex].expected.protocol) == BString(testUrl.Protocol()));
		ASSERT(BString(kTestExplode[testIndex].expected.userName) == BString(testUrl.UserName()));
		ASSERT(BString(kTestExplode[testIndex].expected.password) == BString(testUrl.Password()));
		ASSERT(BString(kTestExplode[testIndex].expected.host) == BString(testUrl.Host()));
		ASSERT(kTestExplode[testIndex].expected.port == testUrl.Port());
		ASSERT(BString(kTestExplode[testIndex].expected.path) == BString(testUrl.Path()));
		ASSERT(BString(kTestExplode[testIndex].expected.request) == BString(testUrl.Request()));
		ASSERT(BString(kTestExplode[testIndex].expected.fragment) == BString(testUrl.Fragment()));
	}
}

using std::cout;
using std::endl;

int
main(int, char**)
{
	cout << "Running lengthTest:" << endl;
	lengthTest();
	cout << endl;

	cout << "Running explodeImplodeTest:" << endl;
	explodeImplodeTest();
	cout << endl;

	BUrl test = "lol";
	cout << "Path: " << test.HasPath() << " -> " << test.Path() << endl;
	cout << "Rebuild: " << test.UrlString() << endl;

	return EXIT_SUCCESS;
}
