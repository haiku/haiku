//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Statistics.h

	BDataIO wrapper around a given attribute for a file. (implementation)
*/

#include "Statistics.h"

/*! \brief Creates a new statistics object and sets the start time
	for the duration timer.
*/
Statistics::Statistics()
	: fDirectories(0)
	, fFiles(0)
	, fSymlinks(0)
	, fAttributes(0)
	, fDirectoryBytes(0)
	, fFileBytes(0)
	, fStartTime(real_time_clock())
{
}

/*! \brief Resets all statistics fields, including the start time of
	the duration timer.
*/
void
Statistics::Reset()
{
	Statistics null;
	*this = null;
}
