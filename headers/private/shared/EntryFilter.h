/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ENTRY_FILTER_H
#define _ENTRY_FILTER_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#if defined(__BEOS__) && !defined(__HAIKU__)
	// BeOS doesn't have <fnmatch.h>, but libroot.so features fnmatch() anyway
	extern "C" int fnmatch(const char *pattern, const char *string, int flags);
#else
#	include <fnmatch.h>
#endif

namespace BPrivate {


class BasicEntryFilter {
public:
	BasicEntryFilter()
		: fPattern(NULL),
		  fIsFileName(false)
	{
	}

	~BasicEntryFilter()
	{
		free(fPattern);
	}

	bool SetTo(const char* pattern, bool isFileName)
	{
		free(fPattern);

		fPattern = strdup(pattern);
		if (fPattern == NULL)
			return false;

		fIsFileName = isFileName;

		return true;
	}

	bool Filter(const char* path, const char* name) const
	{
		if (fPattern != NULL) {
			if (fnmatch(fPattern, (fIsFileName ? name : path), 0) == 0)
				return true;
		}

		return false;
	}

	void SetNextFilter(BasicEntryFilter* next)
	{
		fNextFilter = next;
	}

	BasicEntryFilter* NextFilter() const
	{
		 return fNextFilter;
	}

private:
	char*				fPattern;
	bool				fIsFileName;
	BasicEntryFilter*	fNextFilter;
};


class EntryFilter {
public:
	EntryFilter()
		: fIncludeFilters(NULL),
		  fExcludeFilters(NULL)
	{
	}

	~EntryFilter()
	{
		while (BasicEntryFilter* filter = fIncludeFilters) {
			fIncludeFilters = filter->NextFilter();
			delete filter;
		}

		while (BasicEntryFilter* filter = fExcludeFilters) {
			fExcludeFilters = filter->NextFilter();
			delete filter;
		}
	}

	void AddIncludeFilter(BasicEntryFilter* filter)
	{
		_AddFilter(fIncludeFilters, filter);
	}

	bool AddIncludeFilter(const char* pattern, bool isFilePattern)
	{
		return _AddFilter(fIncludeFilters, pattern, isFilePattern);
	}

	void AddExcludeFilter(BasicEntryFilter* filter)
	{
		_AddFilter(fExcludeFilters, filter);
	}

	bool AddExcludeFilter(const char* pattern, bool isFilePattern)
	{
		return _AddFilter(fExcludeFilters, pattern, isFilePattern);
	}

	bool Filter(const char* path) const
	{
		if (fExcludeFilters == NULL && fIncludeFilters)
			return true;

		// get leaf name
		const char* name = strrchr(path, '/');
		name = (name != NULL ? name + 1 : path);

		// exclude filters
		if (_Filter(fExcludeFilters, path, name))
			return false;

		// include filters -- if none are given, everything matches
		return fIncludeFilters == NULL || _Filter(fIncludeFilters, path, name);
	}

private:
	static void _AddFilter(BasicEntryFilter*& filterList,
		BasicEntryFilter* filter)
	{
		filter->SetNextFilter(filterList);
		filterList = filter;
	}

	static bool _AddFilter(BasicEntryFilter*& filterList, const char* pattern,
		bool isFilePattern)
	{
		BasicEntryFilter* filter = new(std::nothrow) BasicEntryFilter;
		if (filter == NULL)
			return false;

		if (!filter->SetTo(pattern, isFilePattern)) {
			delete filter;
			return false;
		}

		_AddFilter(filterList, filter);

		return true;
	}

	static bool _Filter(const BasicEntryFilter* const& filterList,
		const char* path, const char* name)
	{
		const BasicEntryFilter* filter = filterList;
		while (filter) {
			if (filter->Filter(path, name))
				return true;
			filter = filter->NextFilter();
		}

		return false;
	}

private:
	BasicEntryFilter*	fIncludeFilters;
	BasicEntryFilter*	fExcludeFilters;
};

}	// namespace BPrivate


#endif	// _ENTRY_FILTER_H
