#include "vmInterface.h"
#include <stdio.h>

vmInterface vm(10);

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
		printf ("Creating %s area\n",params->name);
		area1=createFillAndTest(params->areaSize);
		snooze(params->holdSnooze);
		printf ("Freeing %s area\n",params->name);
		vm.freeArea(area1);
		snooze(params->loopSnooze);
		}
	}

int main(int argc,char **argv)
{
	loopTestParameters area1Params={"area1",1000000,2,100000,100000};
	loopTestParameters area2Params={"area2",1000000,2,200000,100000};
	loopTestParameters area3Params={"area3",1000000,2,300000,200000};

	resume_thread(spawn_thread(loopTest,"area test 1",0,&area1Params));
	resume_thread(spawn_thread(loopTest,"area test 2",0,&area2Params));
	resume_thread(spawn_thread(loopTest,"area test 3",0,&area3Params));

	snooze(50000000);

	return 0;
}
