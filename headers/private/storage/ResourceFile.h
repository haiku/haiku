//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file ResourceFile.h
	ResourceFile interface declaration.
*/

#ifndef _sk_resource_file_h_
#define _sk_resource_file_h_

#include <ByteOrder.h>

#include "OffsetFile.h"

struct resource_info;
struct PEFContainerHeader;

namespace StorageKit {

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
	virtual ~ResourceFile();

	status_t SetTo(BFile *file, bool clobber = false);
	void Unset();
	status_t InitCheck() const;

	status_t InitContainer(ResourcesContainer &container);
	status_t ReadResource(ResourceItem &resource, bool force = false);
	status_t ReadResources(ResourcesContainer &container, bool force = false);
	status_t WriteResources(ResourcesContainer &container);

private:
	void _InitFile(BFile &file, bool clobber);
	void _InitELFFile(BFile &file);
	void _InitPEFFile(BFile &file, const PEFContainerHeader &pefHeader);
	void _ReadHeader(resource_parse_info &parseInfo);
	void _ReadIndex(resource_parse_info &parseInfo);
	bool _ReadIndexEntry(resource_parse_info &parseInfo, int32 index,
						 uint32 tableOffset, bool peekAhead);
	void _ReadInfoTable(resource_parse_info &parseInfo);
	bool _ReadInfoTableEnd(const void *data, int32 dataSize);
	const void *_ReadResourceInfo(resource_parse_info &parseInfo,
								  const MemArea &area,
								  const resource_info *info, type_code type,
								  bool *readIndices);

	status_t _WriteResources(ResourcesContainer &container);
	status_t _MakeEmptyResourceFile();

	inline int16 _GetInt16(int16 value);
	inline uint16 _GetUInt16(uint16 value);
	inline int32 _GetInt32(int32 value);
	inline uint32 _GetUInt32(uint32 value);

private:
	OffsetFile	fFile;
	uint32		fFileType;
	bool		fHostEndianess;
	bool		fEmptyResources;
};

// _GetInt16
inline
int16
ResourceFile::_GetInt16(int16 value)
{
	return (fHostEndianess ? value : B_SWAP_INT16(value));
}

// _GetUInt16
inline
uint16
ResourceFile::_GetUInt16(uint16 value)
{
	return (fHostEndianess ? value : B_SWAP_INT16(value));
}

// _GetInt32
inline
int32
ResourceFile::_GetInt32(int32 value)
{
	return (fHostEndianess ? value : B_SWAP_INT32(value));
}

// _GetUInt32
inline
uint32
ResourceFile::_GetUInt32(uint32 value)
{
	return (fHostEndianess ? value : B_SWAP_INT32(value));
}

};	// namespace StorageKit

#endif	// _sk_resource_file_h_
