/* Filter - the base class for all mail filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <String.h>

class _EXPORT BMailFilter;

#include <MailAddon.h>

BMailFilter::BMailFilter(BMessage *)
{
	//----do nothing-----
}

BMailFilter::~BMailFilter()
{
}


void BMailFilter::_ReservedFilter1() {}
void BMailFilter::_ReservedFilter2() {}
void BMailFilter::_ReservedFilter3() {}
void BMailFilter::_ReservedFilter4() {}

