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
	DBGMSG(("construct PictureData\n"));
	DBGMSG(("1: current seek position = 0x%x\n", (int)file->Position()));

	file->Read(&point,  sizeof(BPoint));
	file->Read(&rect,   sizeof(BRect));

	picture = new BPicture();

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


PageData::PageData()
{
	fHollow = true;
}


PageData::PageData(BFile *file, bool reverse)
{
	fFile    = file;
	fReverse = reverse;
	fPictureCount = 0;
	fRest    = 0;
	fOffset  = 0;
	fHollow  = false;

	if (reverse) {
		file->Read(&fPictureCount, sizeof(int32));
		DBGMSG(("picture_count = %" B_PRId32 "\n", fPictureCount));
		fOffset = fFile->Position();
		off_t o = fOffset;
		// seek to start of next page
		fFile->Read(&o, sizeof(o));
		fFile->Seek(o, SEEK_SET);
	}
}


bool
PageData::startEnum()
{
	off_t offset;
	uchar dummy[40];

	if (fHollow)
		return false;

	if (fOffset == 0) {
		fFile->Read(&fPictureCount, sizeof(int32));
		DBGMSG(("picture_count = %" B_PRId32 "\n", fPictureCount));
		fOffset = fFile->Position();
	} else {
		fFile->Seek(fOffset, SEEK_SET);
	}
	// skip page header
	fFile->Seek(sizeof(offset) + sizeof(dummy), SEEK_CUR);

	fRest = fPictureCount;
	return fPictureCount > 0;
}


bool
PageData::enumObject(PictureData **picture_data)
{
	if (fHollow || fPictureCount <= 0) {
		*picture_data = NULL;
	} else {
		*picture_data = new PictureData(fFile);
		if (--fRest > 0) {
			return true;
		}
	}
	return false;
}


SpoolData::SpoolData(BFile *file, int32 page_count, int32 nup, bool reverse)
{
	DBGMSG(("nup        = %" B_PRId32 "\n", nup));
	DBGMSG(("page_count = %" B_PRId32 "\n", page_count));
	DBGMSG(("reverse    = %s\n", reverse ? "true" : "false"));

	if (reverse) {
		if (nup > 1) {
			for (int32 page_index = 0; page_index < page_count; page_index++) {
				if (page_index % nup == 0) {
					fPages.push_front(new PageData(file, reverse));
					fIt = fPages.begin();
					fIt++;
				} else {
					fPages.insert(fIt, new PageData(file, reverse));
				}
			}
			page_count = nup - page_count % nup;
			if (page_count < nup) {
				while (page_count--) {
					fPages.insert(fIt, new PageData);
				}
			}
		} else {
			while (page_count--) {
				fPages.push_front(new PageData(file, reverse));
			}
		}
	} else {
		while (page_count--) {
			fPages.push_back(new PageData(file, reverse));
		}
	}
}


SpoolData::~SpoolData()
{
	for (fIt = fPages.begin(); fIt != fPages.end(); fIt++) {
		delete (*fIt);
	}
}


bool
SpoolData::startEnum()
{
	fIt = fPages.begin();
	return true;
}


bool
SpoolData::enumObject(PageData **page_data)
{
	*page_data = *fIt++;
	if (fIt == fPages.end()) {
		return false;
	}
	return true;
}
