//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
/*!
	\file Disc.cpp

	Disc class, used to enumerate the CD/DVD sessions.
*/
	
#ifndef _DISC_H
#define _DISC_H

#include <errno.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <disk_scanner.h>
#include <KernelExport.h>
#include <scsi.h>

#include "Debug.h"
#include "scsi-mmc.h"

class List;

/*! \brief A class that encapsulates a complete, parsed, and error
	checked table of contents for a CD-ROM.
*/
class Disc {
public:
	Disc(int fd);
	~Disc();
	
	status_t InitCheck();
	status_t GetSessionInfo(int32 index);
	
	void Dump();
	
	// CDs and DVDs are required to have a block size of 2K by
	// the SCSI-3 standard
	static const int kBlockSize = 2048;
	
private:
	status_t _ParseTableOfContents(cdrom_full_table_of_contents_entry entries[],
	                              uint32 count);
	void _SortAndRemoveDuplicates();
	status_t _CheckForErrorsAndWarnings();

	status_t fInitStatus;
	List *fSessionList;
};

#endif	// _DISC_H
