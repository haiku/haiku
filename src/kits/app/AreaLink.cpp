//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		AreaLink.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Helper class for managing chunks of small data in an area
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include "AreaLink.h"
#include <string.h>

//#define AL_DEBUG
 
#ifdef AL_DEBUG
#include <stdio.h>
#endif

class AreaLinkHeader
{
public:
	AreaLinkHeader(void) { MakeEmpty(); lock=B_NAME_NOT_FOUND; }

	void SetAttachmentCount(uint32 size) { attachmentcount=size; }
	uint32 GetAttachmentCount(void) { return attachmentcount; }

	void SetAttachmentSize(uint32 size) { attachmentsize=size; }
	uint32 GetAttachmentSize(void) { return attachmentsize; }
	
	void AddAttachment(uint32 size) { attachmentsize+=size; attachmentcount++; }
	
	void SetLockSem(sem_id sem) { lock=sem; }
	sem_id GetLockSem(void) { return lock; }

	void MakeEmpty(void) { attachmentcount=0; attachmentsize=0; }
	area_info GetInfo(void) { return info; }
	void SetInfo(const area_info &newinfo) { info=newinfo; }
private:
	uint32 attachmentcount;
	uint32 attachmentsize;
	sem_id lock;
	area_info info;
};

AreaLink::AreaLink(area_id area, bool is_arealink_area)
{
	attachlist=new BList(0);
	have_lock=false;
	SetTarget(area,is_arealink_area);
}

AreaLink::AreaLink(void)
{
	attachlist=new BList(0);
	area_ok=false;
	have_lock=false;
	target=B_NAME_NOT_FOUND;
}

AreaLink::~AreaLink(void)
{
	delete attachlist;
}

void AreaLink::SetTarget(area_id area,bool is_arealink_area)
{
	area_info targetinfo;

	if(get_area_info(area,&targetinfo)==B_OK)
	{
		target=area;
		area_ok=true;
		header=(AreaLinkHeader *)targetinfo.address;

		if(is_arealink_area)
			_ReadAttachments();
		else
		{
			header->MakeEmpty();
			header->SetInfo(targetinfo);
		}

		baseaddress=(int8*)targetinfo.address;
		baseaddress+=sizeof(AreaLinkHeader);
	}
	else
	{
		target=B_NAME_NOT_FOUND;
		area_ok=false;
	}
}

void AreaLink::Attach(const void *data, size_t size)
{
	if(!data || size==0)
		return;
	
	Lock();
	
	// Will the attachment fit?
	if(header->GetAttachmentSize()+size>header->GetInfo().size)
	{
		// Being it won't fit, resize the area to fit the thing
		int32 pagenum=int32(header->GetInfo().size/B_PAGE_SIZE);
		resize_area(target,pagenum+1);
	}
	
	// Our attachment will fit, so copy the data into the current location
	// and increment the appropriate header values
	int8 *currentpointer=(int8*)BaseAddress();
	currentpointer+=header->GetAttachmentSize();
	memcpy(currentpointer,data,size);

	attachlist->AddItem(currentpointer);
	header->AddAttachment(size);
	
	Unlock();
}

void AreaLink::Attach(const int32 &data)
{
	Attach(&data,sizeof(int32));
}

void AreaLink::Attach(const int16 &data)
{
	Attach(&data,sizeof(int16));
}

void AreaLink::Attach(const int8 &data)
{
	Attach(&data,sizeof(int8));
}

void AreaLink::Attach(const float &data)
{
	Attach(&data,sizeof(float));
}

void AreaLink::Attach(const bool &data)
{
	Attach(&data,sizeof(bool));
}

void AreaLink::Attach(const rgb_color &data)
{
	Attach(&data,sizeof(rgb_color));
}

void AreaLink::Attach(const BRect &data)
{
	Attach(&data,sizeof(BRect));
}

void AreaLink::Attach(const BPoint &data)
{
	Attach(&data,sizeof(BPoint));
}

void AreaLink::MakeEmpty(void)
{
	attachlist->MakeEmpty();
	header->MakeEmpty();
}

void AreaLink::_ReadAttachments(void)
{
	int8 *index=baseaddress;
	uint16 size;
	
	for(uint32 i=0; i<header->GetAttachmentCount();i++)
	{
		size=*((int16*)index);
		baseaddress+=SIZE_SIZE;
		attachlist->AddItem(baseaddress);
		baseaddress+=size;
	}
}

void AreaLink::Lock(void)
{
	if(!have_lock)
		acquire_sem(header->GetLockSem());
}

void AreaLink::Unlock(void)
{
	if(have_lock)
		release_sem(header->GetLockSem());
}

bool AreaLink::IsLocked(void)
{
	return have_lock;
}
