/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H


#include <InterfaceDefs.h>
#include <String.h>

class SystemInfo;


enum scale_type {
	kNoScale,
	kBytePerSecondScale,
};

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

	virtual const char*	InternalName() const = 0;
	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual const char*	Unit() const;
	virtual rgb_color	Color() const;
	virtual bool		AdaptiveScale() const;
	virtual scale_type	ScaleType() const;
	virtual int32		CPU() const;
	virtual bool		PerCPU() const;
	virtual bool		MultiCPUOnly() const;
	virtual bool		Primary() const;

	static int32		CountSources();
	static const DataSource* SourceAt(int32 index);
	static const DataSource* FindSource(const char* internalName);
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
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		Primary() const;
};


class CachedMemoryDataSource : public MemoryDataSource {
public:
						CachedMemoryDataSource();
	virtual				~CachedMemoryDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		Primary() const;
};


class SwapSpaceDataSource : public MemoryDataSource {
public:
						SwapSpaceDataSource();
	virtual				~SwapSpaceDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		Primary() const;
};


class BlockCacheDataSource : public MemoryDataSource {
public:
						BlockCacheDataSource();
	virtual				~BlockCacheDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
};


class SemaphoresDataSource : public DataSource {
public:
						SemaphoresDataSource();
	virtual				~SemaphoresDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		AdaptiveScale() const;
};


class PortsDataSource : public DataSource {
public:
						PortsDataSource();
	virtual				~PortsDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;
};


class ThreadsDataSource : public DataSource {
public:
						ThreadsDataSource();
	virtual				~ThreadsDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;
};


class TeamsDataSource : public DataSource {
public:
						TeamsDataSource();
	virtual				~TeamsDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;
};


class RunningAppsDataSource : public DataSource {
public:
						RunningAppsDataSource();
	virtual				~RunningAppsDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
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

	virtual const char*	InternalName() const;
	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;

	virtual int32		CPU() const;
	virtual bool		PerCPU() const;
	virtual bool		Primary() const;

private:
			void		_SetCPU(int32 cpu);

	bigtime_t			fPreviousActive;
	bigtime_t			fPreviousTime;
	int32				fCPU;
	BString				fLabel;
	BString				fShortLabel;
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

	virtual const char*	InternalName() const;
	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;

	virtual bool		MultiCPUOnly() const;
	virtual bool		Primary() const;

private:
	bigtime_t			fPreviousActive;
	bigtime_t			fPreviousTime;
};


class PageFaultsDataSource : public DataSource {
public:
						PageFaultsDataSource();
						PageFaultsDataSource(
							const PageFaultsDataSource& other);
	virtual				~PageFaultsDataSource();

	virtual DataSource*	Copy() const;

	virtual void		Print(BString& text, int64 value) const;
	virtual	int64		NextValue(SystemInfo& info);

	virtual const char*	InternalName() const;
	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		AdaptiveScale() const;
	virtual bool		Primary() const;

private:
	uint32				fPreviousFaults;
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

	virtual const char*	InternalName() const;
	virtual const char*	Name() const;
	virtual const char*	Label() const;
	virtual const char*	ShortLabel() const;
	virtual bool		AdaptiveScale() const;
	virtual scale_type	ScaleType() const;
	virtual bool		Primary() const;

private:
	bool				fIn;
	uint64				fPreviousBytes;
	bigtime_t			fPreviousTime;
};


class ClipboardSizeDataSource : public DataSource {
public:
						ClipboardSizeDataSource(bool text);
						ClipboardSizeDataSource(
							const ClipboardSizeDataSource& other);
	virtual				~ClipboardSizeDataSource();

	virtual DataSource*	Copy() const;

	virtual	int64		NextValue(SystemInfo& info);

	virtual const char*	InternalName() const;
	virtual const char*	Label() const;
	virtual const char*	Unit() const;
	virtual bool		AdaptiveScale() const;

private:
	bool				fText;
};


class MediaNodesDataSource : public DataSource {
public:
						MediaNodesDataSource();
	virtual				~MediaNodesDataSource();

	virtual DataSource*	Copy() const;

	virtual const char*	InternalName() const;
	virtual	int64		NextValue(SystemInfo& info);
	virtual const char*	Label() const;
	virtual bool		AdaptiveScale() const;
};

#endif	// DATA_SOURCE_H
