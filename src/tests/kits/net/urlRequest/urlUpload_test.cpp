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
	BUrl google("http://192.168.1.11/~chris/post.php");
	BUrlSynchronousRequest t(google);
	BHttpForm f;
	
	f.AddString("hello", "world");	
	if (f.AddFile("_uploadfile", BPath("libservices.so")) != B_OK) {
		cout << "File upload failed" << endl;
		return EXIT_FAILURE;
	}
	
	// Print form
	for (BHttpForm::Iterator it(f.GetIterator()); BHttpFormData* element = it.Next(); ) {
		cout << element->Name() << ": " << element->Type() << endl;
	}
	
	if (!t.InitCheck()) {
		cout << "URL request failed to initialize" << endl;
		return EXIT_FAILURE;
	}
	
	// Inject form in request
	bool discard = true;
	t.SetProtocolOption(B_HTTPOPT_DISCARD_DATA, &discard);
	t.SetProtocolOption(B_HTTPOPT_POSTFIELDS, &f);

	if (t.Perform() != B_OK) {
		cout << "Error while performing request!" << endl;
		return EXIT_FAILURE;
	}
	
	t.WaitUntilCompletion();
	
	cout << "Request finished!" << endl;	
	cout << t.Result() << endl;
	
	return EXIT_SUCCESS;
}
