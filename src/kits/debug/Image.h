/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>

#include <elf_common.h>
#include <image.h>
#include <OS.h>

#include <util/DoublyLinkedList.h>


struct image_t;
struct runtime_loader_debug_area;


namespace BPrivate {
namespace Debug {


class Image : public DoublyLinkedListLinkImpl<Image> {
public:
								Image();
	virtual						~Image();

			const image_info&	Info() const		{ return fInfo; }
			image_id			ID() const			{ return fInfo.id; }
			const char*			Name() const		{ return fInfo.name; }
			addr_t				TextAddress() const
				{ return (addr_t)fInfo.text; }
			size_t				TextSize() const	{ return fInfo.text_size; }

	virtual	const elf_sym*		LookupSymbol(addr_t address,
									addr_t* _baseAddress,
									const char** _symbolName,
									size_t *_symbolNameLen,
									bool *_exactMatch) const = 0;
	virtual	status_t			NextSymbol(int32& iterator,
									const char** _symbolName,
									size_t* _symbolNameLen,
									addr_t* _symbolAddress, size_t* _symbolSize,
									int32* _symbolType) const = 0;

	virtual	status_t			GetSymbol(const char* name, int32 symbolType,
									void** _symbolLocation, size_t* _symbolSize,
									int32* _symbolType) const;

protected:
			image_info			fInfo;
};


class SymbolTableBasedImage : public Image {
public:
								SymbolTableBasedImage();
	virtual						~SymbolTableBasedImage();

	virtual	const elf_sym*		LookupSymbol(addr_t address,
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
			addr_t				fLoadDelta;
			elf_sym*			fSymbolTable;
			char*				fStringTable;
			int32				fSymbolCount;
			size_t				fStringTableSize;
};


class ImageFile : public SymbolTableBasedImage {
public:
								ImageFile();
	virtual						~ImageFile();

			status_t			Init(const image_info& info);
			status_t			Init(const char* path);

private:
			status_t			_LoadFile(const char* path,
									addr_t* _textAddress, size_t* _textSize,
									addr_t* _dataAddress, size_t* _dataSize);

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
