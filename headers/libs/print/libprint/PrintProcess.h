/*
 * PrintProcess.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PRINTPROCESS_H
#define __PRINTPROCESS_H

#include <vector>
#include <list>
#include <memory>

#include <Rect.h>
#include <Point.h>


using namespace std;


class BFile;
class BPicture;

class PictureData {
public:
	PictureData(BFile *file);
	~PictureData();
	BPoint   point;
	BRect    rect;
	BPicture *picture;
};

class PageData {
public:
	PageData();
	PageData(BFile *file, bool reverse);
	bool startEnum();
	bool enumObject(PictureData **);

private:
	BFile *fFile;
	bool  fReverse;
	int32 fPictureCount;
	int32 fRest;
	off_t fOffset;
	bool  fHollow;
};

typedef list<PageData *>	PageDataList;

class SpoolData {
public:
	SpoolData(BFile *file, int32 page_count, int32 nup, bool reverse);
	~SpoolData();
	bool startEnum();
	bool enumObject(PageData **);

private:
	PageDataList fPages;
	PageDataList::iterator fIt;
};

#endif	/* __PRINTPROCESS_H */
