// ResourceFile.h

#ifndef RESOURCE_FILE_H
#define RESOURCE_FILE_H

#include <ByteOrder.h>
#include <List.h>

#include "OffsetFile.h"

class Exception;
class ResourceItem;
struct MemArea;
struct resource_info;
struct PEFContainerHeader;

class ResourceFile {
public:
								ResourceFile();
	virtual						~ResourceFile();

			void				Init(BFile& file);	// throws Exception
			void				Unset();
			status_t			InitCheck() const;

			bool				AddItem(ResourceItem* item, int32 index = -1);
			ResourceItem*		RemoveItem(int32 index);
			bool				RemoveItem(ResourceItem* item);
				int32				IndexOf(ResourceItem* item) const;
			ResourceItem*		ItemAt(int32 index) const;
			int32				CountItems() const;

			uint32				GetResourcesSize() const;
			uint32				WriteResources(void* buffer, uint32 size);
			void				WriteTest();

			void				PrintToStream(bool longInfo = true);

private:
			void				_InitFile(BFile& file);
			void				_InitELFFile(BFile& file);
			void				_InitPEFFile(BFile& file,
									const PEFContainerHeader& pefHeader);
			void				_ReadHeader();
			void				_ReadIndex();
			bool				_ReadIndexEntry(int32 index,
												uint32 tableOffset,
												bool peekAhead);
			void				_ReadInfoTable();
			bool				_ReadInfoTableEnd(const void* data,
												  int32 dataSize);
			const void*			_ReadResourceInfo(const MemArea& area,
												  const resource_info* info,
												  type_code type,
												  bool* readIndices);

	inline	int16				_GetInt16(int16 value);
	inline	uint16				_GetUInt16(uint16 value);
	inline	int32				_GetInt32(int32 value);
	inline	uint32				_GetUInt32(uint32 value);

private:
			BList				fItems;
			OffsetFile			fFile;
			uint32				fFileType;
			off_t				fFileSize;
			int32				fResourceCount;
			ResourceItem*		fInfoTableItem;
			bool				fHostEndianess;
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


#endif	// RESOURCE_FILE_H
