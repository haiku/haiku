/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _NOT_OWNING_ENTRY_REF_H
#define _NOT_OWNING_ENTRY_REF_H


#include <Entry.h>
#include <Node.h>


namespace BPrivate {


/*!	entry_ref subclass that avoids cloning the entry name.

	Therefore initialization is cheaper and cannot fail. It derives from
	entry_ref for convenience. However, care must be taken that:
	 - the name remains valid while the object is in use,
	 - the object isn't passed as an entry_ref to a function that modifies it.
 */
class NotOwningEntryRef : public entry_ref {
public:
	NotOwningEntryRef()
	{
	}

	NotOwningEntryRef(dev_t device, ino_t directory, const char* name)
	{
		SetTo(device, directory, name);
	}

	NotOwningEntryRef(const node_ref& directoryRef, const char* name)
	{
		SetTo(directoryRef, name);
	}

	NotOwningEntryRef(const entry_ref& other)
	{
		*this = other;
	}

	~NotOwningEntryRef()
	{
		name = NULL;
	}

	NotOwningEntryRef& SetTo(dev_t device, ino_t directory, const char* name)
	{
		this->device = device;
		this->directory = directory;
		this->name = const_cast<char*>(name);
		return *this;
	}

	NotOwningEntryRef& SetTo(const node_ref& directoryRef, const char* name)
	{
		return SetTo(directoryRef.device, directoryRef.node, name);
	}

	node_ref DirectoryNodeRef() const
	{
		return node_ref(device, directory);
	}

	NotOwningEntryRef& SetDirectoryNodeRef(const node_ref& directoryRef)
	{
		device = directoryRef.device;
		directory = directoryRef.node;
		return *this;
	}

	NotOwningEntryRef& operator=(const entry_ref& other)
	{
		return SetTo(other.device, other.directory, other.name);
	}
};


} // namespace BPrivate


using ::BPrivate::NotOwningEntryRef;


#endif	// _NOT_OWNING_ENTRY_REF_H
