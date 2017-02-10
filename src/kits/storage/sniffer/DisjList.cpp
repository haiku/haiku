//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file DisjList.cpp
	MIME sniffer Disjunction List class implementation
*/

#include <sniffer/DisjList.h>

using namespace BPrivate::Storage::Sniffer;

DisjList::DisjList()
	: fCaseInsensitive(false)
{
}

DisjList::~DisjList() {
}

void
DisjList::SetCaseInsensitive(bool how) {
	fCaseInsensitive = how;
}

bool
DisjList::IsCaseInsensitive() {
	return fCaseInsensitive;
}




