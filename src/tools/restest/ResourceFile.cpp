// ResourceFile.cpp

#include "ResourceFile.h"

#include <algobase.h>
#include <stdio.h>

#include "Elf.h"
#include "Exception.h"
#include "Pef.h"
#include "ResourceItem.h"
#include "ResourcesDefs.h"
#include "Warnings.h"

// ELF defs
static const uint32	kMaxELFHeaderSize			= sizeof(Elf32_Ehdr) + 32;
static const char	kELFFileMagic[4]			= { 0x7f, 'E', 'L', 'F' };

// sanity bounds
static const uint32	kMaxResourceCount			= 10000;
static const uint32	kELFMaxResourceAlignment	= 1024 * 1024 * 10;	// 10 MB

// recognized file types (indices into kFileTypeNames)
enum {
	FILE_TYPE_UNKNOWN		= 0,
	FILE_TYPE_X86_RESOURCE	= 1,
	FILE_TYPE_PPC_RESOURCE	= 2,
	FILE_TYPE_ELF			= 3,
	FILE_TYPE_PEF			= 4,
};

const char* kFileTypeNames[] = {
	"unknown",
	"x86 resource file",
	"PPC resource file",
	"ELF object file",
	"PEF object file",
};


// helper functions/classes

// read_exactly
static
void
read_exactly(BPositionIO& file, off_t position, void* buffer, size_t size,
			 const char* errorMessage = NULL)
{
	ssize_t read = file.ReadAt(position, buffer, size);
	if (read < 0)
		throw Exception(read, errorMessage);
	else if ((size_t)read != size) {
		if (errorMessage) {
			throw Exception("%s Read to few bytes (%ld/%lu).", errorMessage,
							read, size);
		} else
			throw Exception("Read to few bytes (%ld/%lu).", read, size);
	}
}

// align_value
template<typename TV, typename TA>
static inline
TV
align_value(const TV& value, const TA& alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

// calculate_checksum
static
uint32
calculate_checksum(const void* data, uint32 size)
{
	uint32 checkSum = 0;
	const uint8* csData = (const uint8*)data;
	const uint8* dataEnd = csData + size;
	const uint8* current = csData;
	for (; current < dataEnd; current += 4) {
		uint32 word = 0;
		int32 bytes = min(4L, dataEnd - current);
		for (int32 i = 0; i < bytes; i++)
			word = (word << 8) + current[i];
		checkSum += word;
	}
	return checkSum;
}

// skip_bytes
static inline
const void*
skip_bytes(const void* buffer, int32 offset)
{
	return (const char*)buffer + offset;
}

// skip_bytes
static inline
void*
skip_bytes(void* buffer, int32 offset)
{
	return (char*)buffer + offset;
}

// fill_pattern
static
void
fill_pattern(uint32 byteOffset, void* _buffer, uint32 count)
{
	uint32* buffer = (uint32*)_buffer;
	for (uint32 i = 0; i < count; i++)
		buffer[i] = kUnusedResourceDataPattern[(byteOffset / 4 + i) % 3];
}

// fill_pattern
static
void
fill_pattern(const void* dataBegin, void* buffer, uint32 count)
{
	fill_pattern((char*)buffer - (const char*)dataBegin, buffer, count);
}

// fill_pattern
static
void
fill_pattern(const void* dataBegin, void* buffer, const void* bufferEnd)
{
	fill_pattern(dataBegin, buffer,
				 ((const char*)bufferEnd - (char*)buffer) / 4);
}

// check_pattern
static
bool
check_pattern(uint32 byteOffset, void* _buffer, uint32 count,
			  bool hostEndianess)
{
	bool result = true;
	uint32* buffer = (uint32*)_buffer;
	for (uint32 i = 0; result && i < count; i++) {
		uint32 value = buffer[i];
		if (!hostEndianess)
			value = B_SWAP_INT32(value);
		result
			= (value == kUnusedResourceDataPattern[(byteOffset / 4 + i) % 3]);
	}
	return result;
}

// MemArea
struct MemArea {
	MemArea(const void* data, uint32 size) : data(data), size(size) {}

	inline bool check(const void* _current, uint32 skip = 0) const
	{
		const char* start = (const char*)data;
		const char* current = (const char*)_current;
		return (start <= current && start + size >= current + skip);
	}

	const void*	data;
	uint32		size;
};

// AutoDeleter
template<typename C>
struct AutoDeleter {
	AutoDeleter(C* object, bool array = false) : object(object), array(array)
	{
	}

	~AutoDeleter()
	{
		if (array)
			delete[] object;
		else
			delete object;
	}

	C*		object;
	bool	array;
};


// constructor
ResourceFile::ResourceFile()
			: fItems(),
			  fFile(),
			  fFileType(FILE_TYPE_UNKNOWN),
			  fFileSize(0),
			  fResourceCount(0),
			  fInfoTableItem(NULL),
			  fHostEndianess(true)
{
}

// destructor
ResourceFile::~ResourceFile()
{
	Unset();
}

// Init
void
ResourceFile::Init(BFile& file)
{
	Unset();
	try {
		_InitFile(file);
		_ReadHeader();
		_ReadIndex();
		_ReadInfoTable();
	} catch (Exception exception) {
		Unset();
		throw exception;
	}
}

// Unset
void
ResourceFile::Unset()
{
	// items
	for (int32 i = 0; ResourceItem* item = ItemAt(i); i++)
		delete item;
	fItems.MakeEmpty();
	// file
	fFile.Unset();
	fFileType = FILE_TYPE_UNKNOWN;
	fFileSize = 0;
	// resource count
	fResourceCount = 0;
	// info table
	delete fInfoTableItem;
	fInfoTableItem = NULL;
	fHostEndianess = true;
}

// InitCheck
status_t
ResourceFile::InitCheck() const
{
	return fFile.InitCheck();
}

// AddItem
bool
ResourceFile::AddItem(ResourceItem* item, int32 index)
{
	bool result = false;
	if (item) {
		if (index < 0 || index > CountItems())
			index = CountItems();
		result = fItems.AddItem(item);
	}
	return result;
}

// RemoveItem
ResourceItem*
ResourceFile::RemoveItem(int32 index)
{
	return (ResourceItem*)fItems.RemoveItem(index);
}

// RemoveItem
bool
ResourceFile::RemoveItem(ResourceItem* item)
{
	return RemoveItem(IndexOf(item));
}

// IndexOf
int32
ResourceFile::IndexOf(ResourceItem* item) const
{
	return fItems.IndexOf(item);
}

// ItemAt
ResourceItem*
ResourceFile::ItemAt(int32 index) const
{
	return (ResourceItem*)fItems.ItemAt(index);
}

// CountItems
int32
ResourceFile::CountItems() const
{
	return fItems.CountItems();
}

// GetResourcesSize
uint32
ResourceFile::GetResourcesSize() const
{
	if (!fInfoTableItem || fFile.InitCheck())
		throw Exception("Resource file not initialized.");
	// header
	uint32 size = kResourcesHeaderSize;
	// index section
	uint32 indexSectionSize = kResourceIndexSectionHeaderSize
		+ fResourceCount * kResourceIndexEntrySize;
	indexSectionSize = align_value(indexSectionSize,
								   kResourceIndexSectionAlignment);
	size += indexSectionSize;
	// unknown section
	size += kUnknownResourceSectionSize;
	// data
	uint32 dataSize = 0;
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		dataSize += item->GetSize();
	}
	size += dataSize;
	// info table
	uint32 infoTableSize = 0;
	type_code type = 0;
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		if (i == 0 || type != item->GetType()) {
			if (i != 0)
				infoTableSize += kResourceInfoSeparatorSize;
			type = item->GetType();
			infoTableSize += kMinResourceInfoBlockSize;
		} else
			infoTableSize += kMinResourceInfoSize;
		uint32 nameLen = strlen(item->GetName());
		if (nameLen != 0)
			infoTableSize += nameLen + 1;
	}
	infoTableSize += kResourceInfoSeparatorSize + kResourceInfoTableEndSize;
	size += infoTableSize;
	return size;
}

// WriteResources
uint32
ResourceFile::WriteResources(void* buffer, uint32 bufferSize)
{
	// calculate sizes and offsets
	// header
	uint32 size = kResourcesHeaderSize;
	// index section
	uint32 indexSectionOffset = size;
	uint32 indexSectionSize = kResourceIndexSectionHeaderSize
		+ fResourceCount * kResourceIndexEntrySize;
	indexSectionSize = align_value(indexSectionSize,
								   kResourceIndexSectionAlignment);
	size += indexSectionSize;
	// unknown section
	uint32 unknownSectionOffset = size;
	uint32 unknownSectionSize = kUnknownResourceSectionSize;
	size += unknownSectionSize;
	// data
	uint32 dataOffset = size;
	uint32 dataSize = 0;
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		dataSize += item->GetSize();
	}
	size += dataSize;
	// info table
	uint32 infoTableOffset = size;
	uint32 infoTableSize = 0;
	type_code type = 0;
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		if (i == 0 || type != item->GetType()) {
			if (i != 0)
				infoTableSize += kResourceInfoSeparatorSize;
			type = item->GetType();
			infoTableSize += kMinResourceInfoBlockSize;
		} else
			infoTableSize += kMinResourceInfoSize;
		uint32 nameLen = strlen(item->GetName());
		if (nameLen != 0)
			infoTableSize += nameLen + 1;
	}
	infoTableSize += kResourceInfoSeparatorSize + kResourceInfoTableEndSize;
	size += infoTableSize;
	// check whether the buffer is large enough
	if (!buffer)
		throw Exception("Supplied buffer is NULL.");
	if (bufferSize < size)
		throw Exception("Supplied buffer is too small.");
	// write...
	void* data = buffer;
	// header
	resources_header* resourcesHeader = (resources_header*)data;
	resourcesHeader->rh_resources_magic = kResourcesHeaderMagic;
	resourcesHeader->rh_resource_count = fResourceCount;
	resourcesHeader->rh_index_section_offset = indexSectionOffset;
	resourcesHeader->rh_admin_section_size = indexSectionOffset
											 + indexSectionSize;
	for (int32 i = 0; i < 13; i++)
		resourcesHeader->rh_pad[i] = 0;
	// index section
	// header
	data = skip_bytes(buffer, indexSectionOffset);
	resource_index_section_header* indexHeader
		= (resource_index_section_header*)data;
	indexHeader->rish_index_section_offset = indexSectionOffset;
	indexHeader->rish_index_section_size = indexSectionSize;
	indexHeader->rish_unknown_section_offset = unknownSectionOffset;
	indexHeader->rish_unknown_section_size = unknownSectionSize;
	indexHeader->rish_info_table_offset = infoTableOffset;
	indexHeader->rish_info_table_size = infoTableSize;
	fill_pattern(buffer, &indexHeader->rish_unused_data1, 1);
	fill_pattern(buffer, indexHeader->rish_unused_data2, 25);
	fill_pattern(buffer, &indexHeader->rish_unused_data3, 1);
	// index table
	data = skip_bytes(data, kResourceIndexSectionHeaderSize);
	resource_index_entry* entry = (resource_index_entry*)data;
	uint32 entryOffset = dataOffset;
	for (int32 i = 0; i < fResourceCount; i++, entry++) {
		ResourceItem* item = ItemAt(i);
		uint32 entrySize = item->GetSize();
		entry->rie_offset = entryOffset;
		entry->rie_size = entrySize;
		entry->rie_pad = 0;
		entryOffset += entrySize;
	}
	// padding + unknown section
	data = skip_bytes(buffer, dataOffset);
	fill_pattern(buffer, entry, data);
	// data
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		status_t error = item->LoadData(fFile);
		if (error != B_OK)
			throw Exception(error, "Error loading resource data.");
		uint32 entrySize = item->GetSize();
		memcpy(data, item->GetData(), entrySize);
		data = skip_bytes(data, entrySize);
	}
	// info table
	data = skip_bytes(buffer, infoTableOffset);
	type = 0;
	for (int32 i = 0; i < fResourceCount; i++) {
		ResourceItem* item = ItemAt(i);
		resource_info* info = NULL;
		if (i == 0 || type != item->GetType()) {
			if (i != 0) {
				resource_info_separator* separator
					= (resource_info_separator*)data;
				separator->ris_value1 = 0xffffffff;
				separator->ris_value2 = 0xffffffff;
				data = skip_bytes(data, kResourceInfoSeparatorSize);
			}
			type = item->GetType();
			resource_info_block* infoBlock = (resource_info_block*)data;
			infoBlock->rib_type = type;
			info = infoBlock->rib_info;
		} else
			info = (resource_info*)data;
		// info
		info->ri_id = item->GetID();
		info->ri_index = i + 1;
		info->ri_name_size = 0;
		data = info->ri_name;
		uint32 nameLen = strlen(item->GetName());
		if (nameLen != 0) {
			memcpy(info->ri_name, item->GetName(), nameLen + 1);
			data = skip_bytes(data, nameLen + 1);
			info->ri_name_size = nameLen + 1;
		}
	}
	// separator
	resource_info_separator* separator = (resource_info_separator*)data;
	separator->ris_value1 = 0xffffffff;
	separator->ris_value2 = 0xffffffff;
	// table end
	data = skip_bytes(data, kResourceInfoSeparatorSize);
	resource_info_table_end* tableEnd = (resource_info_table_end*)data;
	void* infoTable = skip_bytes(buffer, infoTableOffset);
	tableEnd->rite_check_sum = calculate_checksum(infoTable,
		infoTableSize - kResourceInfoTableEndSize);
	tableEnd->rite_terminator = 0;
	// final check
	data = skip_bytes(data, kResourceInfoTableEndSize);
	uint32 bytesWritten = (char*)data - (char*)buffer;
	if (bytesWritten != size) {
		throw Exception("Bad boy error: Wrote %lu bytes, though supposed to "
						"write %lu bytes.", bytesWritten, size);
	}
	return size;
}

// WriteTest
void
ResourceFile::WriteTest()
{
	uint32 size = GetResourcesSize();
	if (size != fFileSize) {
		throw Exception("Calculated resources size differs from actual size "
						"in file: %lu vs %Ld.", size, fFileSize);
	}
	char* buffer1 = new char[size];
	char* buffer2 = new char[size];
	try {
		WriteResources(buffer1, size);
		read_exactly(fFile, 0, buffer2, size,
					 "Write test: Error reading resources.");
		for (uint32 i = 0; i < size; i++) {
			if (buffer1[i] != buffer2[i]) {
				off_t filePosition = fFile.GetOffset() + i;
				throw Exception("Written resources differ from those in file. "
								"First difference at byte %lu (file position "
								"%Ld): %x vs %x.", i, filePosition,
								(int)buffer1[i] & 0xff,
								(int)buffer2[i] & 0xff);
			}
		}
	} catch (Exception exception) {
		delete[] buffer1;
		delete[] buffer2;
		throw exception;
	}
	delete[] buffer1;
	delete[] buffer2;
}



// PrintToStream
void
ResourceFile::PrintToStream(bool longInfo)
{
	if (longInfo) {
		off_t resourcesOffset = fFile.GetOffset();
		printf("ResourceFile:\n");
		printf("file type              : %s\n", kFileTypeNames[fFileType]);
		printf("endianess              : %s\n",
			   (fHostEndianess == (bool)B_HOST_IS_LENDIAN) ? "little" : "big");
		printf("resource section offset: 0x%08Lx (%Ld)\n", resourcesOffset,
			   resourcesOffset);
		if (fInfoTableItem) {
			int32 offset = fInfoTableItem->GetOffset();
			int32 size = fInfoTableItem->GetSize();
			printf("resource info table    : offset: 0x%08lx (%ld), "
				   "size: 0x%08lx (%ld)\n", offset, offset, size, size);
		}
		printf("number of resources    : %ld\n", fResourceCount);
		for (int32 i = 0; i < fResourceCount; i++) {
			ResourceItem* item = ItemAt(i);
			item->PrintToStream();
		}
	} else {
		printf("   Type     ID     Size  Name\n");
		printf("  ------ ----- --------  --------------------\n");
		for (int32 i = 0; i < fResourceCount; i++) {
			ResourceItem* item = ItemAt(i);
			type_code type = item->GetType();
			char typeName[4] = { type >> 24, (type >> 16) & 0xff,
								 (type >> 8) & 0xff, type & 0xff };
			printf("  '%.4s' %5ld %8lu  %s\n", typeName, item->GetID(),
				   item->GetSize(), item->GetName());
		}
	}
}


// _InitFile
void
ResourceFile::_InitFile(BFile& file)
{
	status_t error = B_OK;
	fFile.Unset();
	// read the first four bytes, and check, if they identify a resource file
	char magic[4];
	read_exactly(file, 0, magic, 4, "Failed to read magic number.");
	if (!memcmp(magic, kX86ResourceFileMagic, 4)) {
		// x86 resource file
		fHostEndianess = B_HOST_IS_LENDIAN;
		fFileType = FILE_TYPE_X86_RESOURCE;
		fFile.SetTo(file, kX86ResourcesOffset);
	} else if (!memcmp(magic, kPEFFileMagic1, 4)) {
		PEFContainerHeader pefHeader;
		read_exactly(file, 0, &pefHeader, kPEFContainerHeaderSize,
					 "Failed to read PEF container header.");
		if (!memcmp(pefHeader.tag2, kPPCResourceFileMagic, 4)) {
			// PPC resource file
			fHostEndianess = B_HOST_IS_BENDIAN;
			fFileType = FILE_TYPE_PPC_RESOURCE;
			fFile.SetTo(file, kPPCResourcesOffset);
		} else if (!memcmp(pefHeader.tag2, kPEFFileMagic2, 4)) {
			// PEF file
			fFileType = FILE_TYPE_PEF;
			_InitPEFFile(file, pefHeader);
		} else
			throw Exception("File is not a resource file.");
	} else if (!memcmp(magic, kELFFileMagic, 4)) {
		// ELF file
		fFileType = FILE_TYPE_ELF;
		_InitELFFile(file);
	} else if (!memcmp(magic, kX86ResourceFileMagic, 2)) {
		// x86 resource file with screwed magic?
		Warnings::AddCurrentWarning("File magic is 0x%08lx. Should be 0x%08lx "
									"for x86 resource file. Try anyway.",
									ntohl(*(uint32*)magic),
									ntohl(*(uint32*)kX86ResourceFileMagic));
		fHostEndianess = B_HOST_IS_LENDIAN;
		fFileType = FILE_TYPE_X86_RESOURCE;
		fFile.SetTo(file, kX86ResourcesOffset);
	} else
		throw Exception("File is not a resource file.");
	error = fFile.InitCheck();
	if (error != B_OK)
		throw Exception(error, "Failed to initialize resource file.");
	// get the file size
	fFileSize = 0;
	error = fFile.GetSize(&fFileSize);
	if (error != B_OK)
		throw Exception(error, "Failed to get the file size.");
}

// _InitELFFile
void
ResourceFile::_InitELFFile(BFile& file)
{
	status_t error = B_OK;
	// get the file size
	off_t fileSize = 0;
	error = file.GetSize(&fileSize);
	if (error != B_OK)
		throw Exception(error, "Failed to get the file size.");
	// read ELF header
	Elf32_Ehdr fileHeader;
	read_exactly(file, 0, &fileHeader, sizeof(Elf32_Ehdr),
				 "Failed to read ELF header.");
	// check data encoding (endianess)
	switch (fileHeader.e_ident[EI_DATA]) {
		case ELFDATA2LSB:
			fHostEndianess = B_HOST_IS_LENDIAN;
			break;
		case ELFDATA2MSB:
			fHostEndianess = B_HOST_IS_BENDIAN;
			break;
		default:
		case ELFDATANONE:
			throw Exception("Unsupported ELF data encoding.");
			break;
	}
	// get the header values
	uint32 headerSize				= _GetUInt16(fileHeader.e_ehsize);
	uint32 programHeaderTableOffset	= _GetUInt32(fileHeader.e_phoff);
	uint32 programHeaderSize		= _GetUInt16(fileHeader.e_phentsize);
	uint32 programHeaderCount		= _GetUInt16(fileHeader.e_phnum);
	uint32 sectionHeaderTableOffset	= _GetUInt32(fileHeader.e_shoff);
	uint32 sectionHeaderSize		= _GetUInt16(fileHeader.e_shentsize);
	uint32 sectionHeaderCount		= _GetUInt16(fileHeader.e_shnum);
	bool hasProgramHeaderTable = (programHeaderTableOffset != 0);
	bool hasSectionHeaderTable = (sectionHeaderTableOffset != 0);
//printf("headerSize              : %lu\n", headerSize);
//printf("programHeaderTableOffset: %lu\n", programHeaderTableOffset);
//printf("programHeaderSize       : %lu\n", programHeaderSize);
//printf("programHeaderCount      : %lu\n", programHeaderCount);
//printf("sectionHeaderTableOffset: %lu\n", sectionHeaderTableOffset);
//printf("sectionHeaderSize       : %lu\n", sectionHeaderSize);
//printf("sectionHeaderCount      : %lu\n", sectionHeaderCount);
	// check the sanity of the header values
	// ELF header size
	if (headerSize < sizeof(Elf32_Ehdr) || headerSize > kMaxELFHeaderSize) {
		throw Exception("Invalid ELF header: invalid ELF header size: %lu.",
						headerSize);
	}
	uint32 resourceOffset = headerSize;
	uint32 resourceAlignment = 0;
	// program header table offset and entry count/size
	uint32 programHeaderTableSize = 0;
	if (hasProgramHeaderTable) {
		if (programHeaderTableOffset < headerSize
			|| programHeaderTableOffset > fileSize) {
			throw Exception("Invalid ELF header: invalid program header table "
							"offset: %lu.", programHeaderTableOffset);
		}
		programHeaderTableSize = programHeaderSize * programHeaderCount;
		if (programHeaderSize < sizeof(Elf32_Phdr)
			|| programHeaderTableOffset + programHeaderTableSize > fileSize) {
			throw Exception("Invalid ELF header: program header table exceeds "
							"file: %lu.",
							programHeaderTableOffset + programHeaderTableSize);
		}
		resourceOffset = max(resourceOffset, programHeaderTableOffset
											 + programHeaderTableSize);
		// iterate through the program headers
		for (int32 i = 0; i < (int32)programHeaderCount; i++) {
			uint32 shOffset = programHeaderTableOffset + i * programHeaderSize;
			Elf32_Phdr programHeader;
			read_exactly(file, shOffset, &programHeader, sizeof(Elf32_Shdr),
						 "Failed to read ELF program header.");
			// get the header values
			uint32 type			= _GetUInt32(programHeader.p_type);
			uint32 offset		= _GetUInt32(programHeader.p_offset);
			uint32 size			= _GetUInt32(programHeader.p_filesz);
			uint32 alignment	= _GetUInt32(programHeader.p_align);
//printf("segment: type: %ld, offset: %lu, size: %lu, alignment: %lu\n",
//type, offset, size, alignment);
			// check the values
			// PT_NULL marks the header unused,
			if (type != PT_NULL) {
				if (/*offset < headerSize ||*/ offset > fileSize) {
					throw Exception("Invalid ELF program header: invalid "
									"program offset: %lu.", offset);
				}
				uint32 segmentEnd = offset + size;
				if (segmentEnd > fileSize) {
					throw Exception("Invalid ELF section header: segment "
									"exceeds file: %lu.", segmentEnd);
				}
				resourceOffset = max(resourceOffset, segmentEnd);
				resourceAlignment = max(resourceAlignment, alignment);
			}
		}
	}
	// section header table offset and entry count/size
	uint32 sectionHeaderTableSize = 0;
	if (hasSectionHeaderTable) {
		if (sectionHeaderTableOffset < headerSize
			|| sectionHeaderTableOffset > fileSize) {
			throw Exception("Invalid ELF header: invalid section header table "
							"offset: %lu.", sectionHeaderTableOffset);
		}
		sectionHeaderTableSize = sectionHeaderSize * sectionHeaderCount;
		if (sectionHeaderSize < sizeof(Elf32_Shdr)
			|| sectionHeaderTableOffset + sectionHeaderTableSize > fileSize) {
			throw Exception("Invalid ELF header: section header table exceeds "
							"file: %lu.",
							sectionHeaderTableOffset + sectionHeaderTableSize);
		}
		resourceOffset = max(resourceOffset, sectionHeaderTableOffset
											 + sectionHeaderTableSize);
		// iterate through the section headers
		for (int32 i = 0; i < (int32)sectionHeaderCount; i++) {
			uint32 shOffset = sectionHeaderTableOffset + i * sectionHeaderSize;
			Elf32_Shdr sectionHeader;
			read_exactly(file, shOffset, &sectionHeader, sizeof(Elf32_Shdr),
						 "Failed to read ELF section header.");
			// get the header values
			uint32 type		= _GetUInt32(sectionHeader.sh_type);
			uint32 offset	= _GetUInt32(sectionHeader.sh_offset);
			uint32 size		= _GetUInt32(sectionHeader.sh_size);
//printf("section: type: %ld, offset: %lu, size: %lu\n", type, offset, size);
			// check the values
			// SHT_NULL marks the header unused,
			// SHT_NOBITS sections take no space in the file
			if (type != SHT_NULL && type != SHT_NOBITS) {
				if (offset < headerSize || offset > fileSize) {
					throw Exception("Invalid ELF section header: invalid "
									"section offset: %lu.", offset);
				}
				uint32 sectionEnd = offset + size;
				if (sectionEnd > fileSize) {
					throw Exception("Invalid ELF section header: section "
									"exceeds file: %lu.", sectionEnd);
				}
				resourceOffset = max(resourceOffset, sectionEnd);
			}
		}
	}
//printf("resourceOffset: %lu\n", resourceOffset);
	// align the offset
	if (resourceAlignment < kELFMinResourceAlignment)
		resourceAlignment = kELFMinResourceAlignment;
	if (resourceAlignment > kELFMaxResourceAlignment) {
		throw Exception("The ELF object file requires an invalid alignment: "
						"%lu.", resourceAlignment);
	}
	resourceOffset = align_value(resourceOffset, resourceAlignment);
//printf("resourceOffset: %lu\n", resourceOffset);
	if (resourceOffset >= fileSize)
		throw Exception("The ELF object file does not contain resources.");
	// fine, init the offset file
	fFile.SetTo(file, resourceOffset);
}

// _InitPEFFile
void
ResourceFile::_InitPEFFile(BFile& file, const PEFContainerHeader& pefHeader)
{
	status_t error = B_OK;
	// get the file size
	off_t fileSize = 0;
	error = file.GetSize(&fileSize);
	if (error != B_OK)
		throw Exception(error, "Failed to get the file size.");
	// check architecture -- we support PPC only
	if (memcmp(pefHeader.architecture, kPEFArchitecturePPC, 4))
		throw Exception("PEF file architecture is not PPC.");
	fHostEndianess = B_HOST_IS_BENDIAN;
	// get the section count
	uint16 sectionCount = _GetUInt16(pefHeader.sectionCount);
	// iterate through the PEF sections headers
	uint32 sectionHeaderTableOffset = kPEFContainerHeaderSize;
	uint32 sectionHeaderTableEnd
		= sectionHeaderTableOffset + sectionCount * kPEFSectionHeaderSize;
	uint32 resourceOffset = sectionHeaderTableEnd;
	for (int32 i = 0; i < (int32)sectionCount; i++) {
		uint32 shOffset = sectionHeaderTableOffset + i * kPEFSectionHeaderSize;
		PEFSectionHeader sectionHeader;
		read_exactly(file, shOffset, &sectionHeader, kPEFSectionHeaderSize,
					 "Failed to read PEF section header.");
		// get the header values
		uint32 offset	= _GetUInt32(sectionHeader.containerOffset);
		uint32 size		= _GetUInt32(sectionHeader.packedSize);
		// check the values
		if (offset < sectionHeaderTableEnd || offset > fileSize) {
			throw Exception("Invalid PEF section header: invalid "
							"section offset: %lu.", offset);
		}
		uint32 sectionEnd = offset + size;
		if (sectionEnd > fileSize) {
			throw Exception("Invalid PEF section header: section "
							"exceeds file: %lu.", sectionEnd);
		}
		resourceOffset = max(resourceOffset, sectionEnd);
	}
	// init the offset file
	fFile.SetTo(file, resourceOffset);
}

// _ReadHeader
void
ResourceFile::_ReadHeader()
{
	// read the header
	resources_header header;
	read_exactly(fFile, 0, &header, kResourcesHeaderSize,
				 "Failed to read the header.");
	// check the header
	// magic
	uint32 magic = _GetUInt32(header.rh_resources_magic);
	if (magic == kResourcesHeaderMagic) {
		// everything is fine
	} else if (B_SWAP_INT32(magic) == kResourcesHeaderMagic) {
		const char* endianessStr[2] = { "little", "big" };
		int32 endianess
			= (fHostEndianess == ((bool)B_HOST_IS_LENDIAN ? 0 : 1));
		Warnings::AddCurrentWarning("Endianess seems to be %s, although %s "
									"was expected.",
									endianessStr[1 - endianess],
									endianessStr[endianess]);
		fHostEndianess = !fHostEndianess;
	} else
		throw Exception("Invalid resources header magic.");
	// resource count
	uint32 resourceCount = _GetUInt32(header.rh_resource_count);
	if (resourceCount > kMaxResourceCount)
		throw Exception("Bad number of resources.");
	// index section offset
	uint32 indexSectionOffset = _GetUInt32(header.rh_index_section_offset);
	if (indexSectionOffset != kResourceIndexSectionOffset) {
		throw Exception("Unexpected resource index section offset. Is: %lu, "
						"should be: %lu.", indexSectionOffset,
						kResourceIndexSectionOffset);
	}
	// admin section size
	uint32 indexSectionSize = kResourceIndexSectionHeaderSize
							  + kResourceIndexEntrySize * resourceCount;
	indexSectionSize = align_value(indexSectionSize,
								   kResourceIndexSectionAlignment);
	uint32 adminSectionSize = _GetUInt32(header.rh_admin_section_size);
	if (adminSectionSize != indexSectionOffset + indexSectionSize) {
		throw Exception("Unexpected resource admin section size. Is: %lu, "
						"should be: %lu.", adminSectionSize,
						indexSectionOffset + indexSectionSize);
	}
	// set the resource count
	fResourceCount = resourceCount;
}

// _ReadIndex
void
ResourceFile::_ReadIndex()
{
	// read the header
	resource_index_section_header header;
	read_exactly(fFile, kResourceIndexSectionOffset, &header,
				 kResourceIndexSectionHeaderSize,
				 "Failed to read the resource index section header.");
	// check the header
	// index section offset
	uint32 indexSectionOffset = _GetUInt32(header.rish_index_section_offset);
	if (indexSectionOffset != kResourceIndexSectionOffset) {
		throw Exception("Unexpected resource index section offset. Is: %lu, "
						"should be: %lu.", indexSectionOffset,
						kResourceIndexSectionOffset);
	}
	// index section size
	uint32 expectedIndexSectionSize = kResourceIndexSectionHeaderSize
		+ kResourceIndexEntrySize * fResourceCount;
	expectedIndexSectionSize = align_value(expectedIndexSectionSize,
										   kResourceIndexSectionAlignment);
	uint32 indexSectionSize = _GetUInt32(header.rish_index_section_size);
	if (indexSectionSize != expectedIndexSectionSize) {
		throw Exception("Unexpected resource index section size. Is: %lu, "
						"should be: %lu.", indexSectionSize,
						expectedIndexSectionSize);
	}
	// unknown section offset
	uint32 unknownSectionOffset
		= _GetUInt32(header.rish_unknown_section_offset);
	if (unknownSectionOffset != indexSectionOffset + indexSectionSize) {
		throw Exception("Unexpected resource index section size. Is: %lu, "
						"should be: %lu.", unknownSectionOffset,
						indexSectionOffset + indexSectionSize);
	}
	// unknown section size
	uint32 unknownSectionSize = _GetUInt32(header.rish_unknown_section_size);
	if (unknownSectionSize != kUnknownResourceSectionSize) {
		throw Exception("Unexpected resource index section offset. Is: %lu, "
						"should be: %lu.", unknownSectionOffset,
						kUnknownResourceSectionSize);
	}
	// info table offset and size
	uint32 infoTableOffset = _GetUInt32(header.rish_info_table_offset);
	uint32 infoTableSize = _GetUInt32(header.rish_info_table_size);
	if (infoTableOffset + infoTableSize > fFileSize)
		throw Exception("Invalid info table location.");
	fInfoTableItem = new ResourceItem;
	fInfoTableItem->SetLocation(infoTableOffset, infoTableSize);
	// read the index entries
	uint32 indexTableOffset = indexSectionOffset
							  + kResourceIndexSectionHeaderSize;
	int32 maxResourceCount = (unknownSectionOffset - indexTableOffset)
							 / kResourceIndexEntrySize;
	int32 actualResourceCount = 0;
	bool tableEndReached = false;
	for (int32 i = 0; !tableEndReached && i < maxResourceCount; i++) {
		// read one entry
		tableEndReached = !_ReadIndexEntry(i, indexTableOffset,
										   (i >= fResourceCount));
		if (!tableEndReached)
			actualResourceCount++;
	}
	// check resource count
	if (actualResourceCount != fResourceCount) {
		if (actualResourceCount > fResourceCount) {
			Warnings::AddCurrentWarning("Resource index table contains "
										"%ld entries, although it should be "
										"%ld only.", actualResourceCount,
										fResourceCount);
		}
		fResourceCount = actualResourceCount;
	}
}

// _ReadIndexEntry
bool
ResourceFile::_ReadIndexEntry(int32 index, uint32 tableOffset, bool peekAhead)
{
	bool result = true;
	resource_index_entry entry;
	// read one entry
	off_t entryOffset = tableOffset + index * kResourceIndexEntrySize;
	read_exactly(fFile, entryOffset, &entry, kResourceIndexEntrySize,
				 "Failed to read a resource index entry.");
	// check, if the end is reached early
	if (result && check_pattern(entryOffset, &entry,
								kResourceIndexEntrySize / 4, fHostEndianess)) {
		if (!peekAhead) {
			Warnings::AddCurrentWarning("Unexpected end of resource index "
										"table at index: %ld (/%ld).",
										index + 1, fResourceCount);
		}
		result = false;
	}
	uint32 offset = _GetUInt32(entry.rie_offset);
	uint32 size = _GetUInt32(entry.rie_size);
	// check the location
	if (result && offset + size > fFileSize) {
		if (peekAhead) {
			Warnings::AddCurrentWarning("Invalid data after resource index "
										"table.");
		} else {
			throw Exception("Invalid resource index entry: index: %ld, "
							"offset: %lu (%lx), size: %lu (%lx).", index + 1,
							offset, offset, size, size);
		}
		result = false;
	}
	// add the entry
	if (result) {
		ResourceItem* item = new ResourceItem;
		item->SetLocation(offset, size);
		AddItem(item, index);
	}
	return result;
}

// _ReadInfoTable
void
ResourceFile::_ReadInfoTable()
{
	status_t error = B_OK;
	error = fInfoTableItem->LoadData(fFile);
	if (error != B_OK)
		throw Exception(error, "Failed to read resource info table.");
	const void* tableData = fInfoTableItem->GetData();
	int32 dataSize = fInfoTableItem->GetSize();
	//
	bool* readIndices = new bool[fResourceCount + 1];	// + 1 => always > 0
	for (int32 i = 0; i < fResourceCount; i++)
		readIndices[i] = false;
	AutoDeleter<bool> deleter(readIndices, true);
	MemArea area(tableData, dataSize);
	const void* data = tableData;
	// check the table end/check sum
	if (_ReadInfoTableEnd(data, dataSize))
		dataSize -= kResourceInfoTableEndSize;
	// read the infos
	int32 resourceIndex = 1;
	uint32 minRemainderSize
		= kMinResourceInfoBlockSize + kResourceInfoSeparatorSize;
	while (area.check(data, minRemainderSize)) {
		// read a resource block
		if (!area.check(data, kMinResourceInfoBlockSize)) {
			throw Exception("Unexpected end of resource info table at index "
							"%ld.", resourceIndex);
		}
		const resource_info_block* infoBlock
			= (const resource_info_block*)data;
		type_code type = _GetUInt32(infoBlock->rib_type);
		// read the infos of this block
		const resource_info* info = infoBlock->rib_info;
		while (info) {
			data = _ReadResourceInfo(area, info, type, readIndices);
			// prepare for next iteration, if there is another info
			if (!area.check(data, kResourceInfoSeparatorSize)) {
				throw Exception("Unexpected end of resource info table after "
								"index %ld.", resourceIndex);
			}
			const resource_info_separator* separator
				= (const resource_info_separator*)data;
			if (_GetUInt32(separator->ris_value1) == 0xffffffff
				&& _GetUInt32(separator->ris_value2) == 0xffffffff) {
				// info block ends
				info = NULL;
				data = skip_bytes(data, kResourceInfoSeparatorSize);
			} else {
				// another info follows
				info = (const resource_info*)data;
			}
			resourceIndex++;
		}
		// end of the info block
	}
	// handle special case: empty resource info table
	if (resourceIndex == 1) {
		if (!area.check(data, kResourceInfoSeparatorSize)) {
			throw Exception("Unexpected end of resource info table.");
		}
		const resource_info_separator* tableTerminator
			= (const resource_info_separator*)data;
		if (_GetUInt32(tableTerminator->ris_value1) != 0xffffffff
			|| _GetUInt32(tableTerminator->ris_value2) != 0xffffffff) {
			throw Exception("The resource info table ought to be empty, but "
							"is not properly terminated.");
		}
		data = skip_bytes(data, kResourceInfoSeparatorSize);
	}
	// Check, if the correct number of bytes are remaining.
	uint32 bytesLeft = (const char*)tableData + dataSize - (const char*)data;
	if (bytesLeft != 0) {
		throw Exception("Error at the end of the resource info table: %lu "
						"bytes are remaining.", bytesLeft);
	}
	// check, if all items have been initialized
	for (int32 i = fResourceCount - 1; i >= 0; i--) {
		if (!readIndices[i]) {
			Warnings::AddCurrentWarning("Resource item at index %ld "
										"has no info. Item removed.", i + 1);
			if (ResourceItem* item = RemoveItem(i))
				delete item;
			fResourceCount--;
		}
	}
}

// _ReadInfoTableEnd
bool
ResourceFile::_ReadInfoTableEnd(const void* data, int32 dataSize)
{
	bool hasTableEnd = true;
	if ((uint32)dataSize < kResourceInfoSeparatorSize)
		throw Exception("Info table is too short.");
	if ((uint32)dataSize < kResourceInfoTableEndSize)
		hasTableEnd = false;
	if (hasTableEnd) {
		const resource_info_table_end* tableEnd
			= (const resource_info_table_end*)
			  skip_bytes(data, dataSize - kResourceInfoTableEndSize);
		if (_GetInt32(tableEnd->rite_terminator) != 0)
			hasTableEnd = false;
		if (hasTableEnd) {
			dataSize -= kResourceInfoTableEndSize;
			// checksum
			uint32 checkSum = calculate_checksum(data, dataSize);
			uint32 fileCheckSum = _GetUInt32(tableEnd->rite_check_sum);
			if (checkSum != fileCheckSum) {
				throw Exception("Invalid resource info table check sum: In "
								"file: %lx, calculated: %lx.", fileCheckSum,
								checkSum);
			}
		}
	}
	if (!hasTableEnd)
		Warnings::AddCurrentWarning("resource info table has no check sum.");
	return hasTableEnd;
}

// _ReadResourceInfo
const void*
ResourceFile::_ReadResourceInfo(const MemArea& area, const resource_info* info,
								type_code type, bool* readIndices)
{
	int32 id = _GetInt32(info->ri_id);
	int32 index = _GetInt32(info->ri_index);
	uint16 nameSize = _GetUInt16(info->ri_name_size);
	const char* name = info->ri_name;
	// check the values
	bool ignore = false;
	// index
	if (index < 1 || index > fResourceCount) {
		Warnings::AddCurrentWarning("Invalid index field in resource "
									"info table: %lu.", index);
		ignore = true;
	}
	if (!ignore) {
		if (readIndices[index - 1]) {
			throw Exception("Multiple resource infos with the same index "
							"field: %ld.", index);
		}
		readIndices[index - 1] = true;
	}
	// name size
	if (!area.check(name, nameSize)) {
		throw Exception("Invalid name size (%d) for index %ld in "
						"resource info table.", (int)nameSize, index);
	}
	// check, if name is null terminated
	if (name[nameSize - 1] != 0) {
		Warnings::AddCurrentWarning("Name for index %ld in "
									"resource info table is not null "
									"terminated.", index);
	}
	// set the values
	if (!ignore) {
		BString resourceName(name, nameSize);
		if (ResourceItem* item = ItemAt(index - 1))
			item->SetIdentity(type, id, resourceName.String());
		else {
			throw Exception("Unexpected error: No resource item at index "
							"%ld.", index);
		}
	}
	return skip_bytes(name, nameSize);
}
