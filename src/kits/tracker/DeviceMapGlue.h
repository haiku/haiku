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

/****************************************************************************
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
**                                                                         **
**                          DANGER, WILL ROBINSON!                         **
**                                                                         **
** The interfaces contained here are part of BeOS's                        **
**                                                                         **
**                     >> PRIVATE NOT FOR PUBLIC USE <<                    **
**                                                                         **
**                                                       implementation.   **
**                                                                         **
** These interfaces              WILL CHANGE        in future releases.    **
** If you use them, your app     WILL BREAK         at some future time.   **
**                                                                         **
** (And yes, this does mean that binaries built from OpenTracker will not  **
** be compatible with some future releases of the OS.  When that happens,  **
** we will provide an updated version of this file to keep compatibility.) **
**                                                                         **
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
****************************************************************************/

#ifndef __INTERNAL_GLUE__
#define __INTERNAL_GLUE__

// This file contains stubs of private headers that were needed to build
// Tracker independently of the BeOS build system.
// Care should be taken to make sure the structures in here are not out of date
// 

#include <List.h>
#include <OS.h>


// cruft from TypedList.h -----------------------

class PointerList : public BList {
public:
	PointerList();
	virtual ~PointerList();
	
	bool Owning() const;
private:
	const bool owning;
};

template<class T>
class TypedList : public PointerList {
public:
	virtual ~TypedList();
	
	void MakeEmpty();
	bool RemoveItem(T);
};

template<class T> 
TypedList<T>::~TypedList()
{
	if (Owning())
		// have to nuke elements first
		MakeEmpty();
}

template<class T> 
bool 
TypedList<T>::RemoveItem(T item)
{
	bool result = PointerList::RemoveItem((void *)item);
	
	if (result && Owning()) 
		delete item;

	return result;
}

template<class T> 
void 
TypedList<T>::MakeEmpty()
{
	if (Owning()) {
		int32 numElements = CountItems();
		
		for (int32 count = 0; count < numElements; count++)
			// this is probably not the most efficient, but
			// is relatively indepenent of BList implementation
			// details
			RemoveItem((T)PointerList::LastItem());
	}
	PointerList::MakeEmpty();
}

// from partition.h ------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint64	offset;				/* in device blocks */
	uint64	blocks;
	bool	data;				/* audio or data session */
} session_data;

typedef struct {
	char	partition_name[B_FILE_NAME_LENGTH];
	char	partition_type[B_FILE_NAME_LENGTH];
	char	file_system_short_name[B_FILE_NAME_LENGTH];
	char	file_system_long_name[B_FILE_NAME_LENGTH];
	char	volume_name[B_FILE_NAME_LENGTH];
	char	mounted_at[B_FILE_NAME_LENGTH];
	uint32	logical_block_size;
	uint64	offset;				/* in logical blocks from start of session */
	uint64	blocks;				/* in logical blocks */
	bool	hidden;				/* non-file system partition */
	uchar	partition_code;
	bool	reserved1;
	uint32	reserved2;
} partition_data;

/* Partition add-on entry points */
/*-------------------------------*/

typedef struct {
	bool	can_partition;
	bool	can_repartition;
} drive_setup_partition_flags;


#ifdef __cplusplus
}
#endif


#endif
