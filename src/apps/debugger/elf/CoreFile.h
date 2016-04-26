/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CORE_FILE_H
#define CORE_FILE_H


#include "ElfFile.h"

#include <String.h>


struct CoreFileTeamInfo {
								CoreFileTeamInfo();
			void				Init(int32 id, int32 uid, int32 gid,
									const BString& args);

			int32				Id() const			{ return fId; }
			const BString&		Arguments() const	{ return fArgs; }

private:
			int32				fId;
			int32				fUid;
			int32				fGid;
			BString				fArgs;
};


struct CoreFileAreaInfo {
								CoreFileAreaInfo(ElfSegment* segment, int32 id,
									uint64 baseAddress, uint64 size,
									uint64 ramSize, uint32 locking,
									uint32 protection, const BString& name);

			uint64				BaseAddress() const	{ return fBaseAddress; }
			uint64				Size() const		{ return fSize; }
			uint64				EndAddress() const
									{ return fBaseAddress + fSize; }
			uint32				Locking() const		{ return fLocking; }
			uint32				Protection() const	{ return fProtection; }

			ElfSegment*			Segment() const		{ return fSegment; }

private:
			ElfSegment*			fSegment;
			uint64				fBaseAddress;
			uint64				fSize;
			uint64				fRamSize;
			uint32				fLocking;
			uint32				fProtection;
			int32				fId;
			BString				fName;
};


struct CoreFileImageInfo {
								CoreFileImageInfo(int32 id, int32 type,
									uint64 initRoutine, uint64 termRoutine,
									uint64 textBase, uint64 textSize,
									uint64 dataBase, uint64 dataSize,
									int32 deviceId, int64 nodeId,
									CoreFileAreaInfo* textArea,
									CoreFileAreaInfo* dataArea,
									const BString& name);

			int32				Id() const			{ return fId; }
			int32				Type() const		{ return fType; }
			uint64				TextBase() const	{ return fTextBase; }
			uint64				TextSize() const	{ return fTextSize; }
			uint64				DataBase() const	{ return fDataBase; }
			uint64				DataSize() const	{ return fDataSize; }
			const BString&		Name() const		{ return fName; }

private:
			int32				fId;
			int32				fType;
			uint64				fInitRoutine;
			uint64				fTermRoutine;
			uint64				fTextBase;
			uint64				fTextSize;
			uint64				fDataBase;
			uint64				fDataSize;
			int32				fDeviceId;
			int64				fNodeId;
			CoreFileAreaInfo*	fTextArea;
			CoreFileAreaInfo*	fDataArea;
			BString				fName;
};


struct CoreFileThreadInfo {
								CoreFileThreadInfo(int32 id, int32 state,
									int32 priority, uint64 stackBase,
									uint64 stackEnd, const BString& name);
								~CoreFileThreadInfo();

			bool				SetCpuState(const void* state, size_t size);
			const void*			GetCpuState() const
									{ return fCpuState; }
			size_t				CpuStateSize() const
									{ return fCpuStateSize; }

			int32				Id() const		{ return fId; }
			const BString&		Name() const	{ return fName; }

private:
			int32				fId;
			int32				fState;
			int32				fPriority;
			uint64				fStackBase;
			uint64				fStackEnd;
			BString				fName;
			void*				fCpuState;
			size_t				fCpuStateSize;
};


class CoreFile {
public:
								CoreFile();
								~CoreFile();

			status_t			Init(const char* fileName);

			ElfFile&			GetElfFile()
									{ return fElfFile; }

			const CoreFileTeamInfo& GetTeamInfo() const
									{ return fTeamInfo; }

			int32				CountAreaInfos() const
									{ return fAreaInfos.CountItems(); }
			const CoreFileAreaInfo* AreaInfoAt(int32 index) const
									{ return fAreaInfos.ItemAt(index); }
			const CoreFileAreaInfo* AreaInfoForAddress(uint64 address) const
									{ return _FindArea(address); }

			int32				CountImageInfos() const
									{ return fImageInfos.CountItems(); }
			const CoreFileImageInfo* ImageInfoAt(int32 index) const
									{ return fImageInfos.ItemAt(index); }

			int32				CountThreadInfos() const
									{ return fThreadInfos.CountItems(); }
			const CoreFileThreadInfo* ThreadInfoAt(int32 index) const
									{ return fThreadInfos.ItemAt(index); }
			const CoreFileThreadInfo* ThreadInfoForId(int32 id) const;

private:
			typedef BObjectList<CoreFileAreaInfo> AreaInfoList;
			typedef BObjectList<CoreFileImageInfo> ImageInfoList;
			typedef BObjectList<CoreFileThreadInfo> ThreadInfoList;

private:
			template<typename ElfClass>
			status_t			_Init();

			template<typename ElfClass>
			status_t			_ReadNotes();
			template<typename ElfClass>
			status_t			_ReadNotes(ElfSegment* segment);
			template<typename ElfClass>
			status_t			_ReadNote(const char* name, uint32 type,
									const void* data, uint32 dataSize);
			template<typename ElfClass>
			status_t			_ReadTeamNote(const void* data,
									uint32 dataSize);
			template<typename ElfClass>
			status_t			_ReadAreasNote(const void* data,
									uint32 dataSize);
			template<typename ElfClass>
			status_t			_ReadImagesNote(const void* data,
									uint32 dataSize);
			template<typename ElfClass>
			status_t			_ReadThreadsNote(const void* data,
									uint32 dataSize);

			CoreFileAreaInfo*	_FindArea(uint64 address) const;
			ElfSegment*			_FindAreaSegment(uint64 address) const;

			template<typename Type>
			Type				_ReadValue(const void*& data, uint32& dataSize);
			template<typename Entry>
			void				_ReadEntry(const void*& data, uint32& dataSize,
									Entry& entry, size_t entrySize);
			void				_Advance(const void*& data, uint32& dataSize,
									size_t by);

			template<typename Value>
			Value				Get(const Value& value) const
									{ return fElfFile.Get(value); }

private:
			ElfFile				fElfFile;
			CoreFileTeamInfo	fTeamInfo;
			AreaInfoList		fAreaInfos;
			ImageInfoList		fImageInfos;
			ThreadInfoList		fThreadInfos;
};


#endif	// CORE_FILE_H
