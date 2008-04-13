/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H


#include <InterfaceDefs.h>
#include <String.h>

class SystemInfo;


class DataSource {
public:
						DataSource(int64 initialMin, int64 initialMax);
						DataSource();
						DataSource(const DataSource& other);
	virtual				~DataSource();

	virtual DataSource*	Copy() const;
	virtual DataSource*	CopyForCPU(int32 cpu) const;

			int64		Minimum() const;
			int64		Maximum() const;
			bigtime_t	RefreshInterval() const;

	virtual void		SetLimits(int64 min, int64 max);
	virtual void		SetRefreshInterval(bigtime_t interval);
	virtual void		SetColor(rgb_color color);

	virtual	int64		NextValue(SystemInfo& info);
	virtual void		Print(BString& text, int64 value) const;

	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	Unit() const;
	virtual rgb_color	Color() const;
	virtual bool		AdaptiveScale() const;
	virtual int32		CPU() const;
	virtual bool		PerCPU() const;
	virtual bool		MultiCPUOnly() const;

	static int32		CountSources();
	static const DataSource* SourceAt(int32 index);
	static const DataSource* FindSource(const char* name);
	static int32		IndexOf(const DataSource* source);

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


class CPUUsageDataSource : public DataSource {
public:
						CPUUsageDataSource(int32 cpu = 0);
						CPUUsageDataSource(const CPUUsageDataSource& other);
	virtual				~CPUUsageDataSource();

	virtual DataSource*	Copy() const;
	virtual DataSource*	CopyForCPU(int32 cpu) const;

	virtual void		Print(BString& text, int64 value) const;
	virtual	int64		NextValue(SystemInfo& info);

	virtual const char*	Name() const;
	virtual const char*	Label() const;

	virtual int32		CPU() const;
	virtual bool		PerCPU() const;

private:
			void		_SetCPU(int32 cpu);

	bigtime_t			fPreviousActive;
	bigtime_t			fPreviousTime;
	int32				fCPU;
	BString				fLabel;
};


class CPUCombinedUsageDataSource : public DataSource {
public:
						CPUCombinedUsageDataSource();
						CPUCombinedUsageDataSource(
							const CPUCombinedUsageDataSource& other);
	virtual				~CPUCombinedUsageDataSource();

	virtual DataSource*	Copy() const;

	virtual void		Print(BString& text, int64 value) const;
	virtual	int64		NextValue(SystemInfo& info);

	virtual const char*	Name() const;
	virtual const char*	Label() const;

	virtual bool		MultiCPUOnly() const;

private:
	bigtime_t			fPreviousActive;
	bigtime_t			fPreviousTime;
};


class NetworkUsageDataSource : public DataSource {
public:
						NetworkUsageDataSource(bool in);
						NetworkUsageDataSource(
							const NetworkUsageDataSource& other);
	virtual				~NetworkUsageDataSource();

	virtual DataSource*	Copy() const;

	virtual void		Print(BString& text, int64 value) const;
	virtual	int64		NextValue(SystemInfo& info);

	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;

private:
	bool				fIn;
	uint64				fPreviousBytes;
	bigtime_t			fPreviousTime;
};

#endif	// DATA_SOURCE_H
