/*
 * PrintProcess.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <File.h>
#include <Picture.h>
#include <unistd.h>

#include "PrintProcess.h"
#include "DbgMsg.h"

PictureData::PictureData(BFile *file)
{
	off_t offset;
	uchar dummy[40];

	DBGMSG(("construct PictureData\n"));
	DBGMSG(("1: current seek position = 0x%x\n", (int)file->Position()));

	file->Read(&offset, sizeof(offset));
	file->Read(dummy,   sizeof(dummy));
	file->Read(&point,  sizeof(BPoint));
	file->Read(&rect,   sizeof(BRect));

	picture = new BPicture();

	DBGMSG(("next picture position = 0x%x\n", (int)offset));
	DBGMSG(("picture_data::point = %f, %f\n", point.x, point.y));
	DBGMSG(("picture_data::rect = %f, %f, %f, %f\n",
		rect.left, rect.top, rect.right, rect.bottom));
	DBGMSG(("2: current seek position = 0x%x\n", (int)file->Position()));

	picture->Unflatten(file);

	DBGMSG(("3: current seek position = 0x%x\n", (int)file->Position()));
}

PictureData::~PictureData()
{
	delete picture;
}

/*--------------------------------------------------*/

PageData::PageData()
{
	__hollow = true;
}

PageData::PageData(BFile *file, bool reverse)
{
	__file    = file;
	__reverse = reverse;
	__picture_count = 0;
	__rest    = 0;
	__offset  = 0;
	__hollow  = false;

	if (reverse) {
		file->Read(&__picture_count, sizeof(long));
		DBGMSG(("picture_count = %d\n", (int)__picture_count));
		__offset = __file->Position();
		off_t o = __offset;
		for (long i = 0; i < __picture_count; i++) {
			__file->Read(&o, sizeof(o));
			__file->Seek(o, SEEK_SET);
		}
	}
}

bool PageData::startEnum()
{
	if (__hollow)
		return false;

	if (__offset == 0) {
		__file->Read(&__picture_count, sizeof(long));
		DBGMSG(("picture_count = %d\n", (int)__picture_count));
		__offset = __file->Position();
	} else {
		__file->Seek(__offset, SEEK_SET);
	}

	__rest = __picture_count;
	return __picture_count > 0;
}

bool PageData::enumObject(PictureData **picture_data)
{
	if (__hollow || __picture_count <= 0) {
		*picture_data = NULL;
	} else {
		*picture_data = new PictureData(__file);
		if (--__rest > 0) {
			return true;
		}
	}
	return false;
}

/*--------------------------------------------------*/

SpoolData::SpoolData(
	BFile *file,
	int   page_count,
	int   nup,
	bool  reverse)
{
	DBGMSG(("nup        = %d\n", nup));
	DBGMSG(("page_count = %d\n", page_count));
	DBGMSG(("reverse    = %s\n", reverse ? "true" : "false"));

	if (reverse) {
		if (nup > 1) {
			for (int page_index = 0; page_index < page_count; page_index++) {
				if (page_index % nup == 0) {
					__pages.push_front(new PageData(file, reverse));
					__it = __pages.begin();
					__it++;
				} else {
					__pages.insert(__it, new PageData(file, reverse));
				}
			}
			page_count = nup - page_count % nup;
			if (page_count < nup) {
				while (page_count--) {
					__pages.insert(__it, new PageData);
				}
			}
		} else {
			while (page_count--) {
				__pages.push_front(new PageData(file, reverse));
			}
		}
	} else {
		while (page_count--) {
			__pages.push_back(new PageData(file, reverse));
		}
	}
}

SpoolData::~SpoolData()
{
	for (__it = __pages.begin(); __it != __pages.end(); __it++) {
		delete (*__it);
	}
}

bool SpoolData::startEnum()
{
	__it = __pages.begin();
	return true;
}

bool SpoolData::enumObject(PageData **page_data)
{
	*page_data = *__it++;
	if (__it == __pages.end()) {
		return false;
	}
	return true;
}
