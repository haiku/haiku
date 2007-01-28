/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */


#include "AreaLink.h"

#include <Rect.h>

#include <string.h>

//#define AL_DEBUG
#ifdef AL_DEBUG
#include <stdio.h>
#endif


class AreaLinkHeader {
	public:
		AreaLinkHeader() { MakeEmpty(); fLock = B_NAME_NOT_FOUND; }

		void SetAttachmentCount(uint32 size) { fAttachmentCount = size; }
		uint32 GetAttachmentCount() { return fAttachmentCount; }

		void SetAttachmentSize(uint32 size) { fAttachmentSize = size; }
		uint32 GetAttachmentSize() { return fAttachmentSize; }

		void AddAttachment(uint32 size) { fAttachmentSize += size; fAttachmentCount++; }
	
		void SetLockSem(sem_id sem) { fLock = sem; }
		sem_id GetLockSem() { return fLock; }

		void MakeEmpty() { fAttachmentCount = 0; fAttachmentSize = 0; }
		area_info GetInfo() { return fInfo; }
		void SetInfo(const area_info &newInfo) { fInfo = newInfo; }
		
	private:
		uint32 fAttachmentCount;
		uint32 fAttachmentSize;
		sem_id fLock;
		area_info fInfo;
};


//	#pragma mark -


AreaLink::AreaLink(area_id area, bool isAreaLinkArea)
	:fAttachList(new BList(0)),
	fHaveLock(false)
{
	SetTarget(area, isAreaLinkArea);
}


AreaLink::AreaLink()
	:fAttachList(new BList(0)),
	fTarget(B_NAME_NOT_FOUND),
	fAreaIsOk(false),
	fHaveLock(false)
	
{	
}


AreaLink::~AreaLink()
{
	delete fAttachList;
}


void
AreaLink::SetTarget(area_id area, bool isAreaLinkArea)
{
	area_info targetInfo;

	if (get_area_info(area, &targetInfo) == B_OK) {
		fTarget = area;
		fAreaIsOk = true;
		fHeader = (AreaLinkHeader *)targetInfo.address;

		if (isAreaLinkArea)
			_ReadAttachments();
		else {
			fHeader->MakeEmpty();
			fHeader->SetInfo(targetInfo);
		}

		fBaseAddress = (int8*)targetInfo.address;
		fBaseAddress += sizeof(AreaLinkHeader);
	} else {
		fTarget = B_NAME_NOT_FOUND;
		fAreaIsOk = false;
	}
}


void
AreaLink::Attach(const void *data, size_t size)
{
	if (!data || size == 0)
		return;
	
	Lock();
	
	// Will the attachment fit?
	if (fHeader->GetAttachmentSize() + size > fHeader->GetInfo().size) {
		// Being it won't fit, resize the area to fit the thing
		int32 pageNum = int32(fHeader->GetInfo().size / B_PAGE_SIZE);
		resize_area(fTarget, pageNum + 1);
	}
	
	// Our attachment will fit, so copy the data into the current location
	// and increment the appropriate fHeader values
	int8 *currentpointer = (int8*)BaseAddress();
	currentpointer += fHeader->GetAttachmentSize();
	memcpy(currentpointer, data, size);

	fAttachList->AddItem(currentpointer);
	fHeader->AddAttachment(size);
	
	Unlock();
}


void
AreaLink::Attach(const int32 &data)
{
	Attach(&data, sizeof(int32));
}


void
AreaLink::Attach(const int16 &data)
{
	Attach(&data, sizeof(int16));
}


void
AreaLink::Attach(const int8 &data)
{
	Attach(&data, sizeof(int8));
}


void
AreaLink::Attach(const float &data)
{
	Attach(&data, sizeof(float));
}


void
AreaLink::Attach(const bool &data)
{
	Attach(&data, sizeof(bool));
}


void
AreaLink::Attach(const rgb_color &data)
{
	Attach(&data, sizeof(rgb_color));
}


void
AreaLink::Attach(const BRect &data)
{
	Attach(&data, sizeof(BRect));
}


void
AreaLink::Attach(const BPoint &data)
{
	Attach(&data, sizeof(BPoint));
}


void
AreaLink::MakeEmpty()
{
	fAttachList->MakeEmpty();
	fHeader->MakeEmpty();
}


void
AreaLink::_ReadAttachments()
{
	int8 *index = fBaseAddress;
	uint16 size;
	
	for (uint32 i = 0; i < fHeader->GetAttachmentCount(); i++) {
		size = *((int16*)index);
		fBaseAddress += SIZE_SIZE;
		fAttachList->AddItem(fBaseAddress);
		fBaseAddress += size;
	}
}


void
AreaLink::Lock()
{
	if (!fHaveLock)
		acquire_sem(fHeader->GetLockSem());
}


void
AreaLink::Unlock()
{
	if (fHaveLock)
		release_sem(fHeader->GetLockSem());
}


bool
AreaLink::IsLocked()
{
	return fHaveLock;
}
