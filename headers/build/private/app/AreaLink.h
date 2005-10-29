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
//	File Name:		AreaLink.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Helper class for managing chunks of small data in an area
//  
//------------------------------------------------------------------------------
#ifndef AREALINK_H
#define AREALINK_H

#include <List.h>
#include <InterfaceDefs.h>
#include <OS.h>

#define MAX_ATTACHMENT_SIZE 65535	// in bytes
#define SIZE_SIZE 2					// size of the size records in an AreaLink area

class AreaLinkHeader;

class AreaLink
{
public:
	AreaLink(area_id area, bool is_arealink_area=false);
	AreaLink(void);
	~AreaLink(void);

	void SetTarget(area_id area, bool is_arealink_area=false);
	area_id GetTarget(void) { return target; }

	void Attach(const void *data, size_t size);
	inline void Attach(const int32 &data);
	inline void Attach(const int16 &data);
	inline void Attach(const int8 &data);
	inline void Attach(const float &data);
	inline void Attach(const bool &data);
	inline void Attach(const rgb_color &data);
	inline void Attach(const BRect &data);
	inline void Attach(const BPoint &data);

	void MakeEmpty(void);
	void *ItemAt(int32 index) { return attachlist->ItemAt(index); }
	int32 CountAttachments(void) { return attachlist->CountItems(); }
	void *BaseAddress(void) { return (void *)baseaddress; }
	
	void Lock(void);
	void Unlock(void);
	bool IsLocked(void);
protected:
	void _ReadAttachments(void);

	BList *attachlist;
	area_id target;
	bool area_ok;
	bool have_lock;
	int8 *baseaddress;
	AreaLinkHeader *header;
};

#endif
