/*

TList.h

Copyright (c) 2002 OpenBeOS.

Author:
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/* 
TODO: Move this header file into a "common" directory!
This class is an minor extended version of the one found in
current/src/add-ons/print/drivers/pdf/source
*/

#ifndef _TLIST_H
#define _TLIST_H

#include <List.h>

template <class T>
class TList {
private:
	BList fList;
	bool  fOwnsItems;
	
	typedef int (*sort_func)(const void*, const void*);
	
public:
	TList(bool ownsItems = true) : fOwnsItems(ownsItems) { }
	virtual ~TList();
	void     MakeEmpty();
	int32    CountItems() const;
	T*       ItemAt(int32 index) const;
	void     AddItem(T* p);
	T*       Items();
	void     SortItems(int (*comp)(const T**, const T**));

};

// TList
template<class T>
TList<T>::~TList() {
	MakeEmpty();
}


template<class T>
void TList<T>::MakeEmpty() {
	const int32 n = CountItems();
	if (fOwnsItems) {
		for (int i = 0; i < n; i++) {
			delete ItemAt(i);
		}
	}
	fList.MakeEmpty();
}


template<class T>
int32 TList<T>::CountItems() const {
	return fList.CountItems();
}


template<class T>
T* TList<T>::ItemAt(int32 index) const {
	return (T*)fList.ItemAt(index);
}


template<class T>
void TList<T>::AddItem(T* p) {
	fList.AddItem(p);
}


template<class T>
T* TList<T>::Items() {
	return (T*)fList.Items();
}


template<class T>
void TList<T>::SortItems(int (*comp)(const T**, const T**)) {
	sort_func sort = (sort_func)comp;
	fList.SortItems(sort);
}

#endif
