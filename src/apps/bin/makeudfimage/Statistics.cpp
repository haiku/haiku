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

#include <stdio.h>

/*! \brief Returns a string describing the given number of bytes
	in the most appropriate units (i.e. bytes, KB, MB, etc.).
*/
static
std::string
bytes_to_string(uint64 bytes)
{
	const uint64 kb = 1024;			// kilo
	const uint64 mb = 1024 * kb;	// mega
	const uint64 gb = 1024 * mb;	// giga
	const uint64 tb = 1024 * gb;	// tera
	const uint64 pb = 1024 * tb;	// peta
	const uint64 eb = 1024 * pb;	// exa
	std::string units;
	uint64 divisor = 1;
	if (bytes >= eb) {
		units = "EB";
		divisor = eb;
	} else if (bytes >= pb) {
		units = "PB";
		divisor = pb;
	} else if (bytes >= tb) {
		units = "TB";
		divisor = tb;
	} else if (bytes >= gb) {
		units = "GB";
		divisor = gb;
	} else if (bytes >= mb) {
		units = "MB";
		divisor = mb;
	} else if (bytes >= kb) {
		units = "KB";
		divisor = kb;
	} else {	
		units = "bytes";
		divisor = 1;
	}
	double scaledValue = double(bytes) / double(divisor);
	char scaledString[10];
		// Should really only need 7 chars + NULL...
	sprintf(scaledString, "%.1f ", scaledValue);
	return std::string(scaledString) + units;	
}

/*! \brief Creates a new statistics object and sets the start time
	for the duration timer.
*/
Statistics::Statistics()
	: fStartTime(real_time_clock())
	, fDirectories(0)
	, fFiles(0)
	, fSymlinks(0)
	, fAttributes(0)
	, fDirectoryBytes(0)
	, fFileBytes(0)
	, fImageSize(0)
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


/*! \brief Returns a string describing the number of bytes
	allocated to directory data and metadata, displayed in the
	appropriate units (i.e. bytes, KB, MB, etc.).
*/
std::string
Statistics::DirectoryBytesString() const
{
	return bytes_to_string(DirectoryBytes());
}

/*! \brief Returns a string describing the number of bytes
	allocated to file data and metadata, displayed in the
	appropriate units (i.e. bytes, KB, MB, etc.).
*/
std::string
Statistics::FileBytesString() const
{
	return bytes_to_string(FileBytes());
}

/*! \brief Returns a string describing the total image size,
	displayed in the appropriate units (i.e. bytes, KB, MB, etc.).
*/
std::string
Statistics::ImageSizeString() const
{
	return bytes_to_string(ImageSize());
}

