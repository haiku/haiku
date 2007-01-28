/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */
#ifndef AREALINK_H
#define AREALINK_H


#include <InterfaceDefs.h>
#include <List.h>
#include <OS.h>


#define MAX_ATTACHMENT_SIZE 65535	// in bytes
#define SIZE_SIZE 2					// size of the size records in an AreaLink area

class AreaLinkHeader;


class AreaLink {
	public:
		AreaLink(area_id area, bool is_arealink_area = false);
		AreaLink();
		~AreaLink();

		void SetTarget(area_id area, bool is_arealink_area = false);
		area_id GetTarget() const { return fTarget; }

		void Attach(const void *data, size_t size);
		inline void Attach(const int32 &data);
		inline void Attach(const int16 &data);
		inline void Attach(const int8 &data);
		inline void Attach(const float &data);
		inline void Attach(const bool &data);
		inline void Attach(const rgb_color &data);
		inline void Attach(const BRect &data);
		inline void Attach(const BPoint &data);

		void MakeEmpty();
		void *ItemAt(const int32 index) const { return fAttachList->ItemAt(index); }
		int32 CountAttachments() const { return fAttachList->CountItems(); }
		void *BaseAddress() const { return (void *)fBaseAddress; }
	
		void Lock();
		void Unlock();
		bool IsLocked();
		
	private:
		void _ReadAttachments();
		
	private:
		BList *fAttachList;
		area_id fTarget;
		bool fAreaIsOk;
		bool fHaveLock;
		int8 *fBaseAddress;
		AreaLinkHeader *fHeader;
};

#endif	// AREALINK_H
