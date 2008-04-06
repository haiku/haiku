/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H


#include <InterfaceDefs.h>

class BString;
class SystemInfo;


class DataSource {
public:
						DataSource(int64 initialMin, int64 initialMax);
						DataSource();
						DataSource(const DataSource& other);
	virtual				~DataSource();

	virtual DataSource*	Copy() const;

			int64		Minimum() const;
			int64		Maximum() const;
			bigtime_t	RefreshInterval() const;

	virtual void		SetLimits(int64 min, int64 max);
	virtual void		SetRefreshInterval(bigtime_t interval);
	virtual void		SetColor(rgb_color color);

	virtual	int64		NextValue(SystemInfo& info);
	virtual void		Print(BString& text, int64 value) const;

	virtual const char*	Label() const;
	virtual const char*	Unit() const;
	virtual rgb_color	Color() const;
	virtual bool		AdaptiveScale() const;

	static int32		CountSources();
	static const DataSource* SourceAt(int32 index);

protected:
	int64				fMinimum;
	int64				fMaximum;
	bigtime_t			fInterval;
	rgb_color			fColor;
};


class MemoryDataSource : public DataSource {
public:
						MemoryDataSource();
	virtual				~MemoryDataSource();

	virtual void		Print(BString& text, int64 value) const;
	virtual const char*	Unit() const;
};


class UsedMemoryDataSource : public MemoryDataSource {
public:
						UsedMemoryDataSource();
	virtual				~UsedMemoryDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	Label() const;
};


class CachedMemoryDataSource : public MemoryDataSource {
public:
						CachedMemoryDataSource();
	virtual				~CachedMemoryDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	Label() const;
};


class ThreadsDataSource : public DataSource {
public:
						ThreadsDataSource();
	virtual				~ThreadsDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;
};


class CpuUsageDataSource : public DataSource {
public:
						CpuUsageDataSource();
						CpuUsageDataSource(const CpuUsageDataSource& other);
	virtual				~CpuUsageDataSource();

	virtual DataSource*	Copy() const;

	virtual void		Print(BString& text, int64 value) const;
	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	Label() const;

private:
	bigtime_t			fPreviousActive;
	bigtime_t			fPreviousTime;
};

#endif	// DATA_SOURCE_H
