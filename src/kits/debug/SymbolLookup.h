/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef SYMBOL_LOOKUP_H
#define SYMBOL_LOOKUP_H

#include <stdio.h>

#include <image.h>
#include <OS.h>

#include <util/DoublyLinkedList.h>


struct image_t;
struct runtime_loader_debug_area;


namespace BPrivate {
namespace Debug {

class Image;


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

	bool ContainsLocalAddress(const void* address) const
	{
		return (addr_t)address >= (addr_t)fLocalAddress
			&& (addr_t)address < (addr_t)fLocalAddress + fSize;
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

	const void *PrepareAddress(const void *remoteAddress, int32 size) const;
	const void *PrepareAddressNoThrow(const void *remoteAddress,
		int32 size) const;

	template<typename Type> inline const Type &Read(
		const Type &remoteData) const
	{
		const void *remoteAddress = &remoteData;
		const void *localAddress = PrepareAddress(remoteAddress,
			sizeof(remoteData));
		return *(const Type*)localAddress;
	}

	Area* AreaForLocalAddress(const void* address) const;

private:
	Area &_FindArea(const void *address, int32 size) const;
	Area* _FindAreaNoThrow(const void *address, int32 size) const;

	typedef DoublyLinkedList<Area>	AreaList;

protected:
	team_id		fTeam;

private:
	AreaList	fAreas;
};


// SymbolIterator
struct SymbolIterator {
	const Image*		image;
	int32				currentIndex;
};


// SymbolLookup
class SymbolLookup : private RemoteMemoryAccessor {
public:
	SymbolLookup(team_id team);
	~SymbolLookup();

	status_t Init();

	status_t LookupSymbolAddress(addr_t address, addr_t *_baseAddress,
		const char **_symbolName, size_t *_symbolNameLen,
		const char **_imageName, bool *_exactMatch) const;

	status_t InitSymbolIterator(image_id imageID,
		SymbolIterator& iterator) const;
	status_t InitSymbolIteratorByAddress(addr_t address,
		SymbolIterator& iterator) const;
	status_t NextSymbol(SymbolIterator& iterator, const char** _symbolName,
		size_t* _symbolNameLen, addr_t* _symbolAddress, size_t* _symbolSize,
		int32* _symbolType) const;

	status_t GetSymbol(image_id imageID, const char* name, int32 symbolType,
		void** _symbolLocation, size_t* _symbolSize, int32* _symbolType) const;

private:
	class LoadedImage;
	friend class LoadedImage;

private:
	const image_t* _FindLoadedImageAtAddress(addr_t address) const;
	const image_t* _FindLoadedImageByID(image_id id) const;
	Image* _FindImageAtAddress(addr_t address) const;
	Image* _FindImageByID(image_id id) const;
	size_t _SymbolNameLen(const char* address) const;

private:
	const runtime_loader_debug_area	*fDebugArea;
	DoublyLinkedList<Image>	fImages;
};

}	// namespace Debug
}	// namespace BPrivate

using BPrivate::Debug::SymbolLookup;

#endif	// SYMBOL_LOOKUP_H
