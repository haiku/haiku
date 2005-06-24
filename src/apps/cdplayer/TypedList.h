/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// TypedList is a garden-variety of a type-safe template version of BList

#ifndef __LIST_TEMPLATE__
#define __LIST_TEMPLATE__

#ifndef _BE_H
#include <List.h>
#endif

#include <Debug.h>

class _PointerList : public BList {
public:
	_PointerList(const _PointerList &list);
	_PointerList(int32 itemsPerBlock = 20, bool owning = false);
	virtual ~_PointerList();

	typedef void *(* GenericEachFunction)(void *, void *);
	typedef int (* GenericCompareFunction)(const void *, const void *);
	void *EachElement(GenericEachFunction, void *);
	
	bool AddUnique(void *);
		// return true if item added or already in the list
	bool AddUnique(void *, GenericCompareFunction);

	bool Owning() const;
private:
	const bool owning;
};

// TypedList -
// to be used as a list of pointers to objects; this class should contain
// pretty much no code, just stubs that do proper type conversion
// it uses BetterEachBList for all of it's functionality and provides a 
// typed interface
template<class T>
class TypedList : public _PointerList {
public:
	TypedList(int32 itemsPerBlock = 20, bool owning = false);
	TypedList(const TypedList&);
	virtual ~TypedList();

	TypedList &operator=(const TypedList &);

	// iteration and sorting
	typedef T (* EachFunction)(T, void *);
	typedef const T (* ConstEachFunction)(const T, void *);
	typedef int (* CompareFunction)(const T *, const T *);

	// adding and removing
	bool AddItem(T);
	bool AddItem(T, int32);
	bool AddList(TypedList *);
	bool AddList(TypedList *, int32);
	bool AddUnique(T);
	bool AddUnique(T, CompareFunction);
	
	bool RemoveItem(T);
	T RemoveItem(int32);
	T RemoveItemAt(int32);
		// same as RemoveItem(int32), RemoveItem does not work when T is a scalar

	void MakeEmpty();

	// item access
	T ItemAt(int32) const;
	T ItemAtFast(int32) const;
		// does not do an index range check

	T FirstItem() const;
	T LastItem() const;
	
	T Items() const;

	// misc. getters
	int32 IndexOf(const T) const;
	bool HasItem(const T) const;
	bool IsEmpty() const;
	int32 CountItems() const;


	T EachElement(EachFunction, void *);
	const T EachElement(ConstEachFunction, void *) const;
		// Do for each are obsoleted by this list, possibly add
		// them for convenience
	void SortItems(CompareFunction);
	bool ReplaceItem(int32 index, T item);
	bool SwapItems(int32 a, int32 b);
	bool MoveItem(int32 from, int32 to);

private:
	friend class ParseArray;
};
	
template<class T> 
TypedList<T>::TypedList(int32 itemsPerBlock, bool owning)
	:	_PointerList(itemsPerBlock, owning)
{
}

template<class T> 
TypedList<T>::TypedList(const TypedList<T> &list)
	:	_PointerList(list)
{
	ASSERT(!list.Owning());
	// copying owned lists does not work yet
}

template<class T> 
TypedList<T>::~TypedList()
{
	if (Owning())
		// have to nuke elements first
		MakeEmpty();
}

template<class T> 
TypedList<T> &
TypedList<T>::operator=(const TypedList<T> &from)
{
	ASSERT(!from.Owning());
	// copying owned lists does not work yet

	return (TypedList<T> &)BList::operator=(from);
}

template<class T> 
bool 
TypedList<T>::AddItem(T item)
{
	return _PointerList::AddItem(item);
}

template<class T> 
bool 
TypedList<T>::AddItem(T item, int32 atIndex)
{
	return _PointerList::AddItem(item, atIndex);
}

template<class T> 
bool 
TypedList<T>::AddList(TypedList<T> *newItems)
{
	return _PointerList::AddList(newItems);
}

template<class T> 
bool
TypedList<T>::AddList(TypedList<T> *newItems, int32 atIndex)
{
	return _PointerList::AddList(newItems, atIndex);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item)
{
	return _PointerList::AddUnique(item);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item, CompareFunction function)
{
	return _PointerList::AddUnique(item, (GenericCompareFunction)function);
}


template<class T> 
bool 
TypedList<T>::RemoveItem(T item)
{
	bool result = _PointerList::RemoveItem(item);
	
	if (result && Owning()) {
		delete item;
	}

	return result;
}

template<class T> 
T 
TypedList<T>::RemoveItem(int32 index)
{
	return (T)_PointerList::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::RemoveItemAt(int32 index)
{
	return (T)_PointerList::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::ItemAt(int32 index) const
{
	return (T)_PointerList::ItemAt(index);
}

template<class T> 
T 
TypedList<T>::ItemAtFast(int32 index) const
{
	return (T)_PointerList::ItemAtFast(index);
}

template<class T> 
int32 
TypedList<T>::IndexOf(const T item) const
{
	return _PointerList::IndexOf(item);
}

template<class T> 
T 
TypedList<T>::FirstItem() const
{
	return (T)_PointerList::FirstItem();
}

template<class T> 
T 
TypedList<T>::LastItem() const
{
	return (T)_PointerList::LastItem();
}

template<class T> 
bool 
TypedList<T>::HasItem(const T item) const
{
	return _PointerList::HasItem(item);
}

template<class T> 
bool 
TypedList<T>::IsEmpty() const
{
	return _PointerList::IsEmpty();
}

template<class T> 
int32 
TypedList<T>::CountItems() const
{
	return _PointerList::CountItems();
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
			RemoveItem(LastItem());
	}
	_PointerList::MakeEmpty();
}

template<class T> 
T
TypedList<T>::EachElement(EachFunction func, void *params)
{ 
	return (T)_PointerList::EachElement((GenericEachFunction)func, params); 
}


template<class T> 
const T
TypedList<T>::EachElement(ConstEachFunction func, void *params) const
{ 
	return (const T)
		const_cast<TypedList<T> *>(this)->_PointerList::EachElement(
		(GenericEachFunction)func, params); 
}


template<class T>
T
TypedList<T>::Items() const
{
	return (T)_PointerList::Items();
}

template<class T> 
void
TypedList<T>::SortItems(CompareFunction function)
{ 
	ASSERT(sizeof(T) == sizeof(void *));
	_PointerList::SortItems((GenericCompareFunction)function);
}

template<class T>
bool TypedList<T>::ReplaceItem(int32 index, T item)
{
	return _PointerList::ReplaceItem(index, (void *)item);
}

template<class T>
bool TypedList<T>::SwapItems(int32 a, int32 b)
{
	return _PointerList::SwapItems(a, b);
}

template<class T>
bool TypedList<T>::MoveItem(int32 from, int32 to)
{
	return _PointerList::MoveItem(from, to);
}


#endif
