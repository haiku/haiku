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
	area_info ai;
	char *testName;
	void *foo;

	testName="vmInterface construction";
	try { vm = new vmInterface (30); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }

	try {
		testName="getAreaByName with NULL area test";
		if (vm->getAreaByName(NULL)>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		testName="getAreaByName with unknown area name test";
		if (vm->getAreaByName("MOO COW")>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		testName="getAreaByName with existant area test";
		int areaNum,found;
		if ((areaNum=vm->createArea("test",1,&foo))<0) 
			throw (testName); 
		if ((found=vm->getAreaByName("test"))==areaNum) 
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
