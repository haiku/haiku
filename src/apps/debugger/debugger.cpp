/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

//#include "DwarfManager.h"


int
main(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage:...\n");
		return 1;
	}

	const char* fileName = argv[1];

//	DwarfManager manager;
//	manager.LoadFile(fileName);
//	manager.FinishLoading();
(void)fileName;

	return 0;
}
