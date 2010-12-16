/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SHARED_IMAGE_H
#define SHARED_IMAGE_H

#include <image.h>
#include <OS.h>
#include <String.h>

#include "Referenceable.h"


class debug_symbol_iterator;
class debug_symbol_lookup_context;
class SharedImage;


class Symbol {
public:
	Symbol(SharedImage* image, addr_t base, size_t size, const char* name)
		:
		image(image),
		base(base),
		size(size),
		name(name)
	{
	}

	const char* Name() const	{ return name.String(); }

	SharedImage*	image;
	addr_t			base;
	size_t			size;
	BString			name;
};


struct SymbolComparator {
	inline bool operator()(const Symbol* a, const Symbol* b) const
	{
		return a->base < b->base;
	}
};


class SharedImage : public BReferenceable {
public:
								SharedImage();
								~SharedImage();

			status_t			Init(team_id owner, image_id imageID);
			status_t			Init(const char* path);

	inline	const char*			Name() const;
	inline	const image_info&	Info() const;

	inline	Symbol**			Symbols() const;
	inline	int32				SymbolCount() const;

	inline	bool				ContainsAddress(addr_t address) const;
			int32				FindSymbol(addr_t address) const;

private:
			status_t			_Init(debug_symbol_iterator* iterator);

private:
			image_info			fInfo;
			Symbol**			fSymbols;
			int32				fSymbolCount;
};


// #pragma mark -


const char*
SharedImage::Name() const
{
	return fInfo.name;
}


const image_info&
SharedImage::Info() const
{
	return fInfo;
}


Symbol**
SharedImage::Symbols() const
{
	return fSymbols;
}


int32
SharedImage::SymbolCount() const
{
	return fSymbolCount;
}


bool
SharedImage::ContainsAddress(addr_t address) const
{
	return address >= (addr_t)fInfo.text
		&& address <= (addr_t)fInfo.data + fInfo.data_size - 1;
}


#endif	// SHARED_IMAGE_H
