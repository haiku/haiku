//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file ResourceStrings.h
	BResourceStrings interface declaration.
*/

#ifndef _sk_resource_strings_h_
#define _sk_resource_strings_h_

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
	virtual	void _ReservedResourceStrings0();
	virtual	void _ReservedResourceStrings1();
	virtual	void _ReservedResourceStrings2();
	virtual	void _ReservedResourceStrings3();
	virtual	void _ReservedResourceStrings4();
	virtual	void _ReservedResourceStrings5();
};

#endif	// _sk_resource_strings_h_
