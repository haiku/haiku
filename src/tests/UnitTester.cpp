#include <string>
#include <iostream>
#include <unistd.h>

#include <Path.h>

/* Since some of our obos libraries (such as libtranslation.so) have
   identically named R5 equivalents, it was necessary to have two
   separate unit testing programs, each with their own lib/ subdir,
   to make sure that the obos tests are run with obos libs while
   the r5 tests are run with r5 libs.
   
   In the interest of keeping things simple (and not invalidating
   the instructions sitting in the newsletter article I wrote :-),
   this shell program has been created to allow all unit tests to
   still be run from a single application. All it does is filter
   out "-obos" and "-r5" arguments and run the appropriate
   UnitTesterHelper program (which does all the hard work).
*/
int main(int argc, char *argv[]) {
	// Look for "-obos" or "-r5" arguments, then run
	// the appropriate UnitTesterHelper program, letting
	// it do all the dirty work.
	bool doR5Tests = false;
	bool beVerbose = false;
	std::string cmd = "";
	for (int i = 1; i < argc; i++) {
		std::string arg(argv[i]);
		if (arg == "-r5")
			doR5Tests = true;
		else if (arg == "-obos")
			doR5Tests = false;
		else {
			if (arg.length() == 3 && arg[0] == '-' && arg[1] == 'v'
				  && '0' <= arg[2] && arg[2] <= '9')
			{
				beVerbose = (arg[2] - '0') >= 4;
			}
			cmd += " " + arg;
		}
	}
	// get test dir path
	BPath path(argv[0]);
	if (path.InitCheck() == B_OK) {
		if (path.GetParent(&path) != B_OK)
			cout << "Couldn't get test dir." << endl;
	} else
		cout << "Couldn't find the path to the test app." << endl;
	// construct the command path
	cmd = (doR5Tests ? "unittester_r5/UnitTesterHelper_r5" : "unittester/UnitTesterHelper") + cmd;
	cmd = string(path.Path()) + "/" + cmd;
	if (beVerbose)
		cout << "Executing: '" << cmd << "'" << endl;	
	system(cmd.c_str());
	return 0;
}

