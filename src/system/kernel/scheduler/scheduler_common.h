/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_COMMON_H
#define KERNEL_SCHEDULER_COMMON_H


#include <algorithm>

#include <debug.h>
#include <kscheduler.h>
#include <load_tracking.h>
#include <smp.h>
#include <thread.h>
#include <user_debugger.h>
#include <util/MinMaxHeap.h>

#include "RunQueue.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


namespace Scheduler {


struct CPUEntry;
struct CoreEntry;

const int kLowLoad = kMaxLoad * 20 / 100;
const int kTargetLoad = kMaxLoad * 55 / 100;
const int kHighLoad = kMaxLoad * 70 / 100;
const int kMediumLoad = (kHighLoad + kTargetLoad) / 2;
const int kVeryHighLoad = (kMaxLoad + kHighLoad) / 2;

const int kLoadDifference = kMaxLoad * 20 / 100;

extern bool gSingleCore;
extern bool gTrackCoreLoad;
extern bool gTrackCPULoad;


void init_debug_commands();


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_COMMON_H

