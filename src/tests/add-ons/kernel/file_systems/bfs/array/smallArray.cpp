#include "Utility.h"

#include <List.h>
#include <stdlib.h>
#include <stdio.h>


int32 gIterations = 1000000;
int32 gNumber = 8;


int
main(int argc,char **argv)
{
	char buffer[1024];
	sorted_array *array = (sorted_array *)buffer;
	array->count = 0;

	srand(42);

	for (int32 i = 0;i < gIterations;i++) {
		// add entries to the array

		for (int32 num = 0;num < gNumber;num++)
			array->Insert(rand());

		for (int32 num = 0;num < gNumber;num++) {
			int32 index = int32(1.0 * rand() * array->count / RAND_MAX);
			array->Remove(array->values[index]);
		}
	}
	return 0;
}
