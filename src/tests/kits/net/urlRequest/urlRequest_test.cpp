#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <KernelKit.h>
#include <NetworkKit.h>

using std::cout;
using std::endl;

int
main(int, char**)
{	
	BUrl yahoo("http://www.yahoo.fr");
	BUrlContext c;
	BUrlRequest t(yahoo);

	t.SetContext(&c);

	if (!t.InitCheck()) {
		cout << "URL request failed to initialize" << endl;
		return EXIT_FAILURE;
	}

	if (t.Perform() != B_OK) {
		cout << "Error while performing request!" << endl;
		return EXIT_FAILURE;
	}

	// Do nothing while the request is not finished
	while (t.IsRunning()) {
		cout << std::flush;
		snooze(10000);
	}

	// Print the result
	cout << "Request result : " << t.Result().StatusCode() << " (" << t.Result().StatusText() << ")" << endl;
	//cout << "  * " << c.GetCookieJar().CountCookies() << " cookies in context after request" << endl;
	cout << "  * " << t.Result().Headers().CountHeaders() << " headers received" << endl;
	cout << "  * " << t.Result().RawData().Position() << " bytes of raw data:" << endl;
	cout << t.Result() << endl;
	cout << "End of request" << endl << endl;

	// Flat view of the cookie jar :
	cout << "cookie.txt :" << endl;

	ssize_t flatSize = c.GetCookieJar().FlattenedSize();
	char *flatCookieJar = new char[flatSize];

	c.GetCookieJar().Flatten(flatCookieJar, flatSize);

	cout << flatCookieJar << endl << endl;

	return EXIT_SUCCESS;
}
