/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef _T_RESOURCE_SET_H
#define _T_RESOURCE_SET_H


#include <List.h>
#include <Locker.h>
#include <SupportDefs.h>


class BBitmap;
class BResources;
class BDataIO;
class BString;

namespace TResourcePrivate {
	class TypeItem;
	class TypeList;
}

using namespace TResourcePrivate;

const uint32 B_STRING_BLOCK_TYPE = 'SBLK';

class TStringBlock {
public:
	TStringBlock(BDataIO* data);
	TStringBlock(const void* block, size_t size);
	virtual ~TStringBlock();

	const char* String(size_t index) const;

private:
	size_t PreIndex(char* strings, ssize_t len);
	void MakeIndex(const char* strings, ssize_t len, size_t indexLen,
		size_t* outIndex);

	size_t fNumEntries;
	size_t* fIndex;
	char* fStrings;
	bool fOwnData;
};

class TResourceSet {
public:
	TResourceSet();
	virtual ~TResourceSet();

	status_t AddResources(BResources* resources);
	status_t AddDirectory(const char* fullPath);
	status_t AddEnvDirectory(const char* envPath,
		const char* defaultValue = NULL);

	const void* FindResource(type_code type, int32 id, size_t* outSize);
	const void* FindResource(type_code type, const char* name, size_t* outSize);

	const BBitmap* FindBitmap(type_code type, int32 id);
	const BBitmap* FindBitmap(type_code type, const char* name);

	const TStringBlock* FindStringBlock(type_code type, int32 id);
	const TStringBlock* FindStringBlock(type_code type, const char* name);

	const char* FindString(type_code type, int32 id, uint32 index);
	const char* FindString(type_code type, const char* name, uint32 index);

private:
	status_t ExpandString(BString* out, const char* in);
	TypeList* FindTypeList(type_code type);

	TypeItem* FindItemID(type_code type, int32 id);
	TypeItem* FindItemName(type_code type, const char* name);

	TypeItem* LoadResource(type_code type, int32 id, const char* name,
		TypeList** inoutList = NULL);

	BBitmap* ReturnBitmapItem(type_code type, TypeItem* from);
	TStringBlock* ReturnStringBlockItem(TypeItem* from);

	BLocker fLock;				// access control.
	BList fResources;			// containing BResources objects.
	BList fDirectories;			// containing BPath objects.
	BList fTypes;				// containing TypeList objects.
};

TResourceSet* AppResSet();


#endif	/* _T_RESOURCE_SET_H */
