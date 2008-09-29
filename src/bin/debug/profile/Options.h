/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>

#include <OS.h>


struct Options {
	Options()
		:
		interval(1000),
		stack_depth(5),
		output(NULL),
		profile_kernel(true),
		profile_loading(false),
		profile_teams(true),
		profile_threads(true),
		analyze_full_stack(false)
	{
	}

	bigtime_t	interval;
	int32		stack_depth;
	FILE*		output;
	bool		profile_kernel;
	bool		profile_loading;
	bool		profile_teams;
	bool		profile_threads;
	bool		analyze_full_stack;
};


extern Options gOptions;
extern const char* kCommandName;



#endif	// OPTIONS_H

