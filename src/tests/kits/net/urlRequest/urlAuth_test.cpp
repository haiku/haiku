#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <KernelKit.h>
#include <NetworkKit.h>

#ifdef ASSERT
#undef ASSERT
#endif

#define REPORT(i, assert, line) cout << "ASSERT() failed at line " << line \
									<< ": " <<  #assert << " (test #" << i << ")" << endl
#define ASSERT(index, assertion) { if (!(assertion)) { REPORT(index, assertion, __LINE__ ); } }

using std::cout;
using std::endl;

int
main(int, char**)
{	
	BUrl authTest("http://192.168.1.11/~chris/auth_digest/");
	BUrlRequest t(authTest);
	BUrlContext c;

	t.SetContext(&c);
	
	t.SetProtocolOption(B_HTTPOPT_AUTHUSERNAME, (void*)"haiku");
	t.SetProtocolOption(B_HTTPOPT_AUTHPASSWORD, (void*)"haiku");
	
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
}
