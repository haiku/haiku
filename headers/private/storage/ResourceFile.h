/*
 * Copyright 2002-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RESOURCE_FILE_H
#define _RESOURCE_FILE_H


/*!
	\file ResourceFile.h
	ResourceFile interface declaration.
*/


#include <ByteOrder.h>

#include "OffsetFile.h"


struct resource_info;
struct PEFContainerHeader;


namespace BPrivate {
namespace Storage {


class Exception;
struct MemArea;
class ResourceItem;
struct resource_parse_info;
class ResourcesContainer;


/*!
	\class ResourceFile
	\brief Represents a file capable of containing resources.

	This class provides access to the resources of a file.
	Basically a ResourceFile object can be set to a file, load infos for the
	resources without loading their data (InitContainer()), read the data of
	one (ReadResource()) or all resources (ReadResources()) and write all
	resources to the file (WriteResources()).

	Note, that the object does only provide the I/O functionality, it does
	not store any information about the resources -- this is done via a
	ResourcesContainer. We gain flexibility using this approach, since e.g.
	a certain resource may be represented by more than one ResourceItem and
	we can have as many ResourcesContainers for the resources as we like.
	In particular it is nice, that at any time we can write an arbitrary set
	of resources to the file.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>

	\version 0.0.0
*/
class ResourceFile {
public:
								ResourceFile();
	virtual						~ResourceFile();

			status_t			SetTo(BFile* file, bool clobber = false);
			void				Unset();
			status_t			InitCheck() const;

			status_t			InitContainer(ResourcesContainer& container);
			status_t			ReadResource(ResourceItem& resource,
									bool force = false);
			status_t			ReadResources(ResourcesContainer& container,
									bool force = false);
			status_t			WriteResources(ResourcesContainer& container);

private:
			void				_InitFile(BFile& file, bool clobber);

			void				_InitELFFile(BFile& file);

			template<typename ElfHeader, typename ElfProgramHeader,
				typename ElfSectionHeader>
			void				_InitELFXFile(BFile& file, uint64 fileSize);

			void				_InitPEFFile(BFile& file,
									const PEFContainerHeader& pefHeader);
			void				_ReadHeader(resource_parse_info& parseInfo);
			void				_ReadIndex(resource_parse_info& parseInfo);
			bool				_ReadIndexEntry(BPositionIO& buffer,
									resource_parse_info& parseInfo,
									int32 index, uint32 tableOffset,
									bool peekAhead);
			void				_ReadInfoTable(resource_parse_info& parseInfo);
			bool				_ReadInfoTableEnd(const void* data,
									int32 dataSize);
			const void*			_ReadResourceInfo(
									resource_parse_info& parseInfo,
									const MemArea& area,
									const resource_info* info, type_code type,
									bool* readIndices);

			status_t			_WriteResources(ResourcesContainer& container);
			status_t			_MakeEmptyResourceFile();

	inline	int16				_GetInt(int16 value) const;
	inline	uint16				_GetInt(uint16 value) const;
	inline	int32				_GetInt(int32 value) const;
	inline	uint32				_GetInt(uint32 value) const;
	inline	int64				_GetInt(int64 value) const;
	inline	uint64				_GetInt(uint64 value) const;

private:
			OffsetFile			fFile;
			uint32				fFileType;
			bool				fHostEndianess;
			bool				fEmptyResources;
};


inline int16
ResourceFile::_GetInt(int16 value) const
{
	return fHostEndianess ? value : (int16)B_SWAP_INT16((uint16)value);
}


inline uint16
ResourceFile::_GetInt(uint16 value) const
{
	return fHostEndianess ? value : B_SWAP_INT16(value);
}


inline int32
ResourceFile::_GetInt(int32 value) const
{
	return fHostEndianess ? value : (int32)B_SWAP_INT32((uint32)value);
}


inline uint32
ResourceFile::_GetInt(uint32 value) const
{
	return fHostEndianess ? value : B_SWAP_INT32(value);
}


inline int64
ResourceFile::_GetInt(int64 value) const
{
	return fHostEndianess ? value : (int64)B_SWAP_INT64((uint64)value);
}


inline uint64
ResourceFile::_GetInt(uint64 value) const
{
	return fHostEndianess ? value : B_SWAP_INT64(value);
}


};	// namespace Storage
};	// namespace BPrivate


#endif	// _RESOURCE_FILE_H
