//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Statistics.h

	BDataIO wrapper around a given attribute for a file. (declarations)
*/

#ifndef _STATISTICS_H
#define _STATISTICS_H

#include <OS.h>
#include <string>
#include <SupportDefs.h>

std::string bytes_to_string(uint64 bytes);

class Statistics {
public:
	Statistics();
	void Reset();
	
	time_t StartTime() const { return fStartTime; }
	time_t ElapsedTime() const { return real_time_clock() - fStartTime; }
	std::string ElapsedTimeString() const;
	
	void AddDirectory() { fDirectories++; }
	void AddFile() { fFiles++; }
	void AddSymlink() { fSymlinks++; }
	void AddAttribute() { fAttributes++; }
	
	void AddDirectoryBytes(uint64 count) { fDirectoryBytes += count; }
	void AddFileBytes(uint64 count) { fFileBytes += count; }

	void SetImageSize(uint64 size) { fImageSize = size; }

	uint64 Directories() const { return fDirectories; }
	uint64 Files() const { return fFiles; }
	uint64 Symlinks() const { return fSymlinks; }
	uint64 Attributes() const { return fAttributes; }
	
	uint64 DirectoryBytes() const { return fDirectoryBytes; }
	uint64 FileBytes() const { return fFileBytes; }
	std::string DirectoryBytesString() const;
	std::string FileBytesString() const;
	
	uint64 ImageSize() const { return fImageSize; }
	std::string ImageSizeString() const;
private:
	time_t fStartTime;
	uint64 fDirectories;
	uint64 fFiles;
	uint64 fSymlinks;
	uint64 fAttributes;
	uint64 fDirectoryBytes;
	uint64 fFileBytes;
	uint64 fImageSize;
};

#endif	// _STATISTICS_H
