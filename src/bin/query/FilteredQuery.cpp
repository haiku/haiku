#include "FilteredQuery.h"

#include <Debug.h>
#include <Entry.h>
#include <String.h>
#include <Volume.h>

// Helper function to copy a query.
// Used to avoid code duplication in
// TFilteredQuery(const BQuery &) and TFilteredQuery(const TFilteredQuery &)
static void
CopyQuery(const BQuery &query, BQuery *dest)
{
	ASSERT(dest);

	BQuery &nonConst = const_cast<BQuery &>(query);

	// BQuery doesn't have a copy constructor,
	// so we have to do the work ourselves...
	// Copy the predicate
	BString buffer;
	nonConst.GetPredicate(&buffer);
	dest->SetPredicate(buffer.String());

	// Copy the targetted volume
	BVolume volume(nonConst.TargetDevice());
	dest->SetVolume(&volume);
}


TFilteredQuery::TFilteredQuery()
{
}


TFilteredQuery::TFilteredQuery(const BQuery &query)
	:
	BQuery()
{
	CopyQuery(query, this);
}


TFilteredQuery::TFilteredQuery(const TFilteredQuery &query)
	:
	BQuery()
{
	CopyQuery(query, this);

	// copy filters
	fFilters = query.fFilters;
}


TFilteredQuery::~TFilteredQuery()
{
	Clear();
}


bool
TFilteredQuery::AddFilter(filter_function filter, void *arg)
{
	filter_pair *filterPair = new filter_pair(filter, arg);

	return fFilters.AddItem(filterPair);
}


void
TFilteredQuery::RemoveFilter(filter_function function)
{
	int32 count = fFilters.CountItems();
	for (int32 i = 0; i < count; i++) {
		filter_pair *pair = fFilters.ItemAt(i);
		if (pair->filter == function) {
			delete fFilters.RemoveItemAt(i);
			break;
		}
	}
}


status_t
TFilteredQuery::GetNextRef(entry_ref *ref)
{
	entry_ref tmpRef;
	status_t result;

	int32 filterCount = fFilters.CountItems();
	while ((result = BQuery::GetNextRef(&tmpRef)) == B_OK) {
		bool accepted = true;
		// We have a match, so let the entry_ref go through the filters
		// and see if it passes all the requirements
		for (int32 i = 0; i < filterCount; i++) {
			filter_pair *pair = fFilters.ItemAt(i);
			filter_function filter = pair->filter;
			accepted = (*filter)(&tmpRef, pair->args);
			if (!accepted)
				break;
		}

		if (accepted) {
			// Ok, this entry_ref passed all tests
			*ref = tmpRef;
			break;
		}
	}

	return result;
}


status_t
TFilteredQuery::GetNextEntry(BEntry *entry, bool traverse)
{
	// This code is almost a full copy/paste from Haiku's
	// BQuery::GetNextEntry(BEntry *entry, bool traverse)

	entry_ref ref;
	status_t error = GetNextRef(&ref);
	if (error == B_OK)
		error = entry->SetTo(&ref, traverse);

	return error;
}


status_t
TFilteredQuery::Clear()
{
	int32 filtersCount = fFilters.CountItems();
	for (int32 i = 0; i < filtersCount; i++)
		delete fFilters.RemoveItemAt(i);

	return BQuery::Clear();
}
