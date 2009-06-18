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


class Team;


class Image : public Referenceable, public DoublyLinkedListLinkImpl<Image> {
public:
								Image(Team* team, const ImageInfo& imageInfo);
								~Image();

			status_t			Init();


			Team*				GetTeam() const	{ return fTeam; }
			image_id			ID() const		{ return fInfo.ImageID(); }
			const char*			Name() const	{ return fInfo.Name(); }
			const ImageInfo&	Info() const	{ return fInfo; }

private:
			Team*				fTeam;
			ImageInfo			fInfo;
};


typedef DoublyLinkedList<Image> ImageList;


#endif	// IMAGE_H
