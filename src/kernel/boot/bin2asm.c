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
	int start = 1;

	while(!feof(infp)) {
		int err;
		err = fread(&c, sizeof(c), 1, infp);
		if(err != 1)
			break;

		if((column % NUM_COLUMNS) == 0) {
			if(!start) {
				printf("\n");
			} else {
				start = 0;
			}
			printf(".byte\t");
		} else {
			printf(",");
		}

		printf("0x%02x", ((int)c) & 0xff);

		column++;
	}
	printf("\n");

	return 0;
}

