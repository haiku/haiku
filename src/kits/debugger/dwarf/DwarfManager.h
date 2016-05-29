/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_MANAGER_H
#define DWARF_MANAGER_H

#include <Locker.h>

#include <util/DoublyLinkedList.h>


class DwarfFile;
struct DwarfFileLoadingState;


class DwarfManager {
public:
								DwarfManager(uint8 addressSize);
								~DwarfManager();

			status_t			Init();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			status_t			LoadFile(const char* fileName,
									DwarfFileLoadingState& _loadingState);
									// _loadingState receives a reference
									// to the corresponding DwarfFile.

			status_t			FinishLoading();

private:
			typedef DoublyLinkedList<DwarfFile> FileList;

private:
			uint8				fAddressSize;
			BLocker				fLock;
			FileList			fFiles;
};



#endif	// DWARF_MANAGER_H
