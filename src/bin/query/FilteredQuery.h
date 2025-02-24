#ifndef __FILTEREDQUERY_H
#define __FILTEREDQUERY_H

#include <Query.h>

#include "ObjectList.h"

typedef bool (*filter_function)(const entry_ref *ref, void *arg);

struct filter_pair {
	filter_function filter;
	void *args;

	filter_pair(filter_function function, void *arguments)
	{
		filter = function;
		args = arguments;
	}
};


class TFilteredQuery : public BQuery {
public:
	TFilteredQuery();

	// BQuery doesn't have a copy constructor. We supply
	// this method to workaround this problem
	TFilteredQuery(const BQuery &query);
	TFilteredQuery(const TFilteredQuery &query);
	virtual ~TFilteredQuery();

	bool AddFilter(filter_function function, void *arg);
	void RemoveFilter(filter_function function);

	virtual status_t GetNextRef(entry_ref *ref);
	virtual status_t GetNextEntry(BEntry *entry, bool traverse = false);

	status_t Clear();

private:
	BObjectList<filter_pair> fFilters;
};

#endif //__FILTEREDQUERY_H
