#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface vm(20);

void writeByte(unsigned long addr,unsigned int offset, char value) { vm.setByte(addr+offset,value); }

unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm.getByte(addr+offset); return value; }

int createFillAndTest(int pages)
{
	unsigned long addr;
	int area1;
	area1=vm.createArea("Mine",pages,(void **)(&addr));
	for (int i=0;i<pages*PAGE_SIZE;i++)
		writeByte(addr,i,i%256);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		if (i%256!=readByte(addr,i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(addr,i));
	return area1;
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
	loopTestParameters *params=((loopTestParameters *)parameters);
	int area1;

	while (1)
		{
		snooze(params->initialSnooze);
		//printf ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize);
		snooze(params->holdSnooze);
		//printf ("Freeing %s area\n",params->name);
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
		area1=createFillAndTest(params->areaSize);
		snooze(params->holdSnooze);
		
		vm.getAreaInfo(area1,&ai);
		printf ("Area info : \n");
		printf ("name : %s\n",ai.name);
		printf ("size : %d\n",ai.size);
		printf ("lock : %d\n",ai.lock);
		printf ("team : %d\n",ai.team);
		printf ("ram_size : %d\n",ai.ram_size);
		printf ("in_count : %d\n",ai.in_count);
		printf ("out_count : %d\n",ai.out_count);
		printf ("copy_count : %d\n",ai.copy_count);
		printf ("address : %x\n",ai.address);

		printf ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}

int32 mmapTest (void *parameters)
	{
	void *map;

	int fd = open ("OBOS_mmap",O_RDWR|O_CREAT,0x777);
	printf ("Opened file, fd = %d\n",fd);

	loopTestParameters *params=((loopTestParameters *)parameters);
	int size=params->areaSize; // Note that this is in bytes, not in pages
//	while (1)
		{
		snooze(params->initialSnooze);
		printf ("Creating %s mmap\n",params->name);
		snooze(params->holdSnooze);
		map=vm.mmap(NULL,size,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
		printf ("mmap address = %x\n",map);
		for (int i=0;i<size;i++)
			writeByte((int32)map,i,i%256);
		for (int i=0;i<size;i++)
			if (i%256!=readByte((int32)map,i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte((int32)map,i));
		snooze(params->loopSnooze);
		}
		close(fd);
	printf ("Closed file, fd = %d\n",fd);
	}


int main(int argc,char **argv)
{
	loopTestParameters area1Params={"area1",1000000,2,100000,100000};
	loopTestParameters area2Params={"area2",1000000,2,200000,100000};
	loopTestParameters area3Params={"area3",1000000,2,300000,200000};
	loopTestParameters info1Params={"info1",500000,2,400000,30000};
	loopTestParameters mmap1Params={"mmap",500000,8192,400000,1000000};

	//resume_thread(spawn_thread(loopTest,"area test 1",0,&area1Params));
	//resume_thread(spawn_thread(loopTest,"area test 2",0,&area2Params));
	//resume_thread(spawn_thread(loopTest,"area test 3",0,&area3Params));
	//resume_thread(spawn_thread(getInfoTest,"info test 1",0,&info1Params));
	resume_thread(spawn_thread(mmapTest,"mmap test 1",0,&mmap1Params));

	snooze(10000000);

	return 0;
}
