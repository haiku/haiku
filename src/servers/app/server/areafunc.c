#include <OS.h>
#include "bget.h"
#include <stdio.h>

void * expand_area_storage(long size);
void contract_area_storage(void *buffer);

void set_area_buffer_management(void)
{
	bectl(NULL, expand_area_storage, contract_area_storage, B_PAGE_SIZE);
}

void * expand_area_storage(long size)
{
	long areasize=0;
	area_id a;
	int *parea;
	
	if(size<B_PAGE_SIZE)
		areasize=B_PAGE_SIZE;
	else
	{
		if((size % B_PAGE_SIZE)!=0)
			areasize=((long)(size/B_PAGE_SIZE)+1)*B_PAGE_SIZE;
		else
			areasize=size;
	}
	a=create_area("bitmap_area",(void**)&parea,B_ANY_ADDRESS,areasize,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(a==B_BAD_VALUE ||
	   a==B_NO_MEMORY ||
	   a==B_ERROR)
	{
	   	printf("PANIC: BitmapManager couldn't allocate area!!\n");
	   	return NULL;
	}

	return parea;
}

void contract_area_storage(void *buffer)
{
	area_id trash=area_for(buffer);

	if(trash==B_ERROR)
		return;
	delete_area(trash);
}
