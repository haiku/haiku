/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef _SYMBOL_LOOKUP_H
#define _SYMBOL_LOOKUP_H

#include <stdio.h>

#include <OS.h>

#include <util/DoublyLinkedList.h>

//#define TRACE_DEBUG_SYMBOL_LOOKUP
#ifdef TRACE_DEBUG_SYMBOL_LOOKUP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif

struct image_t;
struct runtime_loader_debug_area;

namespace BPrivate {

// Exception
class Exception {
public:
	Exception(status_t error)
		: fError(error)
	{
	}

	Exception(const Exception &other)
		: fError(other.fError)
	{
	}

	status_t Error() const	{ return fError; }

private:
	status_t	fError;
};

// Area
class Area : public DoublyLinkedListLinkImpl<Area> {
public:
	Area(area_id id, const void *address, int32 size)
		: fRemoteID(id),
		  fLocalID(-1),
		  fRemoteAddress(address),
		  fLocalAddress(NULL),
		  fSize(size)
	{
	}

	~Area()
	{
		if (fLocalID >= 0)
			delete_area(fLocalID);
	}

	const void* RemoteAddress() const	{ return fRemoteAddress; }
	const void* LocalAddress() const	{ return fLocalAddress; }
	int32 Size() const					{ return fSize; }

	bool ContainsAddress(const void *address, int32 size) const
	{
		return ((addr_t)fRemoteAddress <= (addr_t)address
			&& (addr_t)address + size <= (addr_t)fRemoteAddress + fSize);
	}

	const void *PrepareAddress(const void *address);

private:
	area_id		fRemoteID;
	area_id		fLocalID;
	const void	*fRemoteAddress;
	void		*fLocalAddress;
	int32		fSize;
};

// RemoteMemoryAccessor
class RemoteMemoryAccessor {
public:
	RemoteMemoryAccessor(team_id team);
	~RemoteMemoryAccessor();

	status_t Init();

	const void *PrepareAddress(const void *remoteAddress, int32 size);

	template<typename Type> inline const Type &Read(const Type &remoteData)
	{
		const void *remoteAddress = &remoteData;
		const void *localAddress = PrepareAddress(remoteAddress,
			sizeof(remoteData));
		return *(const Type*)localAddress;
	}

private:
	Area &_FindArea(const void *address, int32 size);

	typedef DoublyLinkedList<Area>	AreaList;

protected:
	team_id		fTeam;

private:
	AreaList	fAreas;
};

// SymbolLookup
class SymbolLookup : private RemoteMemoryAccessor {
public:
	SymbolLookup(team_id team);
	~SymbolLookup();

	status_t Init();

	status_t LookupSymbolAddress(addr_t address, addr_t *_baseAddress,
		const char **_symbolName, const char **_imageName, bool *_exactMatch);

private:
	const image_t *_FindImageAtAddress(addr_t address);

	const runtime_loader_debug_area	*fDebugArea;
};

}	// namespace BPrivate

using BPrivate::SymbolLookup;

#endif	// _SYMBOL_LOOKUP_H
