/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBJECT_LIST_PRIVATE_H
#define _OBJECT_LIST_PRIVATE_H


#include <ObjectList.h>


template<class T>
class BObjectList<T>::Private {
public:
	Private(BObjectList<T>* objectList)
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
