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
	int areaNum;
	char *testName;
	void *foo;
	unsigned long baseAddress;

	testName="vmInterface construction";
	try { vm = new vmInterface (30); }
	catch (const char *t) { error ("Exception thrown in %s! %s\n",testName,t); exit(1); }

	try {
		testName="resizeArea with unknown area test";
		if (vm->resizeArea(54,8192)>=0) 
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		if ((areaNum=vm->createArea("test",1,&foo))<0) 
			throw (testName); 
		baseAddress=(unsigned long)foo;

		testName="initial test sizes of area";
		try {vm->setInt(baseAddress-1,25); error ("%s lower bounds test failed!\n",testName); } catch (char *t) {error ("%s lower bounds test passed!\n",testName);}
		try {vm->setInt(baseAddress+8192,25); error ("%s upper bounds test failed!\n",testName);} catch (char *t) {error ("%s upper bounds test passed!\n",testName);}
			
		testName="enlarging size of area";
		if (vm->resizeArea(areaNum,16384)!=B_OK)
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		testName=" test enlarged sizes of area";
		try {vm->setInt(baseAddress-1,25); error ("%s lower bounds test failed!\n",testName); } catch (char *t) {error ("%s lower bounds test passed!\n",testName);}
		try {vm->setInt(baseAddress+8192,25); error ("%s middle bounds test passed!\n",testName);} catch (char *t) {error ("%s middle bounds test failed!\n",testName);}
		try {vm->setInt(baseAddress+16385,25); error ("%s upper bounds test failed!\n",testName);} catch (char *t) {error ("%s upper bounds test passed!\n",testName);}
		
		testName="reducing size of area";
		if (vm->resizeArea(areaNum,4096)!=B_OK)
			error ("%s failed!\n",testName); 
		else
			error ("%s passed!\n",testName); 

		testName=" test enlarged sizes of area";
		try {vm->setInt(baseAddress-1,25); error ("%s lower bounds test failed!\n",testName); } catch (char *t) {error ("%s lower bounds test passed!\n",testName);}
		try {vm->setInt(baseAddress+4097,25); error ("%s upper bounds test failed!\n",testName);} catch (char *t) {error ("%s upper bounds test passed!\n",testName);}
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
