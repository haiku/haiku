/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <image.h>

#include <util/DoublyLinkedList.h>


class Image : public DoublyLinkedListLinkImpl<Image> {
public:
								Image(const image_info& imageInfo);
								~Image();

			status_t			Init();

			image_id			ID() const		{ return fInfo.id; }
			const char*			Name() const	{ return fInfo.name; }
			const image_info&	Info() const	{ return fInfo; }

private:
			image_info			fInfo;
};


typedef DoublyLinkedList<Image> ImageList;


#endif	// IMAGE_H
