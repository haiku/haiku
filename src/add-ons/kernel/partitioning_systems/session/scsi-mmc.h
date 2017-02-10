//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _SCSI_MMC_H
#define _SCSI_MMC_H

/*!
	\file scsi-mmc.h
	SCSI-3 MMC support structures. 
	
	The protocols followed in this module are based on information
	taken from the "SCSI-3 Multimedia Commands" draft, revision 10A.
	
	The SCSI command of interest is "READ TOC/PMA/ATIP", command
	number \c 0x43.
	
	The format of interest for said command is "Full TOC", format
	number \c 0x2.
*/

#include <errno.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <KernelExport.h>
#include <scsi.h>

#include "Debug.h"

/*! \brief raw_device_command flags

	This is the only combination of flags that works on my system. :-)
*/
const uint8 kScsiFlags = B_RAW_DEVICE_DATA_IN
                           | B_RAW_DEVICE_REPORT_RESIDUAL
                           | B_RAW_DEVICE_SHORT_READ_VALID;

/*! \brief Timeout value used when making scsi commands

	I'm honestly not sure what value to use here. It's currently
	1 second because I got sick of waiting for the timeout while
	figuring out how to get the commands to work.
*/
const uint32 kScsiTimeout = 1000000;

//! SCSI "READ TOC/PMA/ATIP" command struct
typedef struct {
	uint8 command;		//!< 0x43 == READ TOC/PMA/ATIP
	uint8 reserved2:1,
	      msf:1,		//!< addressing format: 0 == LBA, 1 == MSF; try lba first, then msf if that fails
	      reserved1:3,
	      reserved0:3;
	uint8 format:4,     //!< sub-command: 0x0 == "TOC", 0x1 == "Session Info", 0x2 == "Full TOC", ...
	      reserved3:4;		
	uint8 reserved4;
	uint8 reserved5;
	uint8 reserved6;
	uint8 number;		//!< track/session number
	uint16 length;		//!< length of data buffer passed in raw_device_command.data; BIG-ENDIAN!!!
	uint8 control;		//!< control codes; 0x0 should be fine
} __attribute__((packed)) scsi_table_of_contents_command; 

// Some of the possible "READ TOC/PMA/ATIP" formats
const uint8 kTableOfContentsFormat = 0x00;
const uint8 kSessionFormat = 0x01;
const uint8 kFullTableOfContentsFormat = 0x02;	//!< "READ TOC/PMA/ATIP" format of interest
	
/*! \brief Minutes:Seconds:Frames format address

	- Each msf frame corresponds to one logical block.
	- Each msf second corresponds to 75 msf frames
	- Each msf minute corresponds to 60 msf seconds
	- Logical block 0 is at msf address 00:02:00
*/
typedef struct {
	uint8 reserved;
	uint8 minutes;
	uint8 seconds;
	uint8 frames;
} msf_address;

#define CDROM_FRAMES_PER_SECOND (75)
#define CDROM_FRAMES_PER_MINUTE (CDROM_FRAMES_PER_SECOND*60)

/*! \brief Returns an initialized \c msf_address struct
*/
static
inline
msf_address
make_msf_address(uint8 minutes, uint8 seconds, uint8 frames)
{
	msf_address result;
	result.reserved = 0;
	result.minutes = minutes;
	result.seconds = seconds;
	result.frames = frames;
	return result;
}

/*! \brief Converts the given msf address to lba format
*/
static
inline
off_t
msf_to_lba(msf_address msf)
{
	return (CDROM_FRAMES_PER_MINUTE * msf.minutes)
	       + (CDROM_FRAMES_PER_SECOND * msf.seconds)
	       + msf.frames - 150;
}

/*! \brief Header for data returned by all formats of SCSI
    "READ TOC/PMA/ATIP" command.
*/
typedef struct {
	uint16 length;	//!< Length of data in reply (not including these 2 bytes); BIG ENDIAN!!!
	uint8 first;	//!< First track/session in reply
	uint8 last;		//!< Last track/session in reply
} cdrom_table_of_contents_header;

/*! \brief Type of entries returned by "READ TOC/PMA/ATIP" when called with format
    \c kTableOfContentsFormat == 0x00
*/
typedef struct {
	uint8 reserved0;
	uint8 control:4,
	      adr:4;
	uint8 track_number;
	uint8 reserved1;
	uint32 address;
} cdrom_table_of_contents_entry;

/*! \brief Type of entries returned by "READ TOC/PMA/ATIP" when called with format
    \c kFullTableOfContentsFormat == 0x02
*/
typedef struct {
	uint8 session;
	uint8 control:4,
	      adr:4;
	uint8 tno;
	uint8 point;
	uint8 minutes;
	uint8 seconds;
	uint8 frames;
	uint8 zero;
	uint8 pminutes;
	uint8 pseconds;
	uint8 pframes;
} cdrom_full_table_of_contents_entry;

/*! \brief Bitflags for control entries
*/
enum {
	kControlDataTrack = 0x4,
	kControlCopyPermitted = 0x2,
};

#endif	// _SCSI_MMC_H
