/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include "CommonOptions.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>


CommonOptions::CommonOptions()
	:
	fDebugLevel(0)
{
}


CommonOptions::~CommonOptions()
{
}


bool
CommonOptions::HandleOption(int option)
{
	switch (option) {
		case OPTION_DEBUG:
		{
			char* end;
			fDebugLevel = strtol(optarg, &end, 0);
			if (end == optarg) {
				fprintf(stderr,
					"*** invalid argument for option --debug\n");
				exit(1);
			}
			return true;
		}

		default:
			return false;
	}
}
