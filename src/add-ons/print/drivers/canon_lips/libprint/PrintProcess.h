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

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

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
	BFile *__file;
	bool  __reverse;
	int   __picture_count;
	int   __rest;
	off_t __offset;
	bool  __hollow;
};

typedef list<PageData *>	PageDataList;

class SpoolData {
public:
	SpoolData(BFile *file, int page_count, int nup, bool reverse);
	~SpoolData();
	bool startEnum();
	bool enumObject(PageData **);

private:
	PageDataList __pages;
	PageDataList::iterator __it;
};

#endif	/* __PRINTPROCESS_H */
