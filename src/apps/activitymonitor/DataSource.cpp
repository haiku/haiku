/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DataSource.h"

#include <stdio.h>

#include <OS.h>
#include <String.h>

#include "SystemInfo.h"


const DataSource* kSources[] = {
	new UsedMemoryDataSource(),
	new CachedMemoryDataSource(),
	new ThreadsDataSource(),
	new CpuUsageDataSource(),
};
const size_t kSourcesCount = sizeof(kSources) / sizeof(kSources[0]);


DataSource::DataSource(int64 initialMin, int64 initialMax)
	:
	fMinimum(initialMin),
	fMaximum(initialMax),
	fInterval(1000000LL),
	fColor((rgb_color){200, 0, 0})
{
}


DataSource::DataSource()
	:
	fMinimum(0),
	fMaximum(100),
	fInterval(1000000LL),
	fColor((rgb_color){200, 0, 0})
{
}


DataSource::DataSource(const DataSource& other)
{
	fMinimum = other.fMinimum;
	fMaximum = other.fMaximum;
	fInterval = other.fInterval;
	fColor = other.fColor;
}


DataSource::~DataSource()
{
}


DataSource*
DataSource::Copy() const
{
	return NULL;
		// this class cannot be copied
}


int64
DataSource::Minimum() const
{
	return fMinimum;
}


int64
DataSource::Maximum() const
{
	return fMaximum;
}


bigtime_t
DataSource::RefreshInterval() const
{
	return fInterval;
}


void
DataSource::SetLimits(int64 min, int64 max)
{
	fMinimum = min;
	fMaximum = max;
}


void
DataSource::SetRefreshInterval(bigtime_t interval)
{
	fInterval = interval;
}


void
DataSource::SetColor(rgb_color color)
{
	fColor = color;
}


int64
DataSource::NextValue(SystemInfo& info)
{
	return 0;
}


void
DataSource::Print(BString& text, int64 value) const
{
	text = "";
	text << value;
}


const char*
DataSource::Label() const
{
	return "";
}


const char*
DataSource::Unit() const
{
	return "";
}


rgb_color
DataSource::Color() const
{
	return fColor;
}


bool
DataSource::AdaptiveScale() const
{
	return false;
}


/*static*/ int32
DataSource::CountSources()
{
	return kSourcesCount;
}


/*static*/ const DataSource*
DataSource::SourceAt(int32 index)
{
	if (index >= (int32)kSourcesCount || index < 0)
		return NULL;

	return kSources[index];
}


//	#pragma mark -


MemoryDataSource::MemoryDataSource()
{
	SystemInfo info;

	fMinimum = 0;
	fMaximum = info.MaxMemory();
}


MemoryDataSource::~MemoryDataSource()
{
}


void
MemoryDataSource::Print(BString& text, int64 value) const
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.1g MB", value / 1048576.0);

	text = buffer;
}


const char*
MemoryDataSource::Unit() const
{
	return "MB";
}


//	#pragma mark -


UsedMemoryDataSource::UsedMemoryDataSource()
{
}


UsedMemoryDataSource::~UsedMemoryDataSource()
{
}


DataSource*
UsedMemoryDataSource::Copy() const
{
	return new UsedMemoryDataSource(*this);
}


int64
UsedMemoryDataSource::NextValue(SystemInfo& info)
{
	return info.UsedMemory();
}


const char*
UsedMemoryDataSource::Label() const
{
	return "Available Memory";
}


//	#pragma mark -


CachedMemoryDataSource::CachedMemoryDataSource()
{
	fColor = (rgb_color){0, 200, 0};
}


CachedMemoryDataSource::~CachedMemoryDataSource()
{
}


DataSource*
CachedMemoryDataSource::Copy() const
{
	return new CachedMemoryDataSource(*this);
}


int64
CachedMemoryDataSource::NextValue(SystemInfo& info)
{
	return info.CachedMemory();
}


const char*
CachedMemoryDataSource::Label() const
{
	return "Cached Memory";
}


//	#pragma mark -


ThreadsDataSource::ThreadsDataSource()
{
	SystemInfo info;

	fMinimum = 0;
	fMaximum = info.MaxThreads();

	fColor = (rgb_color){0, 0, 200};
}


ThreadsDataSource::~ThreadsDataSource()
{
}


DataSource*
ThreadsDataSource::Copy() const
{
	return new ThreadsDataSource(*this);
}


int64
ThreadsDataSource::NextValue(SystemInfo& info)
{
	return info.UsedThreads();
}


const char*
ThreadsDataSource::Label() const
{
	return "Threads";
}


bool
ThreadsDataSource::AdaptiveScale() const
{
	return true;
}


//	#pragma mark -


CpuUsageDataSource::CpuUsageDataSource()
	:
	fPreviousActive(0),
	fPreviousTime(0)
{
	fMinimum = 0;
	fMaximum = 1000;

	fColor = (rgb_color){200, 200, 0};
}


CpuUsageDataSource::CpuUsageDataSource(const CpuUsageDataSource& other)
{
	fPreviousActive = other.fPreviousActive;
	fPreviousTime = other.fPreviousTime;
}


CpuUsageDataSource::~CpuUsageDataSource()
{
}


DataSource*
CpuUsageDataSource::Copy() const
{
	return new CpuUsageDataSource(*this);
}


void
CpuUsageDataSource::Print(BString& text, int64 value) const
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.1g%%", value / 10.0);

	text = buffer;
}


int64
CpuUsageDataSource::NextValue(SystemInfo& info)
{
	int32 running = 0;
	bigtime_t active = 0;

	for (int32 cpu = 0; cpu < info.Info().cpu_count; cpu++) {
		active += info.Info().cpu_infos[cpu].active_time;
		running++;
			// TODO: take disabled CPUs into account
	}

	int64 percent = int64(1000.0 * (active - fPreviousActive)
		/ (running * (info.Time() - fPreviousTime)));
	if (percent < 0)
		percent = 0;
	if (percent > 1000)
		percent = 1000;

	fPreviousActive = active;
	fPreviousTime = info.Time();

	return percent;
}


const char*
CpuUsageDataSource::Label() const
{
	return "CPU Usage";
}
