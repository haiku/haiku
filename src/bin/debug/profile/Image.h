/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <image.h>
#include <OS.h>
#include <String.h>

#include "Referenceable.h"


class debug_symbol_lookup_context;
class Image;


class Symbol {
public:
	Symbol(Image* image, addr_t base, size_t size, const char* name)
		:
		image(image),
		base(base),
		size(size),
		name(name)
	{
	}

	const char* Name() const	{ return name.String(); }

	Image*	image;
	addr_t	base;
	size_t	size;
	BString	name;
};


struct SymbolComparator {
	inline bool operator()(const Symbol* a, const Symbol* b) const
	{
		return a->base < b->base;
	}
};


class Image : public Referenceable {
public:
								Image(const image_info& info, team_id owner,
									int32 creationEvent);
								~Image();

	inline	const image_id		ID() const;
	inline	const image_info&	Info() const;
	inline	team_id				Owner() const;

	inline	int32				CreationEvent() const;
	inline	int32				DeletionEvent() const;
	inline	void				SetDeletionEvent(int32 event);

			status_t			LoadSymbols(
									debug_symbol_lookup_context* lookupContext);

	inline	Symbol**			Symbols() const;
	inline	int32				SymbolCount() const;

	inline	bool				ContainsAddress(addr_t address) const;
			int32				FindSymbol(addr_t address) const;

private:
			image_info			fInfo;
			team_id				fOwner;
			Symbol**			fSymbols;
			int32				fSymbolCount;
			int32				fCreationEvent;
			int32				fDeletionEvent;
};


// #pragma mark -


const image_id
Image::ID() const
{
	return fInfo.id;
}


const image_info&
Image::Info() const
{
	return fInfo;
}


team_id
Image::Owner() const
{
	return fOwner;
}


int32
Image::CreationEvent() const
{
	return fCreationEvent;
}


int32
Image::DeletionEvent() const
{
	return fDeletionEvent;
}


void
Image::SetDeletionEvent(int32 event)
{
	fDeletionEvent = event;
}


Symbol**
Image::Symbols() const
{
	return fSymbols;
}


int32
Image::SymbolCount() const
{
	return fSymbolCount;
}


bool
Image::ContainsAddress(addr_t address) const
{
	return address >= (addr_t)fInfo.text
		&& address <= (addr_t)fInfo.data + fInfo.data_size - 1;
}


#endif	// IMAGE_H
