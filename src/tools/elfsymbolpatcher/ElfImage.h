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
//	File Name:		ElfImage.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Interface declaration of ElfImage, a class encapsulating
//					a loaded ELF image, providing support for accessing the
//					image's symbols and their relocation entries.
//------------------------------------------------------------------------------

#ifndef ELF_IMAGE_H
#define ELF_IMAGE_H

#include <File.h>
#include <image.h>

#include "ElfFile.h"

class BList;

namespace SymbolPatcher {

class ElfSection;
class ElfSymbol;

// ElfImage
class ElfImage {
public:
								ElfImage();
								~ElfImage();
			status_t			SetTo(image_id image);
			void				Unset();
			void				Unload();

			image_id			GetID() const	{ return fImage; }

			status_t			FindSymbol(const char* symbolName,
										   void** address);
			status_t			GetSymbolRelocations(const char* symbolName,
													 BList* relocations);

private:
			status_t			_SetTo(image_id image);

private:
			image_id			fImage;
			ElfFile				fFile;
			uint8*				fTextAddress;
			uint8*				fDataAddress;
			uint8*				fGotAddress;
};

} // namespace SymbolPatcher

using namespace SymbolPatcher;


#endif	// ELF_IMAGE_H
