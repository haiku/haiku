//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file cdrom.c
	disk_scanner session module for SCSI/ATAPI cdrom drives 
	
	The protocols used in this module are based on information taken
	from the "SCSI-3 Multimedia Commands" draft, revision 10A.
	
	The SCSI command of interest is "READ TOC/PMA/ATIP", command
	number \c 0x43.
	
	The format of interest for said command is "Full TOC", format
	number \c 0x2.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <fs_device.h>
#include <KernelExport.h>
#include <scsi.h>
#include <disk_scanner/session.h>

//#define DBG(y) (y)
#define DBG(y)

#define TRACE(x) DBG(dprintf x)

#define CDROM_SESSION_MODULE_NAME "disk_scanner/session/cdrom/v1"
static const char *kModuleDebugName = "session/cdrom";

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
const uint8 kFullTableOfContentsFormat = 0x10;	//!< "READ TOC/PMA/ATIP" format of interest
	
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
uint32
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

/*! Type of entries returned by "READ TOC/PMA/ATIP" when called with format
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

/*! Type of entries returned by "READ TOC/PMA/ATIP" when called with format
    \c kFullTableOfContentsFormat == 0x10
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

/*! Bitflags for control entries
*/
enum {
	kControlDataTrack = 0x4,
	kControlCopyPermitted = 0x2,
};

// std_ops
static
status_t
std_ops(int32 op, ...)
{
	TRACE(("%s: std_ops(0x%lx)\n", kModuleDebugName, op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

/*
static
void
dump_scsi_command(raw_device_command *cmd) {
	int i;
	uint j;
	scsi_table_of_contents_command *scsi_command = (scsi_table_of_contents_command*)(&(cmd->command));

	for (i = 0; i < cmd->command_length; i++)
		TRACE(("%.2x,", cmd->command[i]));
	TRACE(("\n"));

	TRACE(("raw_device_command:\n"));
	TRACE(("  command:\n"));
	TRACE(("    command = %d (0x%.2x)\n", scsi_command->command, scsi_command->command));
	TRACE(("    msf     = %d\n", scsi_command->msf));
	TRACE(("    format  = %d (0x%.2x)\n", scsi_command->format, scsi_command->format));
	TRACE(("    number  = %d\n", scsi_command->number));
	TRACE(("    length  = %d\n", B_BENDIAN_TO_HOST_INT16(scsi_command->length)));
	TRACE(("    control = %d\n", scsi_command->control));
	TRACE(("  command_length    = %d\n", cmd->command_length));
	TRACE(("  flags             = %d\n", cmd->flags));
	TRACE(("  scsi_status       = 0x%x\n", cmd->scsi_status));
	TRACE(("  cam_status        = 0x%x\n", cmd->cam_status));
	TRACE(("  data              = %p\n", cmd->data));
	TRACE(("  data_length       = %ld\n", cmd->data_length));
	TRACE(("  sense_data        = %p\n", cmd->sense_data));
	TRACE(("  sense_data_length = %ld\n", cmd->sense_data_length));
	TRACE(("  timeout           = %lld\n", cmd->timeout));
	TRACE(("data dump:\n"));
	for (j = 0; j < 2048; j++) {//cmd->data_length; j++) {
			uchar c = ((uchar*)cmd->data)[j];

			if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))
				TRACE(("\\%c,", c));
			else
				TRACE(("%.2x,", c));
	}
	TRACE(("\n"));
	TRACE(("sense_data dump:\n"));
	for (j = 0; j < cmd->sense_data_length; j++) {
			uchar c = ((uchar*)cmd->sense_data)[j];
			if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))
				TRACE(("%c", c));
			else if (c == 0)
				TRACE(("_"));
			else
				TRACE(("-"));
	}
	TRACE(("\n"));
}
*/

static
void
dump_full_table_of_contents(uchar *data, uint16 data_length)
{
	cdrom_table_of_contents_header *header;
	cdrom_full_table_of_contents_entry *entries;
	int i, count;
	int header_length;
	
	header = (cdrom_table_of_contents_header*)data;
	entries = (cdrom_full_table_of_contents_entry*)(data+4);
	header_length = B_BENDIAN_TO_HOST_INT16(header->length);
	if (data_length < header_length) {
		TRACE(("dump_full_table_of_contents: warning, data buffer not large enough (%d < %d)\n",
		       data_length, header_length));
		header_length = data_length;
	}
	
	TRACE(("%s: table of contents dump:\n", kModuleDebugName));
	TRACE(("--------------------------------------------------\n"));
	TRACE(("header:\n"));
	TRACE(("  length = %d\n", header_length));
	TRACE(("  first  = %d\n", header->first));
	TRACE(("  last   = %d\n", header->last));
	
	count = (header_length-2) / sizeof(cdrom_full_table_of_contents_entry);
	TRACE(("\n"));
	TRACE(("entry count = %d\n", count));
	
	for (i = 0; i < count; i++) {
		TRACE(("\n"));
		TRACE(("entry #%d:\n", i));
		TRACE(("  session  = %d\n", entries[i].session));
		TRACE(("  adr      = %d\n", entries[i].adr));
		TRACE(("  control  = %d (%s track, copy %s)\n", entries[i].control,
		       (entries[i].control & kControlDataTrack ? "data" : "audio"),
		       (entries[i].control & kControlCopyPermitted ? "permitted" : "prohibited")));
		TRACE(("  tno      = %d\n", entries[i].tno));
		TRACE(("  point    = %d (0x%.2x)\n", entries[i].point, entries[i].point));
		TRACE(("  minutes  = %d\n", entries[i].minutes));
		TRACE(("  frames   = %d\n", entries[i].seconds));
		TRACE(("  seconds  = %d\n", entries[i].frames));
		TRACE(("  zero     = %d\n", entries[i].zero));
		TRACE(("  pminutes = %d\n", entries[i].pminutes));
		TRACE(("  pseconds = %d\n", entries[i].pseconds));
		TRACE(("  pframes  = %d\n", entries[i].pframes));
		TRACE(("  lba      = %ld\n", msf_to_lba(make_msf_address(entries[i].pminutes,
		                              entries[i].pseconds, entries[i].pframes))));
	}
	TRACE(("--------------------------------------------------\n"));
}

// read_table_of_contents
static
status_t
read_table_of_contents(int deviceFD, uint32 first_session, uchar *buffer,
                       uint16 buffer_length, bool msf)
{
	scsi_table_of_contents_command scsi_command;
	raw_device_command raw_command;
	const uint32 sense_data_length = 1024;
	uchar sense_data[sense_data_length];
	status_t error = buffer ? B_OK : B_BAD_VALUE;
	
	TRACE(("%s: read_table_of_contents: (%d, %p, %d)\n", kModuleDebugName,
	       deviceFD, buffer, buffer_length));

	if (error)
		return error;
		
	// Init the scsi command and copy it into the BeOS "raw scsi command" ioctl struct
	memset(raw_command.command, 0, 16);
	scsi_command.command = 0x43;
	scsi_command.msf = 0;
	scsi_command.format = 2;
	scsi_command.number = first_session;
	scsi_command.length = B_HOST_TO_BENDIAN_INT16(buffer_length);
	scsi_command.control = 0;	
	scsi_command.reserved0 = scsi_command.reserved1 = scsi_command.reserved2
	                        = scsi_command.reserved3 = scsi_command.reserved4
	                        = scsi_command.reserved5 = scsi_command.reserved6 = 0;
	memcpy(raw_command.command, &scsi_command, sizeof(scsi_command));

	// Init the rest of the raw command
	raw_command.command_length = 10;
	raw_command.flags = kScsiFlags;
	raw_command.scsi_status = 0;	
	raw_command.cam_status = 0;
	raw_command.data = buffer;
	raw_command.data_length = buffer_length;
	memset(raw_command.data, 0, raw_command.data_length);
	raw_command.sense_data = sense_data;
	raw_command.sense_data_length = sense_data_length;
	memset(raw_command.sense_data, 0, raw_command.sense_data_length);
	raw_command.timeout = kScsiTimeout;	
		
	if (ioctl(deviceFD, B_RAW_DEVICE_COMMAND, &raw_command) == 0) {
		if (raw_command.scsi_status == 0 && raw_command.cam_status == 1) {
			// SUCCESS!!!
			DBG(dump_full_table_of_contents(buffer, buffer_length));
		} else {
			error = B_FILE_ERROR;
			TRACE(("%s: scsi ioctl succeeded, but scsi command failed\n",
			       kModuleDebugName));
		}
	} else {
		error = errno;
		TRACE(("%s: scsi command failed with error 0x%lx\n", kModuleDebugName,
		       error));
	}

	return error;
	
}

// cdrom_session_identify
static
bool
cdrom_session_identify(int deviceFD, off_t deviceSize, int32 blockSize)
{
	device_geometry geometry;
	bool result = false;
	if (ioctl(deviceFD, B_GET_GEOMETRY, &geometry) == 0) 
		result = (geometry.device_type == B_CD);
	else
		dprintf("WARNING: %s: Unable to get device geometry\n", kModuleDebugName);
	return result;
}

// cdrom_session_get_nth_info
/*! \todo Check that read_toc buffer was large enough on success
	\todo Politley ignore states not in { 0xa0,0xa1,0xa2,1-99 }
*/
static
status_t
cdrom_session_get_nth_info(int deviceFD, int32 index, off_t deviceSize,
                   int32 blockSize, struct session_info *sessionInfo)
{
	status_t error = sessionInfo && index >= 0 ? B_OK : B_BAD_VALUE;
	uchar data[2048];
	int32 session = index+1;
	
//	bool found_track = false;
//	bool found_start = false;
//	bool found_end = false;

	off_t start_lba = 0;
	off_t end_lba = 0;

	TRACE(("%s: get_nth_info(%d, %ld, %lld, %ld, %p)\n", kModuleDebugName,
		   deviceFD, index, deviceSize, blockSize, sessionInfo));

	// Attempt to read the table of contents, first in lba mode, then in msf mode
	if (!error) {
		error = read_table_of_contents(deviceFD, 1, data, 2048, false);
	}	
	if (error) {
		TRACE(("%s: lba read_toc failed, trying msf instead\n", kModuleDebugName));
		error = read_table_of_contents(deviceFD, 1, data, 2048, true);
	}
		
	// Interpret the data returned, if successful
	if (!error) {
		cdrom_table_of_contents_header *header;
		cdrom_full_table_of_contents_entry *entries;
		int i, count;

		header = (cdrom_table_of_contents_header*)data;
		entries = (cdrom_full_table_of_contents_entry*)(data+4);
		header->length = B_BENDIAN_TO_HOST_INT16(header->length);
		
		count = (header->length-2) / sizeof(cdrom_full_table_of_contents_entry);
		
		// Check for a valid session index
		if (session < 1 || session > 99)
			error = B_ENTRY_NOT_FOUND;
		
		// Extract the data of interest
		if (!error) {			
			uint32 current_session = 0;		// The number given the current session
			uint32 current_toc_session = 0;	// The number given the current "session"
			uint first_track = 0;			// Track # of first track in "session"
			uint last_track = 0;			// Track # of last track in "session"
			off_t lead_out_lba;				// The address of the end of the current "session"
			enum {
				cdromStart,
				cdromFoundFirstTrack,
				cdromFoundLastTrack,
				cdromFoundLeadOut,
				cdromNeedEndLBA,
				cdromFinished,
				cdromError,
			} state = cdromStart;
			
			for (i = 0; i < count && state != cdromError && state != cdromFinished; i++) {
				// We're only interested in positional info for data tracks
				if (entries[i].control != 0x04 && entries[i].control != 0x06) {
				    	// 0x04 == data track, digital copy prohibited
						// 0x06 == data track, digital copy allowed
					TRACE(("%s: WARNING: session %d, point %d not a data track point\n",
					       kModuleDebugName, entries[i].session, entries[i].point));
				}
				if (entries[i].adr == 0x01)
					// 0x01 == positional information
				{
					switch (entries[i].point) {
						// first track number (in pmin)
						case 0xa0:	
							if (entries[i].session == current_toc_session+1) {
								current_toc_session++;
								first_track = entries[i].pminutes;
								state = cdromFoundFirstTrack;
							} else {
								// something screwy
								state = cdromError;
								TRACE(("%s: Point 0xA0 found in state %d\n",
								       kModuleDebugName, state));
							}
							break;

						// last track number (in pmin)
						case 0xa1:	
							if (entries[i].session == current_toc_session
							    && state == cdromFoundFirstTrack)
							{
								last_track = entries[i].pminutes;
								state = cdromFoundLastTrack;
							} else {
								// something screwy
								state = cdromError;
								TRACE(("%s: Point 0xA1 found in state %d\n",
								       kModuleDebugName, state));
							}
							break;

						// start position of lead-out (in pmsf)
						case 0xa2:
							if (entries[i].session == current_toc_session
							    && state == cdromFoundLastTrack)
							{
								lead_out_lba = msf_to_lba(make_msf_address(
								                          entries[i].pminutes,
								                          entries[i].pseconds,
								                          entries[i].pframes));
								state = cdromFoundLeadOut;
							} else {
								// something screwy
								state = cdromError;
								TRACE(("%s: Point 0xA2 found in state %d\n",
								       kModuleDebugName, state));
							}						            
							break;
								
						default:
							// start position of track (in pmsf)
							if (1 <= entries[i].point && entries[i].point <= 99
									// valid track number
							    && entries[i].session == current_toc_session)
							{
								switch (state) {
									case cdromFoundLeadOut:
										current_session++;
										if ((int)current_session == session) {
											// This is the session we're interested in
											start_lba = msf_to_lba(make_msf_address(
											                       entries[i].pminutes,
										                           entries[i].pseconds,
										                           entries[i].pframes));
											if (entries[i].point == last_track) {
												// Hooray! This the last track in the "session", so
												// we already know the end_lba
												end_lba = lead_out_lba;
												state = cdromFinished;
											} else {
												// Damn, we have to look up the start of the next
												// track still...
												state = cdromNeedEndLBA;
											}
										}
										break;
										
									case cdromNeedEndLBA:
										end_lba = msf_to_lba(make_msf_address(
										                     entries[i].pminutes,
									                         entries[i].pseconds,
									                         entries[i].pframes));
										state = cdromFinished;
										break;
										
									default:
										// something screwy
										state = cdromError;
										TRACE(("%s: Session of interest found in state %d\n",
										       kModuleDebugName, state));
										break;
								}
								
							} else {
								// something screwy
								state = cdromError;
								if (entries[i].session == current_toc_session) {
									TRACE(("%s: New session encountered with track information; "
									       " session == %d, previous session == %d, track == %d\n",
									       kModuleDebugName, entries[i].session, current_toc_session,
									       entries[i].point));
								} else {
									TRACE(("%s: Invalid track encountered: %d\n", kModuleDebugName,
										   entries[i].point));
								}
							}						            
							break;
					}
				}
			}			
			
			if (state == cdromFinished) {
				TRACE(("%s: found session #%ld info\n", kModuleDebugName, session));
				sessionInfo->offset = start_lba * blockSize;
				sessionInfo->size = (end_lba - start_lba) * blockSize;
				sessionInfo->logical_block_size = blockSize;
				sessionInfo->index = index;
				sessionInfo->flags = B_DATA_SESSION;	// possibly B_AUDIO_SESSION for audio tracks?
			} else {
				TRACE(("%s: error reading table of contents, ended up in state %d\n",
				       kModuleDebugName, state));
				error = B_ENTRY_NOT_FOUND;
			}
		}
	}
	
	if (error)	
		TRACE(("%s: get_nth error 0x%lx\n", kModuleDebugName, error));
		
	return error;
}

static session_module_info cdrom_session_module = 
{
	// module_info
	{
		CDROM_SESSION_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	cdrom_session_identify,
	cdrom_session_get_nth_info
};

_EXPORT session_module_info *modules[] =
{
	&cdrom_session_module,
	NULL
};

