//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file ResourcesItem.h
	ResourceItem interface declaration.
*/

#ifndef _RESOURCE_ITEM_H
#define _RESOURCE_ITEM_H

#include <DataIO.h>
#include <String.h>

namespace BPrivate {
namespace Storage {

/*!
	\class ResourceItem
	\brief Represents a resource loaded into memory.
	
	This class represents a resource completely or partially loaded into
	memory. The minimal information stored in the object should be its
	type, ID and name and the size of its data. If the data are not loaded
	then additionally the offset (into a resource file) should be valid.
	As soon as the data are loaded as well, the offset is more or less
	meaningless.

	Creating a new resource can be done by calling SetIdentity() after the
	default construction. The object then represents a resource with the
	specified type, ID and name and with empty data. Data can arbitrarily
	be read and written using the methods the super class BMallocIO provides.

	The memory for the resource data is owned by the ResourceItem object and
	freed on destruction.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/
class ResourceItem : public BMallocIO {
public:
	ResourceItem();
	virtual ~ResourceItem();

	virtual ssize_t WriteAt(off_t pos, const void *buffer, size_t size);
	virtual status_t SetSize(off_t size);

	void SetLocation(int32 offset, size_t initialSize);
	void SetIdentity(type_code type, int32 id, const char *name);

	void SetOffset(int32 offset);
	int32 Offset() const;

	size_t InitialSize() const;
	size_t DataSize() const;

	void SetType(type_code type);
	type_code Type() const;

	void SetID(int32 id);
	int32 ID() const;

	void SetName(const char *name);
	const char *Name() const;

	void *Data() const;

	void SetLoaded(bool loaded);
	bool IsLoaded() const;

	void SetModified(bool modified);
	bool IsModified() const;

private:
	int32		fOffset;
	size_t		fInitialSize;
	type_code	fType;
	int32		fID;
	BString		fName;
	bool		fIsLoaded;
	bool		fIsModified;
};

};	// namespace Storage
};	// namespace BPrivate

#endif	// _RESOURCE_ITEM_H


