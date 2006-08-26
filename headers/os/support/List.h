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
//	File Name:		List.h
//	Author(s):		The Storage kit Team
//	Description:	BList class provides storage for pointers.
//					Not thread safe..
//------------------------------------------------------------------------------

#ifndef _BE_LIST_H 
#define _BE_LIST_H

#include <SupportDefs.h>

/*----- BList class --------------------------------------*/
class BList {

public:	
	BList(int32 count = 20);
	BList(const BList& anotherList);
	virtual ~BList();

	BList& operator =(const BList &);
	
/* Adding and removing items. */
	bool AddItem(void *item, int32 index);
	bool AddItem(void *item);
	bool AddList(const BList *list, int32 index);
	bool AddList(const BList *list);
	bool RemoveItem(void *item);
	void *RemoveItem(int32 index);
	bool RemoveItems(int32 index, int32 count);
	bool ReplaceItem(int32 index, void *newItem);
	void MakeEmpty();
	
/* Reordering items. */
	void SortItems(int (*compareFunc)(const void *, const void *));
	bool SwapItems(int32 indexA, int32 indexB);
	bool MoveItem(int32 fromIndex, int32 toIndex);

/* Retrieving items. */
	void *ItemAt(int32 index) const;
	void *FirstItem() const;
	
	/* Be careful when using this function, since it doesn't */
	/* check if the index you pass is valid, and can lead to */
	/* unexpected results when it isn't.					 */
	void *ItemAtFast(int32) const;
	
	void *LastItem() const;
	void *Items() const;

/* Querying the list. */	
	bool HasItem(void *item) const;
	int32 IndexOf(void *item) const;
	int32 CountItems() const;
	bool IsEmpty() const;
	
/* Iterating over the list. */	
	void DoForEach(bool (*func)(void *));
	void DoForEach(bool (*func)(void *, void *), void *arg2);	
	
/*----- Private or reserved ---------------*/
	
private:
	virtual void _ReservedList1();
	virtual void _ReservedList2();

	// return type differs from BeOS version
	bool Resize(int32 count);

private:
	void**	fObjectList;
	size_t	fPhysicalSize;
	int32	fItemCount;
	int32	fBlockSize;
	uint32	_reserved[2];
};

#endif // _BE_LIST_H 
