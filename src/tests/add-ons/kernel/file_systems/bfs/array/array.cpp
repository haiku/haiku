#include "Utility.h"

#include <List.h>
#include <stdlib.h>
#include <stdio.h>


const int32 kBlockSize = 1024;

int32 gNumber = 100000;
BList gList;
BlockArray gArray(kBlockSize);


void
dumpArray()
{
	printf("  items in array: %ld\n",gArray.CountItems());
	printf("  blocks used:    %ld\n",gArray.BlocksUsed());
	printf("  size:           %ld\n",gArray.Size());
}


int
main(int argc,char **argv)
{
	srand(42);
	
	// insert numbers in the array

	for (int32 i = 0;i < gNumber;i++) {
		int32 num = rand();
		if (num == 0)
			num++;

		if (gArray.Insert(num) == B_OK)
			gList.AddItem((void *)num);
		else if (gArray.Find(num) < 0) {
			printf("Could not insert entry in array, but it's not in there either...\n");
			dumpArray();
		} else
			printf("hola\n");
	}
	
	// check for numbers in the array
	
	for (int32 i = 0;i < gNumber;i++) {
		int32 num = (int32)gList.ItemAt(i);

		if (gArray.Find(num) < 0) {
			printf("could not found entry %ld in array!\n",num);
			dumpArray();
		}
	}
	
	// iterate through the array
	
	sorted_array *array = gArray.Array();
	for (int32 i = 0;i < array->count;i++) {
		if (!gList.HasItem((void *)array->values[i])) {
			printf("Could not find entry %Ld at %ld in list!\n",array->values[i],i);
			dumpArray();
		}
	}
	return 0;
}
