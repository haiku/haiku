/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file DisjList.cpp
	MIME sniffer Disjunction List class implementation
*/

#include "DisjList.h"

using namespace BPrivate::Storage::Sniffer;


DisjList::DisjList()
	:
	fCaseInsensitive(false)
{
}


DisjList::~DisjList()
{
}


void
DisjList::SetCaseInsensitive(bool how)
{
	fCaseInsensitive = how;
}


bool
DisjList::IsCaseInsensitive()
{
	return fCaseInsensitive;
}
