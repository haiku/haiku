/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/* BMessageBody handles data storage and retrieval for BMessage. */

#ifndef _MESSAGE_BODY_H_
#define _MESSAGE_BODY_H_

#include <List.h>
#include "MessageField2.h"

enum {
	B_FLATTENABLE_TYPE = 'FLAT'
};

namespace BPrivate {

class BMessageBody {
public:
							BMessageBody();
							BMessageBody(const BMessageBody &other);
							~BMessageBody();

		BMessageBody		&operator=(const BMessageBody &other);

		status_t			GetInfo(type_code typeRequested, int32 which,
								char **name, type_code *typeFound,
								int32 *countFound = NULL) const;
		status_t			GetInfo(const char *name, type_code *typeFound,
								int32 *countFound = NULL) const;
		status_t			GetInfo(const char *name, type_code *typeFound,
								bool *fixedSize) const;

		int32				CountNames(type_code type) const;
		bool				IsEmpty() const;

		ssize_t				FlattenedSize() const;
		status_t			Flatten(BDataIO *stream) const;
		status_t			Unflatten(BDataIO *stream);

		status_t			AddData(const char *name, BMallocIO *buffer,
								type_code type);
		status_t			RemoveData(const char *name, int32 index = 0);

		bool				HasData(const char *name, type_code t, int32 n) const;
		status_t			FindData(const char *name, type_code type,
								int32 index, const void **data,
								ssize_t *numBytes) const;
		status_t			ReplaceData(const char *name, int32 index,
								BMallocIO *buffer, type_code type);

		status_t			Rename(const char *oldName, const char *newName);
		status_t			RemoveName(const char *name);
		status_t			MakeEmpty();

		void				PrintToStream() const;

		// hash table support
		void				HashInsert(BMessageField *field);
		BMessageField		*HashLookup(const char *name) const;
		BMessageField		*HashRemove(const char *name);
		uint32				HashString(const char *string) const;
		void				HashClear();

private:
		status_t			InitCommon();
		BMessageField		*AddField(const char *name, type_code type,
								status_t &error);
		BMessageField		*FindData(const char *name, type_code type,
								status_t &error) const;

		BList				fFieldList;
		BMessageField		**fFieldTable;
		int32				fFieldTableSize;
};

} // namespace BPrivate

#endif // _MESSAGE_BODY_H_
