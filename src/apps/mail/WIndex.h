/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _WORD_INDEX_H
#define _WORD_INDEX_H


#include <DataIO.h>
#include <String.h>


struct WIndexHead {
	int32 entries;
	int32 entrySize;
	int32 offset;	
};


struct WIndexEntry {
	int32 key;
	int32 offset;
};


class FileEntry : public BString {
public:
							FileEntry();
							FileEntry(const char* entryStr);
	virtual					~FileEntry();
};


class WIndex {
public:
							WIndex(BPositionIO* dataFile, int32 count = 100);
							WIndex(int32 count = 100);
	virtual					~WIndex();

			status_t		InitIndex();
			status_t		UnflattenIndex(BPositionIO* io);
			status_t		FlattenIndex(BPositionIO* io);

			int32			Lookup(int32 key);

	inline	WIndexEntry*	ItemAt(int32 index) {
								return (WIndexEntry*)
									(fEntryList + (index * fEntrySize));
							}

			status_t		AddItem(WIndexEntry* entry);
	inline	int32			CountItems() {
								return fEntries;
							}
	void SortItems();

	virtual	int32			GetKey(const char* s);
	virtual	char*			NormalizeWord(const char* word, char* dest);
		
			status_t		SetTo(BPositionIO* dataFile);
			status_t		SetTo(const char* dataPath, const char* indexPath);
			void			Unset();

	virtual	status_t		BuildIndex() = 0;

	virtual	int32			FindFirst(const char* word);
	virtual	FileEntry*		GetEntry(int32 index);
			FileEntry*		GetEntry(const char* word);

protected:
			status_t		_BlockCheck();
	virtual	size_t			_GetEntrySize(WIndexEntry* entry,
								const char* entryData);

			int32			fEntrySize;
			int32			fEntries;
			int32			fMaxEntries;
			int32			fEntriesPerBlock;
			int32			fBlockSize;
			int32			fBlocks;
			bool			fIsSorted;
			uint8*			fEntryList;
			BPositionIO*	fDataFile;
};

#endif	// _WORD_INDEX_H

