#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface *vm;

void writeByte(unsigned long addr,unsigned int offset, char value) { vm->setByte(addr+offset,value); }
unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm->getByte(addr+offset); return value; }

int main(int argc,char **argv)
{
	try {
	vm = new vmInterface (30);
	error ("Starting Threads!\n");
	
	int myfd=open("mmap_test_1_file",O_RDWR|O_CREAT,0707);
	error ("error = %s\n",strerror(errno));
	/*
	close (myfd);
	return 0;
	*/
	unsigned long result=(unsigned long)(vm->mmap(NULL,16384,PROT_WRITE|PROT_READ,MAP_SHARED,myfd,0));
	for (int i=0;i<16384;i++)
		writeByte(result,i,'A');
	for (int i=0;i<16384;i++)
		if ('A' != readByte(result,i))
			error ("Non matching byte %c at offset %d");
	vm->munmap((void *)result,16384);
	close (myfd);
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
