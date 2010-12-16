/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include "SharedImage.h"


class Image : public BReferenceable {
public:
								Image(SharedImage* image,
									const image_info& info, team_id owner,
									int32 creationEvent);
								~Image();

	inline	SharedImage*		GetSharedImage() const	{ return fImage; }

	inline	const image_id		ID() const;
	inline	const char*			Name() const;
	inline	team_id				Owner() const;
	inline	addr_t				LoadDelta() const		{ return fLoadDelta; }

	inline	int32				CreationEvent() const;
	inline	int32				DeletionEvent() const;
	inline	void				SetDeletionEvent(int32 event);

	inline	Symbol**			Symbols() const;
	inline	int32				SymbolCount() const;

	inline	bool				ContainsAddress(addr_t address) const;
	inline	int32				FindSymbol(addr_t address) const;

private:
			SharedImage*		fImage;
			image_id			fID;
			team_id				fOwner;
			addr_t				fLoadDelta;
			int32				fCreationEvent;
			int32				fDeletionEvent;
};


// #pragma mark -


const image_id
Image::ID() const
{
	return fID;
}


const char*
Image::Name() const
{
	return fImage->Name();
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
	return fImage->Symbols();
}


int32
Image::SymbolCount() const
{
	return fImage->SymbolCount();
}


bool
Image::ContainsAddress(addr_t address) const
{
	return fImage->ContainsAddress(address - fLoadDelta);
}


int32
Image::FindSymbol(addr_t address) const
{
	return fImage->FindSymbol(address - fLoadDelta);
}


#endif	// IMAGE_H
