#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <iostream>

#include <NetworkKit.h>
#include <SupportKit.h>


using std::cout;
using std::endl;


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

	const BNetworkCookie* c;
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
	cout << "Running stressTest:" << endl;
	char* flatJar;
	ssize_t size;
	stressTest(10000, 40000, &flatJar, &size);

	BNetworkCookieJar j;
	j.Unflatten(B_ANY_TYPE, flatJar, size);

	int32 count = 0;
	const BNetworkCookie* c;
	for (BNetworkCookieJar::Iterator it(j.GetIterator()); (c = it.Next()); )
		count++;
	cout << "Count : " << count << endl;

	delete[] flatJar;

	return EXIT_SUCCESS;
}
