/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBJECT_LIST_PRIVATE_H
#define _OBJECT_LIST_PRIVATE_H


#include <ObjectList.h>


template<class T, bool O>
class BObjectList<T, O>::Private {
public:
	Private(BObjectList<T, O>* objectList)
		:
		fObjectList(objectList)
	{
	}

	BList*
	AsBList()
	{
		return fObjectList;
	}

	const BList*
	AsBList() const
	{
		return fObjectList;
	}

private:
	BObjectList<T>* fObjectList;
};


#endif	// _OBJECT_LIST_PRIVATE_H
