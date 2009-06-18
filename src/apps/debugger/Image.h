/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <image.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Team;


class Image : public Referenceable, public DoublyLinkedListLinkImpl<Image> {
public:
								Image(Team* team, const image_info& imageInfo);
								~Image();

			status_t			Init();

			
			Team*				GetTeam() const	{ return fTeam; }
			image_id			ID() const		{ return fInfo.id; }
			const char*			Name() const	{ return fInfo.name; }
			const image_info&	Info() const	{ return fInfo; }

private:
			Team*				fTeam;
			image_info			fInfo;
};


typedef DoublyLinkedList<Image> ImageList;


#endif	// IMAGE_H
