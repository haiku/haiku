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
	int areaNum,found;
	char *testName;
	void *foo;

	testName="vmInterface construction";
	try { vm = new vmInterface (30); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }

	try {
		testName="getAreaByAddress with NULL address test";
		if (vm->getAreaByAddress(NULL)>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		if ((areaNum=vm->createArea("test",1,&foo))<0) 
			throw (testName); 

		testName="getAreaByAddress with out of bounds address test";
		if (vm->getAreaByAddress((void *)(((long)foo)+100000))>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		testName="getAreaByAddress with existant area test";
		if ((found=vm->getAreaByAddress((void *)(((long)foo)+2)))==areaNum) 
			error ("%s passed!\n",testName); 
		else
			error ("%s failed! areaNum = %d, found = %d \n",testName,areaNum,found); 
		}
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
