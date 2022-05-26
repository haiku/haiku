/*

PDF Writer printer driver.

Copyright (c) 2001 - 2008 Haiku.

Authors:
	Philippe Houdoin
	Simon Gauvin
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

#ifndef _PRINT_UTILS_H_
#define _PRINT_UTILS_H_


#include <List.h>
#include <MessageFilter.h>
#include <String.h>
#include <Rect.h>


class BHandler;
class BMessage;
class BWindow;


#define BEGINS_CHAR(byte) ((byte & 0xc0) != 0x80)


BRect ScaleRect(const BRect& rect, float scale);


// set or replace a value in a BMessage
void SetBool(BMessage* msg, const char* name, bool value);
void SetFloat(BMessage* msg, const char* name, float value);
void SetInt32(BMessage* msg, const char* name, int32 value);
void SetString(BMessage* msg, const char* name, const char* value);
void SetRect(BMessage* msg, const char* name, const BRect& rect);
void SetString(BMessage* msg, const char* name, const BString& value);
void AddFields(BMessage* to, const BMessage* from, const char* excludeList[] = NULL,
	const char* includeList[] = NULL, bool overwrite = true);


template <class T>
class TList {
private:
	BList fList;
	typedef int (*sort_func)(const void*, const void*);

public:
	virtual ~TList();
	void     MakeEmpty();
	int32    CountItems() const;
	T*       ItemAt(int32 index) const;
	void     AddItem(T* p);
	T*       RemoveItem(int i);
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
	for (int i = 0; i < n; i++) {
		delete ItemAt(i);
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
T* TList<T>::RemoveItem(int i) {
	return (T*)fList.RemoveItem(i);
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

#endif	// _PRINT_UTILS_H_
