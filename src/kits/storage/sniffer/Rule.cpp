//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Rule.cpp
	MIME sniffer rule implementation
*/

#include <sniffer/Rule.h>

using namespace Sniffer;

Rule::Rule()
	: fPriority(0.0)
{
	// Not implemented
}

Rule::Rule(const char *rule)
	: fPriority(0.0)
{
	// Parse the rule here ??? 
}

status_t
Rule::SetTo(const char *rule) {
	return B_ERROR;	// Not implemented
}



