/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>

#include <image.h>
#include <OS.h>

#include <util/DoublyLinkedList.h>


struct image_t;
struct runtime_loader_debug_area;
struct Elf32_Sym;


namespace BPrivate {
namespace Debug {


class Image : public DoublyLinkedListLinkImpl<Image> {
public:
								Image();
	virtual						~Image();

	virtual	image_id			ID() const = 0;
	virtual	const char*			Name() const = 0;
	virtual	addr_t				TextAddress() const = 0;
	virtual	size_t				TextSize() const = 0;

	virtual	const Elf32_Sym*	LookupSymbol(addr_t address,
									addr_t* _baseAddress,
									const char** _symbolName,
									size_t *_symbolNameLen,
									bool *_exactMatch) const = 0;
	virtual	status_t			NextSymbol(int32& iterator,
									const char** _symbolName,
									size_t* _symbolNameLen,
									addr_t* _symbolAddress, size_t* _symbolSize,
									int32* _symbolType) const = 0;
};


class SymbolTableBasedImage : public Image {
public:
								SymbolTableBasedImage();
	virtual						~SymbolTableBasedImage();

	virtual	image_id			ID() const ;
	virtual	const char*			Name() const;
	virtual	addr_t				TextAddress() const;
	virtual	size_t				TextSize() const;

	virtual	const Elf32_Sym*	LookupSymbol(addr_t address,
									addr_t* _baseAddress,
									const char** _symbolName,
									size_t *_symbolNameLen,
									bool *_exactMatch) const;
	virtual	status_t			NextSymbol(int32& iterator,
									const char** _symbolName,
									size_t* _symbolNameLen,
									addr_t* _symbolAddress, size_t* _symbolSize,
									int32* _symbolType) const;

protected:
			size_t				_SymbolNameLen(const char* symbolName) const;

protected:
			image_info			fInfo;
			addr_t				fLoadDelta;
			Elf32_Sym*			fSymbolTable;
			char*				fStringTable;
			int32				fSymbolCount;
			size_t				fStringTableSize;
};


class ImageFile : public SymbolTableBasedImage {
public:
								ImageFile();
	virtual						~ImageFile();

			status_t			Init(const image_info& info);

private:
			int					fFD;
			off_t				fFileSize;
			uint8*				fMappedFile;
};


class KernelImage : public SymbolTableBasedImage {
public:
								KernelImage();
	virtual						~KernelImage();

			status_t			Init(const image_info& info);
};

}	// namespace Debug
}	// namespace BPrivate


using BPrivate::Debug::ImageFile;


#endif	// IMAGE_H
