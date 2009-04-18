/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TIME_STATS_H
#define TIME_STATS_H

#include <SupportDefs.h>


void	do_scheduling_analysis(bigtime_t startTime, bigtime_t endTime,
			size_t bufferSize);
void	do_timing_analysis(int argc, const char* const* argv,
			bool schedulingAnalysis, int outFD, size_t bufferSize);


#endif	// TIME_STATS_H
