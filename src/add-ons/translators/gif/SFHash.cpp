////////////////////////////////////////////////////////////////////////////////
//
//	File: SFHash.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>

#include "SFHash.h"
#include <stdlib.h>
#include <stdio.h>

SFHash::SFHash(int size) {
	fatalerror = false;
	this->size = size;
	iterate_pos = iterate_depth = 0;
	main_array = (HashItem **)malloc(this->size * sizeof(HashItem *));

    if (main_array == NULL) {
		fatalerror = true;
		return;
	}
	for (int x = 0; x < this->size; x++) {
		main_array[x] = NULL;
	}
}

void SFHash::AddItem(HashItem *item) {
	item->next = NULL;
	int pos = item->key % size;
	
	if (main_array[pos] == NULL) {
		main_array[pos] = item;
	}
	else {
		HashItem *temp = main_array[pos];
		while (temp->next != NULL) temp = temp->next;
		temp->next = item;
	}
}

HashItem*
SFHash::GetItem(unsigned int key)
{
	int pos = key % size;
	HashItem *item = main_array[pos];
	
	while (item != NULL) {
		if (item->key == key) return item;
		item = item->next;
	}
	return NULL;
}

unsigned int
SFHash::CountItems()
{
    unsigned int count = 0;
    for (int x = 0; x < this->size; x++) {
        HashItem *item = main_array[x];
        while (item != NULL) {
            count++;
            item = item->next;
        }
    }
    return count;
}

HashItem*
SFHash::NextItem()
{
    if (iterate_pos >= size) {
        Rewind();
        return NULL;
    }
    HashItem *item;
    while ((item = main_array[iterate_pos]) == NULL) iterate_pos++;
    for (int d = 0; d < iterate_depth; d++) {
        item = item->next;
    }

    if (item->next != NULL) iterate_depth++;
    else {
        iterate_pos++;
        iterate_depth = 0;
    }
    return item;
}

void SFHash::Rewind() {
    iterate_pos = 0;
    iterate_depth = 0;
}

SFHash::~SFHash() {
	for (int x = 0; x < size; x++) {
		HashItem *item = main_array[x];
		while (item != NULL) {
			HashItem *t = item->next;
			delete item;
			item = t;
		}
	}
    free(main_array);
}

