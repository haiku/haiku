/*

PDF Writer printer driver.

Copyright (c) 2001, 2002 OpenBeOS. 

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

#ifndef _UTILS_H
#define _UTILS_H

#include <Window.h>
#include <Message.h>
#include <MessageFilter.h>


class EscapeMessageFilter : public BMessageFilter 
{
private:
	BWindow *fWindow;
	int32    fWhat;
		
public:
	EscapeMessageFilter(BWindow *window, int32 what);
	filter_result Filter(BMessage *msg, BHandler **target);
};


class HWindow : public BWindow
{
protected:
	void Init(uint32 escape_msg);
	
public:
	typedef BWindow 		inherited;

	HWindow(BRect frame, const char *title, window_type type, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE, uint32 escape_msg = B_QUIT_REQUESTED);
	HWindow(BRect frame, const char *title, window_look look, window_feel feel, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE, uint32 escape_msg = B_QUIT_REQUESTED);
	
	virtual void MessageReceived(BMessage* m);
	virtual void AboutRequested();
};

#define BEGINS_CHAR(byte) ((byte & 0xc0) != 0x80)


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
T* TList<T>::Items() {
	return (T*)fList.Items();
}


template<class T>
void TList<T>::SortItems(int (*comp)(const T**, const T**)) {
	sort_func sort = (sort_func)comp;
	fList.SortItems(sort);
}

// PDF coordinate system
class PDFSystem {
private:
	float fHeight;
	float fX;
	float fY;
	float fScale;

public:
	PDFSystem()
		: fHeight(0), fX(0), fY(0), fScale(1) { }
	PDFSystem(float h, float x, float y, float s)
		: fHeight(h), fX(x), fY(y), fScale(s) { }

	void   SetHeight(float h)          { fHeight = h; }
	void   SetOrigin(float x, float y) { fX = x; fY = y; }
	void   SetScale(float scale)       { fScale = scale; }
	float  Height() const              { return fHeight; }
	BPoint Origin() const              { return BPoint(fX, fY); }
	float  Scale() const               { return fScale; }
	
	inline float tx(float x)    { return fX + fScale*x; }
	inline float ty(float y)    { return fHeight - (fY + fScale * y); }
	inline float scale(float f) { return fScale * f; }
};


#endif
