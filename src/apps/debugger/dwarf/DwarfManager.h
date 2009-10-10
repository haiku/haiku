/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_MANAGER_H
#define DWARF_MANAGER_H

#include <Locker.h>

#include <util/DoublyLinkedList.h>


class DwarfFile;


class DwarfManager {
public:
								DwarfManager();
								~DwarfManager();

			status_t			Init();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			status_t			LoadFile(const char* fileName,
									DwarfFile*& _file);
									// returns a reference
			status_t			FinishLoading();

private:
			typedef DoublyLinkedList<DwarfFile> FileList;

private:
			BLocker				fLock;
			FileList			fFiles;
};



#endif	// DWARF_MANAGER_H
