/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tyler Akidau, haiku@akidau.net
 */
#ifndef _DISC_H
#define _DISC_H


#include <errno.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <KernelExport.h>
#include <scsi.h>

#include "Debug.h"
#include "scsi-mmc.h"


class List;
class Session;


/*! \brief A class that encapsulates a complete, parsed, and error
	checked table of contents for a CD-ROM.
*/
class Disc {
public:
								Disc(int fd);
								~Disc();

			status_t			InitCheck();
			Session*			GetSession(int32 index);

			void				Dump();

	// CDs and DVDs are required to have a block size of 2K by
	// the SCSI-3 standard
	static	const int			kBlockSize = 2048;

private:
			uint32				_AdjustForYellowBook(
									cdrom_full_table_of_contents_entry
										entries[],
									uint32 count);
			status_t			_ParseTableOfContents(
									cdrom_full_table_of_contents_entry
										entries[],
									uint32 count);
			void				_SortAndRemoveDuplicates();
			status_t			_CheckForErrorsAndWarnings();

private:
			status_t			fInitStatus;
			List*				fSessionList;
};


/*! \brief Encapsulates the pertinent information for a given session
	on a Disc.
*/
class Session {
public:
								~Session();

			off_t				Offset() { return fOffset; }
			off_t				Size() { return fSize; }
			uint32				BlockSize() { return fBlockSize; }
			int32				Index() { return fIndex; }
			uint32				Flags() { return fFlags; }
			const char*			Type() { return fType; }

private:
	friend class Disc;

								Session(off_t offset, off_t size,
									uint32 blockSize, int32 index, uint32 flags,
									const char* type);

								Session(const Session& other);
									// not implemented
			Session&			operator=(const Session& other);
									// not implemented

private:
			off_t				fOffset;
			off_t				fSize;
			uint32				fBlockSize;
			int32				fIndex;
			uint32				fFlags;
			char*				fType;
};


#endif	// _DISC_H
