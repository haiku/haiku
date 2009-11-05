//------------------------------------------------------------------------------
//	Copyright (c) 2003, Ingo Weinhold
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ElfFile.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Interface declarations of classes for accessing ELF file,
//					or more precisely for iterating through their relocation
//					sections.
//------------------------------------------------------------------------------

#ifndef ELF_FILE_H
#define ELF_FILE_H

#include <File.h>
#include <image.h>

#include "Elf.h"


namespace SymbolPatcher {

class ElfFile;
class ElfSection;

// ElfSymbol
class ElfSymbol {
public:
								ElfSymbol(ElfSection* section = NULL,
										  int32 index = -1);
								~ElfSymbol();

			void				SetTo(ElfSection* section, int32 index);
			void				Unset();

			const Elf32_Sym*	GetSymbolStruct();
			const char*			GetName();
			uint32				GetBinding();
			uint32				GetType();
			uint32				GetTargetSectionIndex();

private:
			ElfSection*			fSection;
			int32				fIndex;
			Elf32_Sym*			fSymbol;
};

// ElfRelocation
class ElfRelocation {
public:
								ElfRelocation(ElfSection* section = NULL,
											  int32 index = -1);
								~ElfRelocation();

			void				SetTo(ElfSection* section, int32 index);
			void				Unset();

			Elf32_Rel*			GetRelocationStruct();
			uint32				GetType();
			uint32				GetSymbolIndex();
			Elf32_Addr			GetOffset();
			status_t			GetSymbol(ElfSymbol* symbol);

private:
			ElfSection*			fSection;
			int32				fIndex;
			Elf32_Rel*			fRelocation;
};

// ElfRelocationIterator
class ElfRelocationIterator {
public:
								ElfRelocationIterator(ElfFile* file);
								~ElfRelocationIterator();

			bool				GetNext(ElfRelocation* relocation);

private:
			ElfSection*			_FindNextSection();

private:
			ElfFile*			fFile;
			int32				fSectionIndex;
			int32				fEntryIndex;
};

// ElfFile
class ElfFile {
public:
								ElfFile();
								~ElfFile();
			status_t			SetTo(const char *filename);
			void				Unset();
			void				Unload();

			BFile*				GetFile()	{ return &fFile; }

			const char*			GetSectionHeaderStrings(size_t* size);
			const char*			GetStringSectionStrings(int32 index,
														size_t* size);

			int32				CountSections() const { return fSectionCount; }
			ElfSection*			SectionAt(int32 index, bool load = false);

			void				Dump();

private:
			status_t			_SetTo(const char *filename);

			Elf32_Shdr*			_SectionHeaderAt(int32 index);

			status_t			_LoadSection(int32 index);

private:
			BFile				fFile;
			Elf32_Ehdr			fHeader;
			uint8*				fSectionHeaders;
			ElfSection*			fSections;
			int32				fSectionCount;
			size_t				fSectionHeaderSize;
};

} // namespace SymbolPatcher

using namespace SymbolPatcher;


#endif	// ELF_FILE_H
