#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>
#include <OS.h>

vmInterface *vm;

int main(int argc,char **argv)
{
	void *location;
	char *testName;

	location=NULL;
	testName="vmInterface construction";
	try { vm = new vmInterface (30); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }

	testName="NULL name vm->createArea test";
	try { if (vm->createArea(NULL,1,&location)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="HUGE name vm->createArea test";
	try { if (vm->createArea("This is a crazy, way too long name that should never, ever be permited to work at any time and under any circumstances!",1,&location)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="0 page count vm->createArea test";
	try { if (vm->createArea("test",0,&location)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="page count larger than VM vm->createArea test";
	try { if (vm->createArea("test",131,&location)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }
	error ("Previous test is expected to fail, until sizing of VM file is worked out\n");

	testName="page count larger than physical memory vm->createArea test";
	try { if (vm->createArea("test",31,&location,ANY,FULL)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="clone vm->createArea test";
	try { if (vm->createArea("test",1,&location,CLONE)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	catch (const char *t) {
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...) {
		error ("Unknown Exception thrown!\n");
		exit(1);
	}

	return 0;
}
