//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		FMWList.h
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//	Description:	List class for something or other (according to DW)
//  
//------------------------------------------------------------------------------
#ifndef _FMWLIST_H_
#define _FMWLIST_H_

#include <List.h>
#include <Window.h>

class FMWList
{
public:
	FMWList(void);
	virtual ~FMWList(void);
	void AddItem(void *item);
	bool HasItem(void *item) const;
	bool RemoveItem(void* item);
	void *ItemAt(int32 i) const;
	int32 CountItems(void) const;
	void *LastItem(void) const;
	void *FirstItem(void) const;
	int32 IndexOf(void *item);

	// special
	void AddFMWList(FMWList *list);
	
	// debugging methods
	void PrintToStream() const;

private:
	BList fList;
};

#endif
