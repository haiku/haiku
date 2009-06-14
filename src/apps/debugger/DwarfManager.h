/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_MANAGER_H
#define DWARF_MANAGER_H

#include <sys/types.h>

#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>


class DwarfManager {
public:
								DwarfManager();
								~DwarfManager();

			status_t			Init();

			status_t			LoadFile(const char* fileName);
			status_t			FinishLoading();

private:
			struct CompilationUnit;
			struct File;
			typedef DoublyLinkedList<File> FileList;

private:
			FileList			fFiles;
};



#endif	// DWARF_MANAGER_H
