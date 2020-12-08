/*
 * Copyright 2016-2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

#include <stdio.h>
#include <string.h>

#include "MediaPlay.h"
#include "MediaTest.h"


void print_usage()
{
	printf("Usage:\n");
	printf("  media_client play <uri>\n");
	printf("  media_client test\n");
}


int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage();
		return 0;
	}

	BApplication app("application/x-vnd.Haiku-media_client");

	int ret = 0;
	if (strcmp(argv[1], "play") == 0) {
		if (argc < 3) {
			print_usage();
		} else
			ret = media_play(argv[2]);
	} else if (strcmp(argv[1], "test") == 0)
		media_test();
	else
		print_usage();

	return ret;
}
