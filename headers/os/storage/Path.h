/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_H
#define _PATH_H


#include <Flattenable.h>
#include <Message.h>
#include <StorageDefs.h>


// Forward declarations
class BDirectory;
class BEntry;
struct entry_ref;


class BPath : public BFlattenable {
public:
							BPath();
							BPath(const BPath& path);
							BPath(const entry_ref* ref);
							BPath(const BEntry* entry);
							BPath(const char* dir, const char* leaf = NULL,
								bool normalize = false);
							BPath(const BDirectory* dir, 
								const char* leaf = NULL,
								bool normalize = false);

	virtual					~BPath();

			status_t		InitCheck() const;

			status_t		SetTo(const entry_ref* ref);
			status_t		SetTo(const BEntry* entry);
			status_t		SetTo(const char* path, const char* leaf = NULL,
								bool normalize = false);
			status_t		SetTo(const BDirectory* dir, 
								const char* leaf = NULL,
								bool normalize = false);
			void			Unset();

			status_t		Append(const char* path, bool normalize = false);

			const char*		Path() const;
			const char*		Leaf() const;
			status_t		GetParent(BPath* path) const;

			bool			operator==(const BPath& item) const;
			bool			operator==(const char* path) const;
			bool			operator!=(const BPath& item) const;
			bool			operator!=(const char* path) const;
			BPath&			operator=(const BPath& item);
			BPath&			operator=(const char* path);

	// BFlattenable protocol
	virtual	bool			IsFixedSize() const;
	virtual	type_code		TypeCode() const;
	virtual	ssize_t			FlattenedSize() const;
	virtual	status_t		Flatten(void* buffer, ssize_t size) const;
	virtual	bool			AllowsTypeCode(type_code code) const;
	virtual	status_t		Unflatten(type_code code, const void* buffer,
								ssize_t size);

private:
	virtual void			_WarPath1();
	virtual void			_WarPath2();
	virtual void			_WarPath3();

			status_t		_SetPath(const char* path);
	static	bool			_MustNormalize(const char* path, status_t* _error);

			uint32			_reserved[4];

			char*			fName;
			status_t		fCStatus;
};

#endif	// _PATH_H
