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
	int areaNumber;

	location=NULL;
	testName="vmInterface construction";
	try { vm = new vmInterface (30); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }
	try {
	testName="delete_area on invalid area test";
	if (vm->delete_area(999999)==B_OK) 
		error ("%s failed!\n",testName) ;
	else 
		error ("%s passed!\n",testName); 

	testName="delete_area twice on same area test";
	if (areaNumber=vm->createArea("sampleArea",1,&location)>=0) {
		if (vm->delete_area(areaNumber)!=B_OK) error("%s:Initial free failed!\n",testName);
		if (vm->delete_area(areaNumber)==B_OK) 
			error("%s:Second free succeeded!\n",testName);
		else 
			error ("%s passed\n",testName);
		}
	}
	catch (const char *t) { error ("Exception thrown in %s, while trying to run !\n",t,testName); }

	return 0;
}
