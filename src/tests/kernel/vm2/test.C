#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface vm(30);
thread_id loop1,loop2,loop3,info1,mmap1,clone1;

void writeByte(unsigned long addr,unsigned int offset, char value) { vm.setByte(addr+offset,value); }

unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm.getByte(addr+offset); return value; }

int createFillAndTest(int pages,char *name)
{
	try{
	unsigned long addr;
	int area1;
	error ("%s: createFillAndTest: about to create \n",name);
	area1=vm.createArea("Mine",pages,(void **)(&addr));
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
	error ("%s: createFillAndTest: reading done\n",name);
	return area1;
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Exception thrown!\n");
		exit(1);
	}
	return 0;
}

struct loopTestParameters
{
	char *name;
	bigtime_t initialSnooze;
	int areaSize;
	bigtime_t holdSnooze;
	bigtime_t loopSnooze;
};

int32 loopTest(void *parameters)
	{
	try{
	error ("Starting Loop Test!\n");
	loopTestParameters *params=((loopTestParameters *)parameters);
	int area1;

	while (1)
		{
		snooze(params->initialSnooze);
		error ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize,params->name);
		snooze(params->holdSnooze);
		error ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Exception thrown!\n");
		exit(1);
	}
	}

int32 getInfoTest(void *parameters)
	{
	loopTestParameters *params=((loopTestParameters *)parameters);
	area_info ai;	
	int area1;

	try{
	while (1)
		{
		snooze(params->initialSnooze);
		//error ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize,params->name);
		snooze(params->holdSnooze);
		
		vm.getAreaInfo(area1,&ai);
		error ("Area info : \n");
		error ("name : %s\n",ai.name);
		error ("size : %ld\n",ai.size);
		error ("lock : %ld\n",ai.lock);
		error ("team : %ld\n",ai.team);
		error ("ram_size : %ld\n",ai.ram_size);
		error ("in_count : %ld\n",ai.in_count);
		error ("out_count : %ld\n",ai.out_count);
		error ("copy_count : %ld\n",ai.copy_count);
		error ("address : %p\n",ai.address);

		error ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Exception thrown!\n");
		exit(1);
	}
	}

int32 mmapTest (void *parameters)
	{
	try{
	void *map;
	loopTestParameters *params=((loopTestParameters *)parameters);
	int size=params->areaSize; // Note that this is in bytes, not in pages
	while (1)
		{
		int fd = open ("OBOS_mmap",O_RDWR|O_CREAT,0x777);
		error ("Opened file, fd = %d\n",fd);
		snooze(params->initialSnooze);
		error ("Creating %s mmap\n",params->name);
		snooze(params->holdSnooze);
		map=vm.mmap(NULL,size,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
		error ("mmap address = %p\n",map);
		for (int i=0;i<size;i++)
			writeByte((int32)map,i,i%256);
		error ("mmapTest: writing done\n");
		for (int i=0;i<size;i++)
			if (i%256!=readByte((int32)map,i))
				error ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte((int32)map,i));
		error ("mmapTest: reading done\n");
		snooze(params->loopSnooze); 
		vm.munmap(map,size);
		close(fd);
		error ("Closed file, fd = %d\n",fd);
		}
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Exception thrown!\n");
		exit(1);
	}
	}

int32 cloneTest (void *parameters)
	{
	loopTestParameters *params=((loopTestParameters *)parameters);
	int area1,area2;
	void *cloneAddr=NULL;
	try {
	while (1)
		{
		snooze(params->initialSnooze);
	//	error ("Creating %s area, size = %d\n",params->name,params->areaSize);
		area1=createFillAndTest(params->areaSize,params->name);
	//	error ("cloning, create done \n");
		area2=vm.cloneArea(area1,"Clone1",&cloneAddr);
		for (int i=0;i<params->areaSize*PAGE_SIZE;i++)
			if (i%256!=readByte((int32)cloneAddr,i))
				error ("ERROR! Clone Byte at offset %d of %p does not match: expected: %d, found: %d\n",i,cloneAddr,i%256,readByte((int32)cloneAddr,i));
	//	error ("Snoozing, compare done \n");
		snooze(params->holdSnooze);
	//	error ("Freeing %s area\n",params->name);
		vm.freeArea(area2);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}
	catch (...)
	{
		error ("Exception thrown!\n");
		exit(1);
	}
	}

int main(int argc,char **argv)
{
	loopTestParameters area1Params={"area1",1000000,2,100000,100000};
	loopTestParameters area2Params={"area2",1000000,2,200000,100000};
	loopTestParameters area3Params={"area3",1000000,2,300000,200000};
	loopTestParameters info1Params={"info1",500000,2,400000,30000};
	loopTestParameters mmap1Params={"mmap",500000,8192,400000,1000000};
	loopTestParameters clone1Params={"clone1",200000,2,300000,400000};

	error ("Starting Threads!\n");
	//resume_thread(loop1=spawn_thread(loopTest,"area test 1",0,&area1Params));
	//resume_thread(loop2=spawn_thread(loopTest,"area test 2",0,&area2Params));
	//resume_thread(loop3=spawn_thread(loopTest,"area test 3",0,&area3Params));
	resume_thread(info1=spawn_thread(getInfoTest,"info test 1",0,&info1Params));
	//resume_thread(mmap1=spawn_thread(mmapTest,"mmap test 1",0,&mmap1Params));
	//resume_thread(clone1=spawn_thread(cloneTest,"clone test 1",0,&clone1Params));

	snooze(1000000000);

	return 0;
}
