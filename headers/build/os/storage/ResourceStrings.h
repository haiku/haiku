//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file ResourceStrings.h
	BResourceStrings interface declaration.
*/

#ifndef _RESOURCE_STRINGS_H
#define _RESOURCE_STRINGS_H

#include <Entry.h>
#include <Locker.h>

class BResources;
class BString;

/*!
	\class BResourceStrings
	\brief Simple class to access the string resources in a file.
	
	A BResourceStrings object reads the string type resources from a given
	resource file and provides fast read only access to them.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/
class BResourceStrings {
public:
	BResourceStrings();
	BResourceStrings(const entry_ref &ref);
	virtual ~BResourceStrings();

	status_t InitCheck();
	virtual BString *NewString(int32 id);
	virtual const char *FindString(int32 id);

	virtual status_t SetStringFile(const entry_ref *ref);
	status_t GetStringFile(entry_ref *outRef);

public:
	enum {
		RESOURCE_TYPE = 'CSTR'
	};

protected:
	struct _string_id_hash {
		_string_id_hash();
		~_string_id_hash();
		void assign_string(const char *str, bool makeCopy);
		_string_id_hash *next;
		int32	id;
		char	*data;
		bool 	data_alloced;
		bool	_reserved1[3];
		uint32	_reserved2;
	};

protected:
	BLocker		_string_lock;
	status_t	_init_error;

private:
	entry_ref		fFileRef;
	BResources		*fResources;
	_string_id_hash	**fHashTable;
	int32			fHashTableSize;
	int32			fStringCount;
	uint32			_reserved[16];	// FBC

private:
	void _Cleanup();
	void _MakeEmpty();
	status_t _Rehash(int32 newSize);
	_string_id_hash *_AddString(char *str, int32 id, bool wasMalloced);

	virtual _string_id_hash *_FindString(int32 id);

	// FBC
	virtual	status_t _Reserved_ResourceStrings_0(void *);
	virtual	status_t _Reserved_ResourceStrings_1(void *);
	virtual	status_t _Reserved_ResourceStrings_2(void *);
	virtual	status_t _Reserved_ResourceStrings_3(void *);
	virtual	status_t _Reserved_ResourceStrings_4(void *);
	virtual	status_t _Reserved_ResourceStrings_5(void *);
};

#endif	// _RESOURCE_STRINGS_H


