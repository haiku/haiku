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


typedef struct
{
	const char* cookieString;

	struct
	{
		const char* name;
		const char* value;
		const char* domain;
		const char* path;
		bool 		secure;
		bool 		discard;
		bool		session;
		int32 		maxAge;
	} expected;
} ExplodeTest;

const ExplodeTest	kTestExplode[] =
	//  Cookie string
	//		    Name	 Value	  Domain     Path   Secure   Discard  Session   maxAge
	//       -------- --------- --------- --------- ------   -------  -------- -------
	{
		{ "name=value",
			 { "name", "value",      "",       "",  false,  false,    true,     0 } },
		{ "name=value;secure=true",
			 { "name", "value",      "",       "",   true,  false,    true,     0 } },
		{ "name=value;secure=false;maxage=5",
			 { "name", "value",      "",       "",  false,  false,    false,    5 } },
		{ "name=value;discard=true",
			 { "name", "value",      "",       "",  false,  true,    true,     0 } },
	};


void explodeImplodeTest()
{
	uint8 testIndex;
	BNetworkCookie cookie;

	for (testIndex = 0; testIndex < (sizeof(kTestExplode) / sizeof(ExplodeTest)); testIndex++)
	{
		cookie.ParseCookieString(kTestExplode[testIndex].cookieString);

		ASSERT(testIndex, BString(kTestExplode[testIndex].expected.name) == BString(cookie.Name()));
		ASSERT(testIndex, BString(kTestExplode[testIndex].expected.value) == BString(cookie.Value()));
		ASSERT(testIndex, BString(kTestExplode[testIndex].expected.domain) == BString(cookie.Domain()));
		ASSERT(testIndex, BString(kTestExplode[testIndex].expected.path) == BString(cookie.Path()));
		ASSERT(testIndex, kTestExplode[testIndex].expected.secure == cookie.Secure());
		ASSERT(testIndex, kTestExplode[testIndex].expected.discard == cookie.Discard());
		ASSERT(testIndex, kTestExplode[testIndex].expected.session == cookie.IsSessionCookie());

		if (!cookie.IsSessionCookie())
			ASSERT(testIndex, kTestExplode[testIndex].expected.maxAge == cookie.MaxAge());
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
