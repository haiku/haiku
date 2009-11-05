//------------------------------------------------------------------------------
//	Copyright (c) 2003, Ingo Weinhold
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ElfSymbolPatcher.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Interface declaration of classes used for patching ELF
//					symbols. Central class is ElfSymbolPatcher. It is a kind of
//					roster, managing all necessary infos (loaded images) and
//					being able to fill ElfSymbolPatchInfos with life.
//					An ElfSymbolPatchInfo represents a symbol and is able to
//					patch/restore it. An ElfSymbolPatchGroup bundles several
//					ElfSymbolPatchInfos and can update their data, e.g.
//					when images are loaded/unloaded. It uses a
//					ElfSymbolPatcher internally and provides a more convenient
//					API for the user.
//------------------------------------------------------------------------------

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ElfImage.h"
#include "ElfSymbolPatcher.h"

/////////////////////////////
// ElfSymbolPatchInfo::Entry
//

class ElfSymbolPatchInfo::Entry {
public:
	static	Entry*				Create(image_id image, void*** targets,
									   int32 targetCount);
			void				Delete();

			image_id			GetImage() const	{ return fImage; }

			void				Patch(void* newAddress);

private:
								Entry();
								Entry(const Entry&);
								Entry(image_id image, void*** targets,
									  int32 targetCount);
								~Entry();

private:
			image_id			fImage;
			int32				fPatchTargetCount;
			void**				fPatchTargets[1];
};

// Create
ElfSymbolPatchInfo::Entry*
ElfSymbolPatchInfo::Entry::Create(image_id image, void*** targets,
								  int32 targetCount)
{
	if (!targets || targetCount <= 0)
		return NULL;
	void* buffer = malloc(sizeof(Entry) + sizeof(void**) * (targetCount - 1));
	Entry* entry = NULL;
	if (buffer)
		entry = new(buffer) Entry(image, targets, targetCount);
	return entry;
}

// Delete
void
ElfSymbolPatchInfo::Entry::Delete()
{
	this->~Entry();
	free(this);
}

// Patch
void
ElfSymbolPatchInfo::Entry::Patch(void* newAddress)
{
//printf("ElfSymbolPatchInfo::Entry::Patch(): patching %ld addresses\n",
//fPatchTargetCount);
	for (int i = 0; i < fPatchTargetCount; i++)
		*fPatchTargets[i] = newAddress;
}

// constructor
ElfSymbolPatchInfo::Entry::Entry(image_id image, void*** targets,
								 int32 targetCount)
	: fImage(image),
	  fPatchTargetCount(targetCount)
{
	memcpy(fPatchTargets + 0, targets, targetCount * sizeof(void**));
}

// destructor
ElfSymbolPatchInfo::Entry::~Entry()
{
}


//////////////////////
// ElfSymbolPatchInfo
//

// constructor
ElfSymbolPatchInfo::ElfSymbolPatchInfo()
	: fSymbolName(),
	  fOriginalAddress(NULL),
	  fOriginalAddressImage(-1),
	  fEntries()
{
}

// destructor
ElfSymbolPatchInfo::~ElfSymbolPatchInfo()
{
	Unset();
}

// InitCheck
status_t
ElfSymbolPatchInfo::InitCheck() const
{
	return (fOriginalAddress && fSymbolName.Length() ? B_OK : B_NO_INIT);
}

// GetSymbolName
const char*
ElfSymbolPatchInfo::GetSymbolName() const
{
	return fSymbolName.String();
}

// GetOriginalAddress
void*
ElfSymbolPatchInfo::GetOriginalAddress() const
{
	return fOriginalAddress;
}

// GetOriginalAddressImage
image_id
ElfSymbolPatchInfo::GetOriginalAddressImage() const
{
	return fOriginalAddressImage;
}

// Patch
status_t
ElfSymbolPatchInfo::Patch(void* newAddress)
{
//printf("ElfSymbolPatchInfo::Patch(): patching %ld images\n",
//fEntries.CountItems());
	status_t error = InitCheck();
	if (error == B_OK) {
		for (int i = 0; Entry* entry = EntryAt(i); i++)
			entry->Patch(newAddress);
	}
	return error;
}

// Restore
status_t
ElfSymbolPatchInfo::Restore()
{
	return Patch(fOriginalAddress);
}

// Unset
void
ElfSymbolPatchInfo::Unset()
{
	for (int i = 0; Entry* entry = EntryAt(i); i++)
		entry->Delete();
	fEntries.MakeEmpty();
	fSymbolName.SetTo("");
	fOriginalAddress = NULL;
	fOriginalAddressImage = -1;
}

// SetSymbolName
status_t
ElfSymbolPatchInfo::SetSymbolName(const char* name)
{
	fSymbolName.SetTo(name);
	if (name && fSymbolName != name)
		return B_NO_MEMORY;
	return B_OK;
}

// SetOriginalAddress
void
ElfSymbolPatchInfo::SetOriginalAddress(void* address, image_id image)
{
	fOriginalAddress = address;
	fOriginalAddressImage = image;
}

// CreateEntry
status_t
ElfSymbolPatchInfo::CreateEntry(image_id image, BList* targets)
{
	if (!targets || targets->CountItems() == 0)
		return B_BAD_VALUE;
	Entry* entry = Entry::Create(image, (void***)targets->Items(),
								 targets->CountItems());
	if (!entry)
		return B_NO_MEMORY;
	if (!fEntries.AddItem(entry)) {
		entry->Delete();
		return B_NO_MEMORY;
	}
	return B_OK;
}

// DeleteEntry
bool
ElfSymbolPatchInfo::DeleteEntry(image_id image)
{
	for (int i = 0; Entry* entry = EntryAt(i); i++) {
		if (entry->GetImage() == image) {
			fEntries.RemoveItem(i);
			entry->Delete();
			return true;
		}
	}
	return false;
}

// EntryAt
ElfSymbolPatchInfo::Entry*
ElfSymbolPatchInfo::EntryAt(int32 index)
{
	return (Entry*)fEntries.ItemAt(index);
}

// EntryFor
ElfSymbolPatchInfo::Entry*
ElfSymbolPatchInfo::EntryFor(image_id image)
{
	for (int i = 0; Entry* entry = EntryAt(i); i++) {
		if (entry->GetImage() == image)
			return entry;
	}
	return NULL;
}


/////////////////
// UpdateAdapter
//

// constructor
ElfSymbolPatcher::UpdateAdapter::UpdateAdapter()
{
}

// destructor
ElfSymbolPatcher::UpdateAdapter::~UpdateAdapter()
{
}

// ImageAdded
void
ElfSymbolPatcher::UpdateAdapter::ImageAdded(ElfImage* image)
{
}

// ImageRemoved
void
ElfSymbolPatcher::UpdateAdapter::ImageRemoved(ElfImage* image)
{
}


////////////////////
// ElfSymbolPatcher
//

// constructor
ElfSymbolPatcher::ElfSymbolPatcher()
	: fImages(),
	  fInitStatus(B_NO_INIT)
{
	fInitStatus = _Init();
	if (fInitStatus != B_OK)
		_Cleanup();
}

// destructor
ElfSymbolPatcher::~ElfSymbolPatcher()
{
	_Cleanup();
}

// InitCheck
status_t
ElfSymbolPatcher::InitCheck() const
{
	return fInitStatus;
}

// Update
status_t
ElfSymbolPatcher::Update(UpdateAdapter* updateAdapter)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;
	// remove obsolete images
	int32 count = fImages.CountItems();
	for (int i = count - 1; i >= 0; i--) {
		ElfImage* image = _ImageAt(i);
		image_info info;
		if (get_image_info(image->GetID(), &info) != B_OK) {
			if (updateAdapter)
				updateAdapter->ImageRemoved(image);
			fImages.RemoveItem(i);
			delete image;
		}
	}
	// add new images
	status_t error = B_OK;
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		ElfImage* image = _ImageForID(info.id);
		if (image)
			continue;
		image = new(std::nothrow) ElfImage;
		if (!image)
			return B_NO_MEMORY;
		if (!fImages.AddItem(image)) {
			delete image;
			return B_NO_MEMORY;
		}
		error = image->SetTo(info.id);
		if (updateAdapter)
			updateAdapter->ImageAdded(image);
	}
	return error;
}

// Unload
void
ElfSymbolPatcher::Unload()
{
	for (int i = 0; ElfImage* image = _ImageAt(i); i++)
		image->Unload();
}

// GetSymbolPatchInfo
status_t
ElfSymbolPatcher::GetSymbolPatchInfo(const char* symbolName,
									 ElfSymbolPatchInfo* info)
{
	// check parameters and intialization
	if (!symbolName || !info)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return B_NO_INIT;
	// set the symbol name
	info->Unset();
	status_t error = info->SetSymbolName(symbolName);
	if (error != B_OK)
		return error;
	for (int i = 0; ElfImage* image = _ImageAt(i); i++) {
//printf("searching in image: %ld\n", image->GetID());
		// get the symbol's relocations
		BList patchTargets;
		error = image->GetSymbolRelocations(symbolName, &patchTargets);
		if (error != B_OK)
			break;
		if (patchTargets.CountItems() > 0) {
			error = info->CreateEntry(image->GetID(), &patchTargets);
			if (error != B_OK)
				break;
		}
		// get the symbol's address
		void* address = NULL;
		if (image->FindSymbol(symbolName, &address) == B_OK && address) {
			if (info->GetOriginalAddress()) {
				// A symbol with that name lives in at least two images.
				// Better bail out.
// TODO: That doesn't work so well (on gcc 4). E.g. the libsupc++ symbols might
// appear in several images.
//printf("Found the symbol in more than one image!\n");
				error = B_ERROR;
				break;
			} else
				info->SetOriginalAddress(address, image->GetID());
		}
	}
	// set the symbol address
	if (!info->GetOriginalAddress())
{
//printf("Symbol not found in any image!\n");
		error = B_ERROR;
}
	// cleanup on error
	if (error != B_OK)
		info->Unset();
	return error;
}

// UpdateSymbolPatchInfo
status_t
ElfSymbolPatcher::UpdateSymbolPatchInfo(ElfSymbolPatchInfo* info,
										ElfImage* image)
{
	if (!info || !image || !info->GetSymbolName())
		return B_BAD_VALUE;
	// get the symbol's relocations
	BList patchTargets;
	status_t error
		= image->GetSymbolRelocations(info->GetSymbolName(), &patchTargets);
	if (error == B_OK)
		error = info->CreateEntry(image->GetID(), &patchTargets);
	return error;
}

// _Init
status_t
ElfSymbolPatcher::_Init()
{
	status_t error = B_OK;
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		ElfImage* image = new(std::nothrow) ElfImage;
		if (!image)
			return B_NO_MEMORY;
		if (!fImages.AddItem(image)) {
			delete image;
			return B_NO_MEMORY;
		}
		error = image->SetTo(info.id);
	}
	return error;
}

// _Cleanup
void
ElfSymbolPatcher::_Cleanup()
{
	for (int i = 0; ElfImage* image = _ImageAt(i); i++)
		delete image;
	fImages.MakeEmpty();
}

// _ImageAt
ElfImage*
ElfSymbolPatcher::_ImageAt(int32 index) const
{
	return (ElfImage*)fImages.ItemAt(index);
}

// _ImageForID
ElfImage*
ElfSymbolPatcher::_ImageForID(image_id id) const
{
	for (int i = 0; ElfImage* image = _ImageAt(i); i++) {
		if (image->GetID() == id)
			return image;
	}
	return NULL;
}


///////////////////////
// ElfSymbolPatchGroup
//

// constructor
ElfSymbolPatchGroup::ElfSymbolPatchGroup(ElfSymbolPatcher* patcher)
	: fPatcher(patcher),
	  fPatchInfos(),
	  fOwnsPatcher(false),
	  fPatched(false)
{
	// create a patcher if none has been supplied
	if (!fPatcher) {
		fPatcher = new(std::nothrow) ElfSymbolPatcher;
		if (fPatcher) {
			if (fPatcher->InitCheck() == B_OK)
				fOwnsPatcher = true;
			else {
				delete fPatcher;
				fPatcher = NULL;
			}
		}
	}
}

// destructor
ElfSymbolPatchGroup::~ElfSymbolPatchGroup()
{
	RemoveAllPatches();
	if (fPatcher && fOwnsPatcher)
		delete fPatcher;
}

// AddPatch
status_t
ElfSymbolPatchGroup::AddPatch(const char* symbolName, void* newAddress,
							  void** originalAddress)
{
	// check initialization and parameters
	if (!fPatcher)
		return B_NO_INIT;
	if (!symbolName || !originalAddress)
		return B_BAD_VALUE;
	// allocate patch info
	PatchInfo* patchInfo = new(std::nothrow) PatchInfo;
	if (!patchInfo)
		return B_NO_MEMORY;
	// init and add the patch info
	status_t error = fPatcher->GetSymbolPatchInfo(symbolName, patchInfo);
	if (error == B_OK) {
		if (fPatchInfos.AddItem(patchInfo)) {
			patchInfo->fNewAddress = newAddress;
			*originalAddress = patchInfo->GetOriginalAddress();
		} else
			error = B_NO_MEMORY;
	}
	// cleanup on failure
	if (error != B_OK && patchInfo) {
		fPatchInfos.RemoveItem(patchInfo);
		delete patchInfo;
	}
	return error;
}

// RemoveAllPatches
void
ElfSymbolPatchGroup::RemoveAllPatches()
{
	for (int i = 0; PatchInfo* info = (PatchInfo*)fPatchInfos.ItemAt(i); i++)
		delete info;
	fPatchInfos.MakeEmpty();
	fPatched = false;
}

// Patch
status_t
ElfSymbolPatchGroup::Patch()
{
//printf("ElfSymbolPatchGroup::Patch(): patching %ld symbols\n",
//fPatchInfos.CountItems());
	if (!fPatcher)
		return B_NO_INIT;
	if (fPatched)
		return B_OK;
	for (int i = 0; PatchInfo* info = (PatchInfo*)fPatchInfos.ItemAt(i); i++)
		info->Patch(info->fNewAddress);
	fPatched = true;
	return B_OK;
}

// Restore
status_t
ElfSymbolPatchGroup::Restore()
{
	if (!fPatcher)
		return B_NO_INIT;
	if (!fPatched)
		return B_OK;
	for (int i = 0; PatchInfo* info = (PatchInfo*)fPatchInfos.ItemAt(i); i++)
		info->Restore();
	fPatched = false;
	return B_OK;
}

// Update
status_t
ElfSymbolPatchGroup::Update()
{
	if (!fPatcher)
		return B_NO_INIT;
	return fPatcher->Update(this);
}

// ImageAdded
void
ElfSymbolPatchGroup::ImageAdded(ElfImage* image)
{
	for (int i = 0; PatchInfo* info = (PatchInfo*)fPatchInfos.ItemAt(i); i++) {
		fPatcher->UpdateSymbolPatchInfo(info, image);
		if (fPatched) {
			ElfSymbolPatchInfo::Entry* entry = info->EntryFor(image->GetID());
			if (entry)
				info->Patch(info->fNewAddress);
		}
	}
}

// ImageRemoved
void
ElfSymbolPatchGroup::ImageRemoved(ElfImage* image)
{
	int32 count = fPatchInfos.CountItems();
	for (int i = count - 1; i >= 0; i--) {
		PatchInfo* info = (PatchInfo*)fPatchInfos.ItemAt(i);
		if (info->GetOriginalAddressImage() == image->GetID()) {
			// the image the symbol lives in, has been removed: remove the
			// complete patch info
			fPatchInfos.RemoveItem(i);
			delete info;
		} else
			info->DeleteEntry(image->GetID());
	}
}

