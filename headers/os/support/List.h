// List.h
// Just here to be able to compile and test BResources.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#ifndef _sk_list_h_
#define _sk_list_h_

#include <SupportDefs.h>

class BList {
public:
	BList(int32 count = 20);
	BList(const BList& anotherList);
	virtual ~BList();

	bool AddItem(void *item, int32 index);
	bool AddItem(void *item);
	bool AddList(BList *list, int32 index);
	bool AddList(BList *list);
	int32 CountItems() const;
	void DoForEach(bool (*func)(void *));
	void DoForEach(bool (*func)(void *, void *), void *arg2);
	void *FirstItem() const;
	bool HasItem(void *item) const;
	int32 IndexOf(void *item) const;
	bool IsEmpty() const;
	void *ItemAt(int32 index) const;
	void *Items() const;
	void *LastItem() const;
	void MakeEmpty();
	bool RemoveItem(void *item);
	void *RemoveItem(int32 index);
	bool RemoveItems(int32 index, int32 count);
	void SortItems(int (*compareFunc)(const void *, const void *));

	BList& operator =(const BList &);

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

#endif // _sk_list_h_
