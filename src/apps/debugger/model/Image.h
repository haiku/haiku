/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <image.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "ImageInfo.h"


enum image_debug_info_state {
	IMAGE_DEBUG_INFO_NOT_LOADED,
	IMAGE_DEBUG_INFO_LOADING,
	IMAGE_DEBUG_INFO_LOADED,
	IMAGE_DEBUG_INFO_UNAVAILABLE
};


class ImageDebugInfo;
class LocatableFile;
class Team;


class Image : public BReferenceable, public DoublyLinkedListLinkImpl<Image> {
public:
								Image(Team* team, const ImageInfo& imageInfo,
									LocatableFile* imageFile);
								~Image();

			status_t			Init();

			Team*				GetTeam() const		{ return fTeam; }
			image_id			ID() const			{ return fInfo.ImageID(); }
			const BString&		Name() const		{ return fInfo.Name(); }
			const ImageInfo&	Info() const		{ return fInfo; }
			image_type			Type() const		{ return fInfo.Type(); }
			LocatableFile*		ImageFile() const	{ return fImageFile; }

			bool				ContainsAddress(target_addr_t address) const;

			// mutable attributes follow (locking required)
			ImageDebugInfo*		GetImageDebugInfo() const { return fDebugInfo; }
			image_debug_info_state ImageDebugInfoState() const
									{ return fDebugInfoState; }
			status_t			SetImageDebugInfo(ImageDebugInfo* debugInfo,
									image_debug_info_state state);

private:
			Team*				fTeam;
			ImageInfo			fInfo;
			LocatableFile*		fImageFile;
			// mutable
			ImageDebugInfo*		fDebugInfo;
			image_debug_info_state fDebugInfoState;
};


typedef DoublyLinkedList<Image> ImageList;


#endif	// IMAGE_H
