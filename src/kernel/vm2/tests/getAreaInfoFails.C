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
		testName="getAreaInfo with non-existant areatest";
		if (vm->getAreaInfo(999999,&ai)>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		int areaNum;
		testName="getAreaInfo with NULL area_info areatest";
		if (areaNum=vm->createArea("test",1,&foo)<0) 
			throw (testName); 
		if (vm->getAreaInfo(areaNum,&ai)>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 
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
