/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __STRING_LIST_H
#define __STRING_LIST_H

#include <List.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class StringList
{
public:
			StringList();
			~StringList();
	
			// AddItem makes a copy of the data
			void AddItem(const char *string);

			// either the String or NULL
			const char *ItemAt(int index) const;
			
			void MakeEmpty();
			
private:
	BList	list;
};


inline
StringList::StringList()
 :	list()
{
}


inline
StringList::~StringList()
{
	MakeEmpty();
}


inline void
StringList::AddItem(const char *string)
{
	list.AddItem(strdup(string));
}


inline const char *
StringList::ItemAt(int index) const
{
	return (const char *)list.ItemAt(index);
}
			

inline void
StringList::MakeEmpty()
{
	int i = list.CountItems();
	while (--i > -1)
		free(list.RemoveItem(i));
}


#endif
