/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <stdio.h>
#include <stdlib.h>

#define NUM_COLUMNS 16

int main(int argc, char **argv)
{
	FILE *infp = stdin;
	char c;
	int column = 0;

	while(!feof(infp)) {
		int err;
		err = fread(&c, sizeof(c), 1, infp);
		if(err != 1)
			break;

		printf("0x%02x,", ((int)c) & 0xff);
		if((++column % NUM_COLUMNS) == 0) {
			printf("\n");
		}
	}

	return 0;
}

