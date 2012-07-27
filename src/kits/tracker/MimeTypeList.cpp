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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Mime.h>

#include "AutoLock.h"
#include "MimeTypeList.h"
#include "Thread.h"


ShortMimeInfo::ShortMimeInfo(const BMimeType &mimeType)
	:	fCommonMimeType(true)
{
	fPrivateName = mimeType.Type();

	char buffer[B_MIME_TYPE_LENGTH];

	// weed out apps - their preferred handler is themselves
	if (mimeType.GetPreferredApp(buffer) == B_OK
		&& fPrivateName.ICompare(buffer) == 0)
		fCommonMimeType = false;

	// weed out metamimes without a short description
	if (mimeType.GetShortDescription(buffer) != B_OK || buffer[0] == 0)
		fCommonMimeType = false;
	else
		fShortDescription = buffer;
}


ShortMimeInfo::ShortMimeInfo(const char* shortDescription)
	:	fShortDescription(shortDescription)
{
}

const char*
ShortMimeInfo::InternalName() const
{
	return fPrivateName.String();
}

const char*
ShortMimeInfo::ShortDescription() const
{
	return fShortDescription.String();
}

int
ShortMimeInfo::CompareShortDescription(const ShortMimeInfo* a, const ShortMimeInfo* b)
{
	return a->fShortDescription.ICompare(b->fShortDescription);
}

bool
ShortMimeInfo::IsCommonMimeType() const
{
	return fCommonMimeType;
}


//	#pragma mark -


MimeTypeList::MimeTypeList()
	:	fMimeList(100, true),
		fCommonMimeList(30, false),
		fLock("mimeListLock")
{
	fLock.Lock();
	Thread::Launch(NewMemberFunctionObject(&MimeTypeList::Build, this),
		B_NORMAL_PRIORITY);
}

static int
MatchOneShortDescription(const ShortMimeInfo* a, const ShortMimeInfo* b)
{
	return strcasecmp(a->ShortDescription(), b->ShortDescription());
}

const ShortMimeInfo*
MimeTypeList::FindMimeType(const char* shortDescription) const
{
	ShortMimeInfo tmp(shortDescription);
	const ShortMimeInfo* result = fCommonMimeList.BinarySearch(tmp,
		&MatchOneShortDescription);

	return result;
}

const ShortMimeInfo*
MimeTypeList::EachCommonType(bool (*func)(const ShortMimeInfo*, void*),
	void* state) const
{
	AutoLock<Benaphore> locker(fLock);
	int32 count = fCommonMimeList.CountItems();
	for (int32 index = 0; index < count; index++) {
		if ((func)(fCommonMimeList.ItemAt(index), state))
			return fCommonMimeList.ItemAt(index);
	}
	return NULL;
}

void
MimeTypeList::Build()
{
	ASSERT(fLock.IsLocked());

	BMessage message;
	BMimeType::GetInstalledTypes(&message);

	int32 count;
	uint32 type;
	message.GetInfo("types", &type, &count);

	for (int32 index = 0; index < count; index++) {
		const char* str;
		if (message.FindString("types", index, &str) != B_OK)
			continue;

		BMimeType mimetype(str);
		if (mimetype.InitCheck() != B_OK)
			continue;

		ShortMimeInfo* mimeInfo = new ShortMimeInfo(mimetype);
		fMimeList.AddItem(mimeInfo);
		if (mimeInfo->IsCommonMimeType())
			fCommonMimeList.AddItem(mimeInfo);
	}
	fCommonMimeList.SortItems(&ShortMimeInfo::CompareShortDescription);
	fLock.Unlock();
}


