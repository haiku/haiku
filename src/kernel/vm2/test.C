#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface vm(30);

void writeByte(unsigned long addr,unsigned int offset, char value) { vm.setByte(addr+offset,value); }

unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm.getByte(addr+offset); return value; }

int createFillAndTest(int pages,char *name)
{
	try{
	unsigned long addr;
	int area1;
	printf ("%s: createFillAndTest: about to create \n",name);
	area1=vm.createArea("Mine",pages,(void **)(&addr));
	printf ("%s: createFillAndTest: create done\n",name);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		{				
		if (!(i%100) )
			printf ("Writing to byte %d of area %s\n",i,name);
		writeByte(addr,i,i%256);
		}
	printf ("%s: createFillAndTest: writing done\n",name);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		{
		if (!(i%100) )
			printf ("Reading from byte %d of area %s\n",i,name);
		if (i%256!=readByte(addr,i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(addr,i));
		}
	printf ("%s: createFillAndTest: reading done\n",name);
	return area1;
	}
	catch (...)
	{
		printf ("Exception thrown!\n");
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
	printf ("Starting Loop Test!\n");
	loopTestParameters *params=((loopTestParameters *)parameters);
	int area1;

	while (1)
		{
		snooze(params->initialSnooze);
		printf ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize,params->name);
		snooze(params->holdSnooze);
		printf ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}

int32 getInfoTest(void *parameters)
	{
	loopTestParameters *params=((loopTestParameters *)parameters);
	area_info ai;	
	int area1;

	while (1)
		{
		snooze(params->initialSnooze);
		//printf ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize,params->name);
		snooze(params->holdSnooze);
		
		vm.getAreaInfo(area1,&ai);
		printf ("Area info : \n");
		printf ("name : %s\n",ai.name);
		printf ("size : %ld\n",ai.size);
		printf ("lock : %ld\n",ai.lock);
		printf ("team : %ld\n",ai.team);
		printf ("ram_size : %ld\n",ai.ram_size);
		printf ("in_count : %ld\n",ai.in_count);
		printf ("out_count : %ld\n",ai.out_count);
		printf ("copy_count : %ld\n",ai.copy_count);
		printf ("address : %p\n",ai.address);

		printf ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}

int32 mmapTest (void *parameters)
	{
	void *map;
	loopTestParameters *params=((loopTestParameters *)parameters);
	int size=params->areaSize; // Note that this is in bytes, not in pages
	while (1)
		{
		int fd = open ("OBOS_mmap",O_RDWR|O_CREAT,0x777);
		printf ("Opened file, fd = %d\n",fd);
		snooze(params->initialSnooze);
		printf ("Creating %s mmap\n",params->name);
		snooze(params->holdSnooze);
		map=vm.mmap(NULL,size,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
		printf ("mmap address = %p\n",map);
		for (int i=0;i<size;i++)
			writeByte((int32)map,i,i%256);
		printf ("mmapTest: writing done\n");
		for (int i=0;i<size;i++)
			if (i%256!=readByte((int32)map,i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte((int32)map,i));
		printf ("mmapTest: reading done\n");
		snooze(params->loopSnooze); 
		vm.munmap(map,size);
		close(fd);
		printf ("Closed file, fd = %d\n",fd);
		}
	}

int32 cloneTest (void *parameters)
	{
	loopTestParameters *params=((loopTestParameters *)parameters);
	int area1,area2;
	void *cloneAddr=NULL;

	while (1)
		{
		snooze(params->initialSnooze);
	//	printf ("Creating %s area, size = %d\n",params->name,params->areaSize);
		area1=createFillAndTest(params->areaSize,params->name);
	//	printf ("cloning, create done \n");
		area2=vm.cloneArea(area1,"Clone1",&cloneAddr);
		for (int i=0;i<params->areaSize*PAGE_SIZE;i++)
			if (i%256!=readByte((int32)cloneAddr,i))
				printf ("ERROR! Clone Byte at offset %d of %p does not match: expected: %d, found: %d\n",i,cloneAddr,i%256,readByte((int32)cloneAddr,i));
	//	printf ("Snoozing, compare done \n");
		snooze(params->holdSnooze);
	//	printf ("Freeing %s area\n",params->name);
		vm.freeArea(area2);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
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

	resume_thread(spawn_thread(loopTest,"area test 1",0,&area1Params));
	resume_thread(spawn_thread(loopTest,"area test 2",0,&area2Params));
	resume_thread(spawn_thread(loopTest,"area test 3",0,&area3Params));
	//resume_thread(spawn_thread(getInfoTest,"info test 1",0,&info1Params));
	//resume_thread(spawn_thread(mmapTest,"mmap test 1",0,&mmap1Params));
	//resume_thread(spawn_thread(cloneTest,"clone test 1",0,&clone1Params));

	snooze(1000000000);

	return 0;
}
