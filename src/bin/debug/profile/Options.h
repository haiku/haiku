/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
		callgrind_directory(NULL),
		profile_all(false),
		profile_kernel(true),
		profile_loading(false),
		profile_teams(true),
		profile_threads(true),
		analyze_full_stack(false),
		summary_result(false)
	{
	}

	bigtime_t	interval;
	int32		stack_depth;
	FILE*		output;
	const char*	callgrind_directory;
	bool		profile_all;
	bool		profile_kernel;
	bool		profile_loading;
	bool		profile_teams;
	bool		profile_threads;
	bool		analyze_full_stack;
	bool		summary_result;
};


extern Options gOptions;
extern const char* kCommandName;


#endif	// OPTIONS_H

