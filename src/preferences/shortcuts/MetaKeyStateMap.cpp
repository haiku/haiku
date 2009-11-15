/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "MetaKeyStateMap.h"


#include <stdio.h>
#include <string.h>


#include "BitFieldTesters.h"


MetaKeyStateMap::MetaKeyStateMap()
	:
	fKeyName(NULL)
{
	// User MUST call SetInfo() before using!
}


MetaKeyStateMap::MetaKeyStateMap(const char* name)
{
	SetInfo(name);
}


void
MetaKeyStateMap::SetInfo(const char* keyName)
{
	fKeyName = new char[strlen(keyName) + 1];
	strcpy(fKeyName, keyName);
}


MetaKeyStateMap::~MetaKeyStateMap()
{
	delete [] fKeyName;
	int nr = fStateDescs.CountItems();
	for (int i = 0; i < nr; i++) 
		delete [] ((const char*) fStateDescs.ItemAt(i));
	
	nr = fStateTesters.CountItems();
	for (int j = 0; j < nr; j++) 
		delete ((BitFieldTester*) fStateTesters.ItemAt(j));
	// _stateBits are stored in-line, no need to delete them
}


void
MetaKeyStateMap::AddState(const char* d, const BitFieldTester* t)
{
	char* copy = new char[strlen(d) + 1];
	strcpy(copy, d);
	fStateDescs.AddItem(copy);
	fStateTesters.AddItem((void *)t);
}


int
MetaKeyStateMap::GetNumStates() const
{
	return fStateTesters.CountItems();
}


const BitFieldTester*
MetaKeyStateMap::GetNthStateTester(int stateNum) const
{
	return ((const BitFieldTester*) fStateTesters.ItemAt(stateNum));
}


const char*
MetaKeyStateMap::GetNthStateDesc(int stateNum) const
{
	return ((const char*) fStateDescs.ItemAt(stateNum));
}


const char*
MetaKeyStateMap::GetName() const
{
	return fKeyName;
}

