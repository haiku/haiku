/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CoreFile.h"

#include <errno.h>

#include <OS.h>

#include <AutoDeleter.h>

#include "Tracing.h"


static const size_t kMaxNotesSize = 10 * 1024 * 1024;


// pragma mark - CoreFileTeamInfo


CoreFileTeamInfo::CoreFileTeamInfo()
	:
	fId(-1),
	fUid(-1),
	fGid(-1),
	fArgs()
{
}


void
CoreFileTeamInfo::Init(int32 id, int32 uid, int32 gid, const BString& args)
{
	fId = id;
	fUid = uid;
	fGid = gid;
	fArgs = args;
}


// pragma mark - CoreFileAreaInfo


CoreFileAreaInfo::CoreFileAreaInfo(ElfSegment* segment, int32 id,
	uint64 baseAddress, uint64 size, uint64 ramSize, uint32 locking,
	uint32 protection, const BString& name)
	:
	fSegment(segment),
	fBaseAddress(baseAddress),
	fSize(size),
	fRamSize(ramSize),
	fLocking(locking),
	fProtection(protection),
	fId(-1)
{
}


// pragma mark - CoreFileImageInfo


CoreFileImageInfo::CoreFileImageInfo(int32 id, int32 type, uint64 initRoutine,
	uint64 termRoutine, uint64 textBase, uint64 textSize, uint64 dataBase,
	uint64 dataSize, int32 deviceId, int64 nodeId, CoreFileAreaInfo* textArea,
	CoreFileAreaInfo* dataArea, const BString& name)
	:
	fId(id),
	fType(type),
	fInitRoutine(initRoutine),
	fTermRoutine(termRoutine),
	fTextBase(textBase),
	fTextSize(textSize),
	fDataBase(dataBase),
	fDataSize(dataSize),
	fDeviceId(deviceId),
	fNodeId(nodeId),
	fTextArea(textArea),
	fDataArea(dataArea),
	fName(name)
{
}


// pragma mark - CoreFileThreadInfo


CoreFileThreadInfo::CoreFileThreadInfo(int32 id, int32 state, int32 priority,
	uint64 stackBase, uint64 stackEnd, const BString& name)
	:
	fId(id),
	fState(state),
	fPriority(priority),
	fStackBase(stackBase),
	fStackEnd(stackEnd),
	fName(name),
	fCpuState(NULL),
	fCpuStateSize(0)
{
}


CoreFileThreadInfo::~CoreFileThreadInfo()
{
	free(fCpuState);
}


bool
CoreFileThreadInfo::SetCpuState(const void* state, size_t size)
{
	free(fCpuState);
	fCpuState = NULL;
	fCpuStateSize = 0;

	if (state != NULL) {
		fCpuState = malloc(size);
		if (fCpuState == NULL)
			return false;
		memcpy(fCpuState, state, size);
		fCpuStateSize = size;
	}

	return true;
}


// pragma mark - CoreFile


CoreFile::CoreFile()
	:
	fElfFile(),
	fTeamInfo(),
	fAreaInfos(32, true),
	fImageInfos(32, true),
	fThreadInfos(32, true)
{
}


CoreFile::~CoreFile()
{
}


status_t
CoreFile::Init(const char* fileName)
{
	status_t error = fElfFile.Init(fileName);
	if (error != B_OK)
		return error;

	if (fElfFile.Is64Bit())
		return _Init<ElfClass64>();
	return _Init<ElfClass32>();
}


const CoreFileThreadInfo*
CoreFile::ThreadInfoForId(int32 id) const
{
	int32 count = fThreadInfos.CountItems();
	for (int32 i = 0; i < count; i++) {
		CoreFileThreadInfo* info = fThreadInfos.ItemAt(i);
		if (info->Id() == id)
			return info;
	}

	return NULL;
}


template<typename ElfClass>
status_t
CoreFile::_Init()
{
	status_t error = _ReadNotes<ElfClass>();
	if (error != B_OK)
		return error;
printf("CoreFile::_Init(): got %" B_PRId32 " areas, %" B_PRId32 " images, %"
B_PRId32 " threads\n", CountAreaInfos(), CountImageInfos(), CountThreadInfos());
	// TODO: Verify that we actually read something!
	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadNotes()
{
	int32 count = fElfFile.CountSegments();
	for (int32 i = 0; i < count; i++) {
		ElfSegment* segment = fElfFile.SegmentAt(i);
		if (segment->Type() == PT_NOTE) {
			status_t error = _ReadNotes<ElfClass>(segment);
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadNotes(ElfSegment* segment)
{
	// read the whole segment into memory
	if ((uint64)segment->FileSize() > kMaxNotesSize) {
		WARNING("Notes segment too large (%" B_PRIdOFF ")\n",
			segment->FileSize());
		return B_UNSUPPORTED;
	}

	size_t notesSize = (size_t)segment->FileSize();
	uint8* notes = (uint8*)malloc(notesSize);
	if (notes == NULL)
		return B_NO_MEMORY;
	MemoryDeleter notesDeleter(notes);

	ssize_t bytesRead = pread(fElfFile.FD(), notes, notesSize,
		(off_t)segment->FileOffset());
	if (bytesRead < 0) {
		WARNING("Failed to read notes segment: %s\n", strerror(errno));
		return errno;
	}
	if ((size_t)bytesRead != notesSize) {
		WARNING("Failed to read whole notes segment\n");
		return B_IO_ERROR;
	}

	// iterate through notes
	typedef typename ElfClass::Nhdr Nhdr;
	while (notesSize > 0) {
		if (notesSize < sizeof(Nhdr)) {
			WARNING("Remaining bytes in notes segment too short for header\n");
			return B_BAD_DATA;
		}

		const Nhdr* header = (const Nhdr*)notes;
		uint32 nameSize = Get(header->n_namesz);
		uint32 dataSize = Get(header->n_descsz);
		uint32 type = Get(header->n_type);

		notes += sizeof(Nhdr);
		notesSize -= sizeof(Nhdr);

		size_t alignedNameSize = (nameSize + 3) / 4 * 4;
		if (alignedNameSize > notesSize) {
			WARNING("Not enough bytes remaining in notes segment for note "
				"name (%zu / %zu)\n", notesSize, alignedNameSize);
			return B_BAD_DATA;
		}

		const char* name = (const char*)notes;
		size_t nameLen = strnlen(name, nameSize);
		if (nameLen == nameSize) {
			WARNING("Unterminated note name\n");
			return B_BAD_DATA;
		}

		notes += alignedNameSize;
		notesSize -= alignedNameSize;

		size_t alignedDataSize = (dataSize + 3) / 4 * 4;
		if (alignedDataSize > notesSize) {
			WARNING("Not enough bytes remaining in notes segment for note "
				"data\n");
			return B_BAD_DATA;
		}

		_ReadNote<ElfClass>(name, type, notes, dataSize);

		notes += alignedDataSize;
		notesSize -= alignedDataSize;
	}

	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadNote(const char* name, uint32 type, const void* data,
	uint32 dataSize)
{
	if (strcmp(name, ELF_NOTE_CORE) == 0) {
		switch (type) {
			case NT_FILE:
				// not needed
				return B_OK;
		}
	} else if (strcmp(name, ELF_NOTE_HAIKU) == 0) {
		switch (type) {
			case NT_TEAM:
				return _ReadTeamNote<ElfClass>(data, dataSize);
			case NT_AREAS:
				return _ReadAreasNote<ElfClass>(data, dataSize);
			case NT_IMAGES:
				return _ReadImagesNote<ElfClass>(data, dataSize);
			case NT_THREADS:
				return _ReadThreadsNote<ElfClass>(data, dataSize);
			break;
		}
	}

	WARNING("Unsupported note type %s/%#" B_PRIx32 "\n", name, type);
	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadTeamNote(const void* data, uint32 dataSize)
{
	typedef typename ElfClass::NoteTeam NoteTeam;

	if (dataSize < sizeof(NoteTeam) + 1) {
		WARNING("Team note too short\n");
		return B_BAD_DATA;
	}
	const NoteTeam* note = (const NoteTeam*)data;

	// check, if args are null-terminated
	const char* args = (const char*)(note + 1);
	size_t argsSize = dataSize - sizeof(NoteTeam);
	if (argsSize == 0 || args[argsSize - 1] != '\0') {
		WARNING("Team note args not terminated\n");
		return B_BAD_DATA;
	}

	int32 id = Get(note->nt_id);
	int32 uid = Get(note->nt_uid);
	int32 gid = Get(note->nt_gid);

	BString copiedArgs(args);
	if (args[0] != '\0' && copiedArgs.Length() == 0)
		return B_NO_MEMORY;

	fTeamInfo.Init(id, uid, gid, copiedArgs);
	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadAreasNote(const void* data, uint32 dataSize)
{
	const size_t addressSize = sizeof(typename ElfClass::Size);
	if (dataSize < addressSize) {
		WARNING("Areas note too short\n");
		return B_BAD_DATA;
	}
	uint64 areaCount = Get(*(const typename ElfClass::Size*)data);

	typedef typename ElfClass::NoteAreaEntry Entry;
	const Entry* table = (Entry*)((const uint8*)data + addressSize);
	dataSize -= addressSize;

	if (areaCount == 0)
		return B_OK;

	// check area count
	if (areaCount > dataSize || areaCount * sizeof(Entry) >= dataSize) {
		WARNING("Areas note too short for area count\n");
		return B_BAD_DATA;
	}

	// check, if strings are null-terminated
	const char* strings = (const char*)(table + areaCount);
	size_t stringsSize = dataSize - areaCount * sizeof(Entry);
	if (stringsSize == 0 || strings[stringsSize - 1] != '\0') {
		WARNING("Areas note strings not terminated\n");
		return B_BAD_DATA;
	}

	for (uint64 i = 0; i < areaCount; i++) {
		// get entry values
		Entry entry;
		memcpy(&entry, table, sizeof(entry));

		int32 id = Get(entry.na_id);
		uint64 baseAddress = Get(entry.na_base);
		uint64 size = Get(entry.na_size);
		uint64 ramSize = Get(entry.na_ram_size);
		uint32 lock = Get(entry.na_lock);
		uint32 protection = Get(entry.na_protection);

		// get name
		if (stringsSize == 0) {
			WARNING("Area %" B_PRIu64 " (ID %#" B_PRIx32 " @ %#" B_PRIx64
				") has no name\n", i, id, baseAddress);
			continue;
		}
		const char* name = strings;
		size_t nameSize = strlen(name) + 1;
		strings += nameSize;
		stringsSize -= nameSize;

		BString copiedName(name);
		if (name[0] != '\0' && copiedName.Length() == 0)
			return B_NO_MEMORY;

		// create and add area
		ElfSegment* segment = _FindAreaSegment(baseAddress);
		CoreFileAreaInfo* area = new(std::nothrow) CoreFileAreaInfo(segment, id,
			baseAddress, size, ramSize, lock, protection, copiedName);
		if (area == NULL || !fAreaInfos.AddItem(area)) {
			delete area;
			return B_NO_MEMORY;
		}

		table++;
	}

	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadImagesNote(const void* data, uint32 dataSize)
{
	const size_t addressSize = sizeof(typename ElfClass::Size);
	if (dataSize < addressSize) {
		WARNING("Images note too short\n");
		return B_BAD_DATA;
	}
	uint64 imageCount = Get(*(const typename ElfClass::Size*)data);

	typedef typename ElfClass::NoteImageEntry Entry;
	const Entry* table = (Entry*)((const uint8*)data + addressSize);
	dataSize -= addressSize;

	if (imageCount == 0)
		return B_OK;

	// check image count
	if (imageCount > dataSize || imageCount * sizeof(Entry) >= dataSize) {
		WARNING("Images note too short for image count\n");
		return B_BAD_DATA;
	}

	// check, if strings are null-terminated
	const char* strings = (const char*)(table + imageCount);
	size_t stringsSize = dataSize - imageCount * sizeof(Entry);
	if (stringsSize == 0 || strings[stringsSize - 1] != '\0') {
		WARNING("Images note strings not terminated\n");
		return B_BAD_DATA;
	}

	for (uint64 i = 0; i < imageCount; i++) {
		// get entry values
		Entry entry;
		memcpy(&entry, table, sizeof(entry));

		int32 id = Get(entry.ni_id);
		int32 type = Get(entry.ni_type);
		uint64 initRoutine = Get(entry.ni_init_routine);
		uint64 termRoutine = Get(entry.ni_term_routine);
		uint64 textBase = Get(entry.ni_text_base);
		uint64 textSize = Get(entry.ni_text_size);
		uint64 dataBase = Get(entry.ni_data_base);
		uint64 dataSize = Get(entry.ni_data_size);
		int32 deviceId = Get(entry.ni_device);
		int64 nodeId = Get(entry.ni_node);

		// get name
		if (stringsSize == 0) {
			WARNING("Image %" B_PRIu64 " (ID %#" B_PRIx32 ") has no name\n",
				i, id);
			continue;
		}
		const char* name = strings;
		size_t nameSize = strlen(name) + 1;
		strings += nameSize;
		stringsSize -= nameSize;

		BString copiedName(name);
		if (name[0] != '\0' && copiedName.Length() == 0)
			return B_NO_MEMORY;

		// create and add image
		CoreFileAreaInfo* textArea = _FindArea(textBase);
		CoreFileAreaInfo* dataArea = _FindArea(dataBase);
		CoreFileImageInfo* image = new(std::nothrow) CoreFileImageInfo(id, type,
			initRoutine, termRoutine, textBase, textSize, dataBase, dataSize,
			deviceId, nodeId, textArea, dataArea, copiedName);
		if (image == NULL || !fImageInfos.AddItem(image)) {
			delete image;
			return B_NO_MEMORY;
		}

		table++;
	}

	return B_OK;
}


template<typename ElfClass>
status_t
CoreFile::_ReadThreadsNote(const void* data, uint32 dataSize)
{
	const size_t addressSize = sizeof(typename ElfClass::Size);
	if (dataSize < 2 * addressSize) {
		WARNING("Threads note too short\n");
		return B_BAD_DATA;
	}

	const typename ElfClass::Size* header
		= (const typename ElfClass::Size*)data;
	uint64 threadCount = Get(header[0]);
	uint64 cpuStateSize = Get(header[1]);

	if (cpuStateSize > 1024 * 1024) {
		WARNING("Threads note: unreasonable CPU state size: %" B_PRIu64 "\n",
			cpuStateSize);
		return B_BAD_DATA;
	}

	typedef typename ElfClass::NoteThreadEntry Entry;
	const uint8* table = (const uint8*)(header + 2);
	dataSize -= 2 * addressSize;

	size_t totalEntrySize = sizeof(Entry) + (size_t)cpuStateSize;

	// check thread count
	if (threadCount > dataSize || threadCount * totalEntrySize >= dataSize) {
		WARNING("Threads note too short for thread count\n");
		return B_BAD_DATA;
	}

	if (threadCount == 0)
		return B_OK;

	// check, if strings are null-terminated
	const char* strings = (const char*)(table + threadCount * totalEntrySize);
	size_t stringsSize = dataSize - threadCount * totalEntrySize;
	if (stringsSize == 0 || strings[stringsSize - 1] != '\0') {
		WARNING("Threads note strings not terminated\n");
		return B_BAD_DATA;
	}

	for (uint64 i = 0; i < threadCount; i++) {
		// get entry values
		Entry entry;
		memcpy(&entry, table, sizeof(entry));

		int32 id = Get(entry.nth_id);
		int32 state = Get(entry.nth_state);
		int32 priority = Get(entry.nth_priority);
		uint64 stackBase = Get(entry.nth_stack_base);
		uint64 stackEnd = Get(entry.nth_stack_end);

		// get name
		if (stringsSize == 0) {
			WARNING("Thread %" B_PRIu64 " (ID %#" B_PRIx32 ") has no name\n",
				i, id);
			continue;
		}
		const char* name = strings;
		size_t nameSize = strlen(name) + 1;
		strings += nameSize;
		stringsSize -= nameSize;

		BString copiedName(name);
		if (name[0] != '\0' && copiedName.Length() == 0)
			return B_NO_MEMORY;

		// create and add thread
		CoreFileThreadInfo* thread = new(std::nothrow) CoreFileThreadInfo(id,
			state, priority, stackBase, stackEnd, copiedName);
		if (thread == NULL || !fThreadInfos.AddItem(thread)) {
			delete thread;
			return B_NO_MEMORY;
		}

		// get CPU state
		const void* cpuState = table + sizeof(Entry);
		if (!thread->SetCpuState(cpuState, cpuStateSize))
			return B_NO_MEMORY;

		table += totalEntrySize;
	}

	return B_OK;
}


CoreFileAreaInfo*
CoreFile::_FindArea(uint64 address) const
{
	int32 count = fAreaInfos.CountItems();
	for (int32 i = 0; i < count; i++) {
		CoreFileAreaInfo* area = fAreaInfos.ItemAt(i);
		if (address >= area->BaseAddress()
				&& address < area->EndAddress()) {
			return area;
		}
	}

	return NULL;
}


ElfSegment*
CoreFile::_FindAreaSegment(uint64 address) const
{
	int32 count = fElfFile.CountSegments();
	for (int32 i = 0; i < count; i++) {
		ElfSegment* segment = fElfFile.SegmentAt(i);
		if (segment->Type() == PT_LOAD && segment->LoadAddress() == address)
			return segment;
	}

	return NULL;
}


