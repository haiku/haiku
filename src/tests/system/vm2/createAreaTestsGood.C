#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>
#include <OS.h>

vmInterface *vm;
/*  - specified address (EXACT)
	- base address (BASE)
	- kernel address (ANY_KERNEL)
	- FULL_LOCK
	- CONTIGUOUS
	- LAZY_LOCK
	- NO_LOCK
	- LOMEM
	*/

static void *orig;
void assignAddress(unsigned long a, void *&b) {b=(void *)a;orig=b;}

int main(int argc,char **argv)
{
	void *location;
	char *testName;

	location=NULL;
	testName="vmInterface construction";
	try { vm = new vmInterface (60); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }

	testName="Specified Address succeeds";
	try {assignAddress(0x14000000,location) ; if (vm->createArea(testName,1,&location,EXACT)<0) throw (testName); 
			if (location != orig) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

	testName="Specified Address fails"; // Because we want the same space
	try {assignAddress(0x14000000,location) ; if (vm->createArea(testName,1,&location,EXACT)>=0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="Base Address"; 
	try {assignAddress(0x12000000,location) ; if (vm->createArea(testName,1,&location,EXACT)<0) throw (testName); 
			if (location != orig) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="Kernel Address"; 
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY_KERNEL)<0) throw (testName); 
			if (location != orig) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); }

	testName="Specified Address ";// This needs more and better testing
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY,FULL)<0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

	testName="Contiguous Memory "; // This needs more and better testing
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY,CONTIGUOUS)<0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

	testName="Lazy Lock "; // This needs more and better testing
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY,LAZY)<0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

	testName="No Lock "; // This needs more and better testing
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY,NO_LOCK)<0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

	testName="LOMEM "; // This needs more and better testing
	try {assignAddress(0,location) ; if (vm->createArea(testName,1,&location,ANY,LOMEM)<0) throw (testName); else error ("%s passed!\n",testName); }
	catch (const char *t) { error ("Exception thrown in %s!\n",t); exit(1);}

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
