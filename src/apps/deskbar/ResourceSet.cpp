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


#define USE_RESOURCES 1

#include "ResourceSet.h"

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <String.h>

#if USE_RESOURCES
#include <Resources.h>
#endif

#if USE_RESOURCES
#define RESOURCES_ONLY(x) x
#else
#define RESOURCES_ONLY(x)
#endif


namespace TResourcePrivate {
	class TypeObject {
	public:
		TypeObject()
			:	fDeleteOK(false)
		{
		}

		virtual ~TypeObject()
		{
			if (!fDeleteOK)
				debugger("deleting object owned by BResourceSet");
		}

		void Delete()
		{
			fDeleteOK = true;
		}

	private:
		TypeObject(const TypeObject &);
		TypeObject &operator=(const TypeObject &);
		bool operator==(const TypeObject &);
		bool operator!=(const TypeObject &);

		bool fDeleteOK;
	};

	class BitmapTypeItem : public BBitmap, public TypeObject {
	public:
		BitmapTypeItem(BRect bounds, uint32 flags, color_space depth,
			int32 bytesPerRow = B_ANY_BYTES_PER_ROW, screen_id screenID
			= B_MAIN_SCREEN_ID)
			:	BBitmap(bounds, flags, depth, bytesPerRow, screenID)
		{
		}

		BitmapTypeItem(const BBitmap* source, bool accepts_views = false,
			bool need_contiguous = false)
			:	BBitmap(source, accepts_views, need_contiguous)
		{
		}

		BitmapTypeItem(BMessage* data)
			:	BBitmap(data)
		{
		}

		virtual ~BitmapTypeItem()
		{
		}
	};

	class StringBlockTypeItem : public TStringBlock, public TypeObject {
	public:
		StringBlockTypeItem(BDataIO* data)
			:	TStringBlock(data)
		{
		}

		StringBlockTypeItem(const void* block, size_t size)
			:	TStringBlock(block, size)
		{
		}

		virtual ~StringBlockTypeItem()
		{
		}
	};

	class TypeItem {
	public:
		TypeItem(int32 id, const char* name, const void* data, size_t size)
			:	fID(id), fName(name),
				fData(const_cast<void*>(data)), fSize(size), fObject(0),
				fOwnData(false), fSourceIsFile(false)
		{
		}

		TypeItem(int32 id, const char* name, BFile* file)
			:	fID(id),
				fName(name),
				fData(NULL),
				fSize(0),
				fObject(NULL),
				fOwnData(true),
				fSourceIsFile(false)
		{
			off_t size;
			if (file->GetSize(&size) == B_OK) {
				fSize = (size_t)size;
				fData = malloc(fSize);
				if (file->ReadAt(0, fData, fSize) < B_OK ) {
					free(fData);
					fData = NULL;
					fSize = 0;
				}
			}
		}

		virtual ~TypeItem()
		{
			if (fOwnData) {
				free(fData);
				fData = NULL;
				fSize = 0;
			}
			SetObject(NULL);
		}

		int32 ID() const
		{
			return fID;
		}

		const char* Name() const
		{
			return fName.String();
		}

		const void* Data() const
		{
			return fData;
		}

		size_t Size() const
		{
			return fSize;
		}

		void SetObject(TypeObject* object)
		{
			if (object == fObject)
				return;
			if (fObject)
				fObject->Delete();
			fObject = object;
		}

		TypeObject* Object() const
		{
			return fObject;
		}

		void SetSourceIsFile(bool state)
		{
			fSourceIsFile = state;
		}

		bool SourceIsFile() const
		{
			return fSourceIsFile;
		}

	private:
		int32 fID;
		BString fName;
		void* fData;
		size_t fSize;
		TypeObject* fObject;
		bool fOwnData;
		bool fSourceIsFile;
	};

	static bool FreeTypeItemFunc(void* item)
	{
		delete reinterpret_cast<TypeItem*>(item);
		return false;
	}

	class TypeList {
	public:
		TypeList(type_code type)
			:	fType(type)
		{
		}

		virtual ~TypeList()
		{
			fItems.DoForEach(FreeTypeItemFunc);
			fItems.MakeEmpty();
		}

		type_code Type() const
		{
			return fType;
		}

		TypeItem* FindItemByID(int32 id)
		{
			for (int32 i = 0; i < fItems.CountItems(); i++ ) {
				TypeItem* it = (TypeItem*)fItems.ItemAt(i);
				if (it->ID() == id)
					return it;
			}
			return NULL;
		}

		TypeItem* FindItemByName(const char* name)
		{
			for (int32 i = 0; i < fItems.CountItems(); i++ ) {
				TypeItem* it = (TypeItem*)fItems.ItemAt(i);
				if (strcmp(it->Name(), name) == 0)
					return it;
			}
			return NULL;
		}

		void AddItem(TypeItem* item)
		{
			fItems.AddItem(item);
		}

	private:
		type_code fType;
		BList fItems;
	};
}

using namespace TResourcePrivate;

//	#pragma mark -
// ----------------------------- TStringBlock -----------------------------


TStringBlock::TStringBlock(BDataIO* data)
	:	fNumEntries(0),
		fIndex(0),
		fStrings(NULL),
		fOwnData(true)
{
	fStrings = (char*)malloc(1024);
	size_t pos = 0;
	ssize_t amount;
	while ((amount = data->Read(fStrings + pos, 1024)) == 1024) {
		pos += amount;
		fStrings = (char*)realloc(fStrings, pos + 1024);
	}
	if (amount > 0)
		pos += amount;

	fNumEntries = PreIndex(fStrings, amount);
	fIndex = (size_t*)malloc(sizeof(size_t) * fNumEntries);
	MakeIndex(fStrings, amount, fNumEntries, fIndex);
}


TStringBlock::TStringBlock(const void* block, size_t size)
	:
	fNumEntries(0),
	fIndex(0),
	fStrings(NULL),
	fOwnData(false)
{
	fIndex = (size_t*)const_cast<void*>(block);
	fStrings = (char*)const_cast<void*>(block);

	// Figure out how many entries there are.
	size_t last_off = 0;
	while (fIndex[fNumEntries] > last_off && fIndex[fNumEntries] < size ) {
		last_off = fIndex[fNumEntries];
		fNumEntries++;
	}
}


TStringBlock::~TStringBlock()
{
	if (fOwnData) {
		free(fIndex);
		free(fStrings);
	}
	fIndex = 0;
	fStrings = NULL;
}


const char*
TStringBlock::String(size_t index) const
{
	if (index >= fNumEntries)
		return NULL;

	return fStrings + fIndex[index];
}


size_t
TStringBlock::PreIndex(char* strings, ssize_t len)
{
	size_t count = 0;
	char* orig = strings;
	char* end = strings + len;
	bool in_cr = false;
	bool first = true;
	bool skipping = false;

	while (orig < end) {
		if (*orig == '\n' || *orig == '\r' || *orig == 0) {
			if (!in_cr && *orig == '\r')
				in_cr = true;
			if (in_cr && *orig == '\n') {
				orig++;
				continue;
			}
			first = true;
			skipping = false;
			*strings = 0;
			count++;
		} else if (first && *orig == '#') {
			skipping = true;
			first = false;
			orig++;
			continue;
		} else if (skipping) {
			orig++;
			continue;
		} else if (*orig == '\\' && (orig + 1) < end) {
			orig++;
			switch (*orig) {
				case '\\':
					*strings = '\\';
					break;

				case '\n':
					*strings = '\n';
					break;

				case '\r':
					*strings = '\r';
					break;

				case '\t':
					*strings = '\t';
					break;

				default:
					*strings = *orig;
					break;
			}
		} else
			*strings = *orig;

		orig++;
		strings++;
	}
	return count;
}


void
TStringBlock::MakeIndex(const char* strings, ssize_t len,
	size_t indexLength, size_t* resultingIndex)
{
	*resultingIndex++ = 0;
	indexLength--;

	ssize_t pos = 0;
	while (pos < len && indexLength > 0) {
		if (strings[pos] == 0 ) {
			*resultingIndex++ = (size_t)pos + 1;
			indexLength--;
		}
		pos++;
	}
}


//	#pragma mark -


#if USE_RESOURCES
static bool
FreeResourcesFunc(void* item)
{
	delete reinterpret_cast<BResources*>(item);
	return false;
}
#endif


static bool
FreePathFunc(void* item)
{
	delete reinterpret_cast<BPath*>(item);
	return false;
}


static bool
FreeTypeFunc(void* item)
{
	delete reinterpret_cast<TypeList*>(item);
	return false;
}


//	#pragma mark -
// ----------------------------- TResourceSet -----------------------------


TResourceSet::TResourceSet()
{
}


TResourceSet::~TResourceSet()
{
	BAutolock lock(&fLock);
#if USE_RESOURCES
	fResources.DoForEach(FreeResourcesFunc);
	fResources.MakeEmpty();
#endif
	fDirectories.DoForEach(FreePathFunc);
	fDirectories.MakeEmpty();
	fTypes.DoForEach(FreeTypeFunc);
	fTypes.MakeEmpty();
}


status_t
TResourceSet::AddResources(BResources* RESOURCES_ONLY(resources))
{
#if USE_RESOURCES
	if (!resources)
		return B_BAD_VALUE;

	BAutolock lock(&fLock);
	status_t err = fResources.AddItem(resources) ? B_OK : B_ERROR;
	if (err != B_OK)
		delete resources;
	return err;
#else
	return B_ERROR;
#endif
}


status_t
TResourceSet::AddDirectory(const char* fullPath)
{
	if (!fullPath)
		return B_BAD_VALUE;

	BPath* path = new BPath(fullPath);
	status_t err = path->InitCheck();
	if (err != B_OK) {
		delete path;
		return err;
	}

	BAutolock lock(&fLock);
	err = fDirectories.AddItem(path) ? B_OK : B_ERROR;
	if (err != B_OK)
		delete path;

	return err;
}


status_t
TResourceSet::AddEnvDirectory(const char* in, const char* defaultValue)
{
	BString buf;
	status_t err = ExpandString(&buf, in);

	if (err != B_OK) {
		if (defaultValue)
			return AddDirectory(defaultValue);
		return err;
	}

	return AddDirectory(buf.String());
}


status_t
TResourceSet::ExpandString(BString* out, const char* in)
{
	const char* start = in;

	while (*in) {
		if (*in == '$') {
			if (start < in)
				out->Append(start, (int32)(in - start));

			in++;
			char variableName[1024];
			size_t i = 0;
			if (*in == '{') {
				in++;
				while (*in && *in != '}' && i < sizeof(variableName) - 1)
					variableName[i++] = *in++;

				if (*in)
					in++;

			} else {
				while ((isalnum(*in) || *in == '_')
					&& i < sizeof(variableName) - 1)
					variableName[i++] = *in++;
			}

			start = in;
			variableName[i] = '\0';

			const char* val = getenv(variableName);
			if (!val) {
				PRINT(("Error: env var %s not found.\n", &variableName[0]));
				return B_NAME_NOT_FOUND;
			}

			status_t err = ExpandString(out, val);
			if (err != B_OK)
				return err;

		} else if (*in == '\\') {
			if (start < in)
				out->Append(start, (int32)(in - start));
			in++;
			start = in;
			in++;
		} else
			in++;
	}

	if (start < in)
		out->Append(start, (int32)(in - start));

	return B_OK;
}


const void*
TResourceSet::FindResource(type_code type, int32 id, size_t* outSize)
{
	TypeItem* item = FindItemID(type, id);

	if (outSize)
		*outSize = item ? item->Size() : 0;

	return item ? item->Data() : NULL;
}


const void*
TResourceSet::FindResource(type_code type, const char* name, size_t* outSize)
{
	TypeItem* item = FindItemName(type, name);

	if (outSize)
		*outSize = item ? item->Size() : 0;

	return item ? item->Data() : NULL;
}


const BBitmap*
TResourceSet::FindBitmap(type_code type, int32 id)
{
	return ReturnBitmapItem(type, FindItemID(type, id));
}


const BBitmap*
TResourceSet::FindBitmap(type_code type, const char* name)
{
	return ReturnBitmapItem(type, FindItemName(type, name));
}


const TStringBlock*
TResourceSet::FindStringBlock(type_code type, int32 id)
{
	return ReturnStringBlockItem(FindItemID(type, id));
}


const TStringBlock*
TResourceSet::FindStringBlock(type_code type, const char* name)
{
	return ReturnStringBlockItem(FindItemName(type, name));
}


const char*
TResourceSet::FindString(type_code type, int32 id, uint32 index)
{
	const TStringBlock* stringBlock = FindStringBlock(type, id);

	if (!stringBlock)
		return NULL;

	return stringBlock->String(index);
}


const char*
TResourceSet::FindString(type_code type, const char* name, uint32 index)
{
	const TStringBlock* stringBlock = FindStringBlock(type, name);

	if (!stringBlock)
		return NULL;

	return stringBlock->String(index);
}


TypeList*
TResourceSet::FindTypeList(type_code type)
{
	BAutolock lock(&fLock);

	int32 count = fTypes.CountItems();
	for (int32 i = 0; i < count; i++ ) {
		TypeList* list = (TypeList*)fTypes.ItemAt(i);
		if (list && list->Type() == type)
			return list;
	}

	return NULL;
}


TypeItem*
TResourceSet::FindItemID(type_code type, int32 id)
{
	TypeList* list = FindTypeList(type);
	TypeItem* item = NULL;

	if (list)
		item = list->FindItemByID(id);

	if (!item)
		item = LoadResource(type, id, 0, &list);

	return item;
}


TypeItem*
TResourceSet::FindItemName(type_code type, const char* name)
{
	TypeList* list = FindTypeList(type);
	TypeItem* item = NULL;

	if (list)
		item = list->FindItemByName(name);

	if (!item)
		item = LoadResource(type, -1, name, &list);

	return item;
}


TypeItem*
TResourceSet::LoadResource(type_code type, int32 id, const char* name,
	TypeList** inOutList)
{
	TypeItem* item = NULL;

	if (name) {
		BEntry entry;

		// If a named resource, first look in directories.
		fLock.Lock();
		int32 count = fDirectories.CountItems();
		for (int32 i = 0; item == 0 && i < count; i++) {
			BPath* dir = (BPath*)fDirectories.ItemAt(i);
			if (dir) {
				fLock.Unlock();
				BPath path(dir->Path(), name);
				if (entry.SetTo(path.Path(), true) == B_OK ) {
					BFile file(&entry, B_READ_ONLY);
					if (file.InitCheck() == B_OK ) {
						item = new TypeItem(id, name, &file);
						item->SetSourceIsFile(true);
					}
				}
				fLock.Lock();
			}
		}
		fLock.Unlock();
	}

#if USE_RESOURCES
	if (!item) {
		// Look through resource objects for data.
		fLock.Lock();
		int32 count = fResources.CountItems();
		for (int32 i = 0; item == 0 && i < count; i++ ) {
			BResources* resource = (BResources*)fResources.ItemAt(i);
			if (resource) {
				const void* data = NULL;
				size_t size = 0;
				if (id >= 0)
					data = resource->LoadResource(type, id, &size);
				else if (name != NULL)
					data = resource->LoadResource(type, name, &size);

				if (data && size) {
					item = new TypeItem(id, name, data, size);
					item->SetSourceIsFile(false);
				}
			}
		}
		fLock.Unlock();
	}
#endif

	if (item) {
		TypeList* list = inOutList ? *inOutList : NULL;
		if (!list) {
			// Don't currently have a list for this type -- check if there is
			// already one.
			list = FindTypeList(type);
		}

		BAutolock lock(&fLock);

		if (!list) {
			// Need to make a new list for this type.
			list = new TypeList(type);
			fTypes.AddItem(list);
		}
		if (inOutList)
			*inOutList = list;

		list->AddItem(item);
	}

	return item;
}


BBitmap*
TResourceSet::ReturnBitmapItem(type_code, TypeItem* from)
{
	if (!from)
		return NULL;

	TypeObject* obj = from->Object();
	BitmapTypeItem* bitmap = dynamic_cast<BitmapTypeItem*>(obj);
	if (bitmap)
		return bitmap;

	// Can't change an existing object.
	if (obj)
		return NULL;

	// Don't have a bitmap in the item -- we'll try to create one.
	BMemoryIO stream(from->Data(), from->Size());

	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	if (archive.Unflatten(&stream) == B_OK) {
		bitmap = new BitmapTypeItem(&archive);
		if (bitmap && bitmap->InitCheck() != B_OK) {
			bitmap->Delete();
				// allows us to delete this bitmap...
			delete bitmap;
			bitmap = NULL;
		}
	}

	if (bitmap) {
		BAutolock lock(&fLock);
		if (from->Object() != NULL) {
			// Whoops! Someone snuck in under us.
			bitmap->Delete();
			delete bitmap;
			bitmap = dynamic_cast<BitmapTypeItem*>(from->Object());
		} else
			from->SetObject(bitmap);
	}

	return bitmap;
}


TStringBlock*
TResourceSet::ReturnStringBlockItem(TypeItem* from)
{
	if (!from)
		return NULL;

	TypeObject* obj = from->Object();
	StringBlockTypeItem* stringBlock = dynamic_cast<StringBlockTypeItem*>(obj);
	if (stringBlock)
		return stringBlock;

	// Can't change an existing object.
	if (obj)
		return NULL;

	// Don't have a string block in the item -- we'll create one.
	if (from->SourceIsFile() ) {
		BMemoryIO stream(from->Data(), from->Size());
		stringBlock = new StringBlockTypeItem(&stream);
	} else
		stringBlock = new StringBlockTypeItem(from->Data(), from->Size());

	if (stringBlock) {
		BAutolock lock(&fLock);
		if (from->Object() != NULL) {
			// Whoops! Someone snuck in under us.
			delete stringBlock;
			stringBlock = dynamic_cast<StringBlockTypeItem*>(from->Object());
		} else
			from->SetObject(stringBlock);
	}

	return stringBlock;
}


//	#pragma mark -


namespace TResourcePrivate {
	TResourceSet* gResources = NULL;
	BLocker gResourceLocker;
}


TResourceSet*
AppResSet()
{
	// If already have it, return immediately.
	if (gResources)
		return gResources;

	// Don't have 'em, lock access to make 'em.
	if (!gResourceLocker.Lock())
		return NULL;
	if (gResources) {
		// Whoops, somebody else already did.  Oh well.
		gResourceLocker.Unlock();
		return gResources;
	}

	// Make 'em.
	gResources = new TResourceSet;
	gResources->AddResources(BApplication::AppResources());
	gResourceLocker.Unlock();
	return gResources;
}
