#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface *vm;

void writeByte(unsigned long addr,unsigned int offset, char value) { vm->setByte(addr+offset,value); }
unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm->getByte(addr+offset); return value; }

int createFillAndTest(int pages,char *name)
{
	unsigned long addr;
	int area1;
	error ("%s: createFillAndTest: about to create \n",name);
	area1=vm->createArea(name,pages,(void **)(&addr));
	error ("%s: createFillAndTest: create done\n",name);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		{				
		if (!(i%9024) )
			error ("Writing to byte %d of area %s\n",i,name);
		writeByte(addr,i,i%256);
		}
	error ("%s: createFillAndTest: writing done\n",name);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		{
		if (!(i%9024) )
			error ("Reading from byte %d of area %s\n",i,name);
		if (i%256!=readByte(addr,i))
				error ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(addr,i));
		}
	vm->freeArea(area1);
	error ("%s: createFillAndTest: reading done\n",name);
	return area1;
}

int main(int argc,char **argv)
{
	try {
	vm = new vmInterface (30);
	error ("Starting Threads!\n");

	for (int i=0;i<20;i++)
		createFillAndTest(1,"myTest");	
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Unknown Exception thrown!\n");
		exit(1);
	}

	return 0;
}
