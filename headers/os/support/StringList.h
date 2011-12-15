/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SUPPORT_BSTRING_LIST_H_
#define _SUPPORT_BSTRING_LIST_H_


#include <BeBuild.h>
#include <Flattenable.h>
#include <List.h>
#include <String.h>


class BStringList : public BFlattenable {
public:
								BStringList(int32 count = 20);
								BStringList(const BStringList& other);
	virtual						~BStringList();

	// Adding and removing items.
			bool				Add(const BString& string, int32 index);
			bool				Add(const BString& string);
			bool				Add(const BStringList& list, int32 index);
			bool				Add(const BStringList& list);

			bool				Remove(const BString& string,
									bool ignoreCase = false);
			void				Remove(const BStringList& list,
									bool ignoreCase = false);
			BString				Remove(int32 index);
			bool				Remove(int32 index, int32 count);
			bool				Replace(int32 index, const BString& string);

			void				MakeEmpty();

	// Reorder items
			void				Sort(bool ignoreCase = false);
									// TODO: Sort() with custom sort function.
			bool				Swap(int32 indexA, int32 indexB);
			bool				Move(int32 fromIndex, int32 toIndex);

	// Retrieve items
			BString				StringAt(int32 index) const;
			BString				First() const;
			BString				Last() const;

	// Query
			bool				HasString(const BString& string,
									bool ignoreCase = false) const;
			int32				IndexOf(const BString& string,
									bool ignoreCase = false) const;
			int32				CountStrings() const;
			bool				IsEmpty() const;

	// Iteration
			void				DoForEach(bool (*func)(const BString& string));
			void				DoForEach(bool (*func)(const BString& string,
									void* arg2), void* arg2);

			BStringList&		operator=(const BStringList& other);
			bool				operator==(const BStringList& other) const;
			bool				operator!=(const BStringList& other) const;

	// BFlattenable
	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
			void				_IncrementRefCounts() const;
			void				_DecrementRefCounts() const;

private:
			BList				fStrings;
};


inline bool
BStringList::HasString(const BString& string, bool ignoreCase) const
{
	return IndexOf(string, ignoreCase) >= 0;
}


inline bool
BStringList::operator!=(const BStringList& other) const
{
	return !(*this == other);
}


#endif	// _SUPPORT_BSTRING_LIST_H_
