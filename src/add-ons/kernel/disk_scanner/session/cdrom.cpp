//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file cdrom.cpp
	disk_scanner session module for SCSI/ATAPI cdrom drives 
	
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
#include <disk_scanner.h>
#include <KernelExport.h>
#include <scsi.h>

extern "C" {
	#include <disk_scanner/session.h>
}

//#define DBG(y) (y)
#define DBG(y)

#define TRACE(x) DBG(dprintf x)

// These macros turn on or off extensive and generally unnecessary
// debugging output regarding table of contents parsing
#define WARN(x) (dprintf x)
//#define WARN(x)

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

/*! \brief Bitflags for control entries
*/
enum {
	kControlDataTrack = 0x4,
	kControlCopyPermitted = 0x2,
};

extern "C" {
	static status_t std_ops(int32 op, ...);
	static bool cdrom_session_identify(int deviceFD, off_t deviceSize, int32 blockSize);
	static status_t cdrom_session_get_nth_info(int deviceFD, int32 index, off_t deviceSize,
                                               int32 blockSize, struct session_info *sessionInfo);
}

/*! \brief An item that can be stored in a List object.
*/
struct list_item {
public:
	list_item(uint32 index, list_item *next = NULL)
		: index(index)
		, next(next)
	{
	};

	int32 index;
	list_item *next;
};

/*! \brief A simple, singly linked list.
*/
class List {
public:
	List();
	~List();
	
	list_item* Find(int32 index) const;
	void Add(list_item *item);
	void Clear();
	void SortAndRemoveDuplicates();
	
	list_item* First() const;
	list_item* Last() const;
	
private:
	list_item *fFirst;
	list_item *fLast;
};

/*! \brief Keeps track of track information.

	Can be stored in a List object.
*/
struct track : public list_item {
public:
	track(uint32 index, off_t startLBA, uint8 control, uint8 adr, track *next = NULL)
		: list_item(index, next)
		, start_lba(startLBA)
		, control(control)
		, adr(adr)
	{
	}
	
	off_t start_lba;
	uint8 control;		//!< only used to give what are probably useless warnings
	uint8 adr;			//!< only used to give what are probably useless warnings
};

/*! \brief Keeps track of session information.

	Can be stored in a List object.
*/
struct session : public list_item {
public:
	session(uint32 index, session *next = NULL);
	
	bool first_track_hint_is_set();
	bool last_track_hint_is_set();
	bool end_lba_is_set();	// also implies control and adr are set
	
	bool is_audio();

	int8 first_track_hint;
	int8 last_track_hint;
	int8 control;
	int8 adr;
	off_t end_lba;
	
	List track_list;
};

/*! \brief A class that encapsulates a complete, parsed, and error
	checked table of contents for a CD-ROM.
*/
class Disc {
public:
	Disc(cdrom_full_table_of_contents_entry entries[], uint32 count);
	
	status_t InitCheck();
	status_t GetSessionInfo(int32 index, int32 blockSize, session_info *sessionInfo);
	
	void Dump();
	
private:
	status_t ParseTableOfContents(cdrom_full_table_of_contents_entry entries[],
	                              uint32 count);
	void SortAndRemoveDuplicates();
	status_t CheckForErrorsAndWarnings();

	status_t fInitStatus;
	List fSessionList;
};

//------------------------------------------------------------------------------
// List
//------------------------------------------------------------------------------

/*! \brief Creates an empty list.
*/
List::List()
	: fFirst(NULL)
	, fLast(NULL)
{
}

List::~List()
{
	Clear();
}

/*! \brief Returns the ListItem with the given index, or NULL if not found.
*/
list_item*
List::Find(int32 index) const
{
//	TRACE(("%s: List::Find(%ld)\n", kModuleDebugName, index));
	list_item *item = fFirst;
	while (item && item->index != index) {	
		item = item->next;
	}
	return item;
}

/*! \brief Adds the given item to the end of the list.

	\param item The item to add (may not be NULL)
*/
void
List::Add(list_item *item)
{
//	TRACE(("%s: List::Add(%p)\n", kModuleDebugName, item));
	if (item) {
		item->next = NULL;
		if (fLast) {
			fLast->next = item;
			fLast = item;
		} else {
			fFirst = fLast = item;
		}
	} else {
		TRACE(("%s: List::Add(): NULL item parameter\n", kModuleDebugName));
	}
}

/*! \brief Clears the list.
*/
void
List::Clear()
{
	list_item *item = fFirst;
	while (item) {
		list_item *next = item->next;
		delete item;
		item = next;
	}
	fFirst = fLast = NULL;
}

/*! \brief Bubble sorts the list by index, removing any duplicates
	(the first instance is kept).
	
	\todo I believe duplicate removal is actually unnecessary, but I need to verify that.
*/
void
List::SortAndRemoveDuplicates()
{
	bool sorted = false;
	while (!sorted) {
		sorted = true;

		list_item *prev = NULL;
		list_item *item = fFirst;
		list_item *next = NULL;
		while (item && item->next) {
			next = item->next;
//			dprintf("List::Sort: %ld -> %ld\n", item->index, next->index);
			if (item->index > next->index) {
				sorted = false;
				
				// Keep fLast up to date
				if (next == fLast)
					fLast = item;

				// Swap					
				if (prev) {
					// item is not fFirst
					prev->next = next;
					item->next = next->next;
					next->next = item;				
				} else {
					// item must be fFirst
					fFirst = next;
					item->next = next->next;
					next->next = item;
				}			
			} else if (item->index == next->index) {
				// Duplicate indicies
				TRACE(("%s: List::SortAndRemoveDuplicates: duplicate indicies found (#%ld); "
				       "keeping first instance\n", kModuleDebugName, item->index));
				item->next = next->next;
				delete next;
				next = item->next;
				continue;			
			}
			prev = item;
			item = next;
		}
	}
}

/*! \brief Returns the first item in the list, or NULL if empty
*/
list_item*
List::First() const {
	return fFirst;
}

/*! \brief Returns the last item in the list, or NULL if empty
*/
list_item*
List::Last() const {
	return fLast;
}

//------------------------------------------------------------------------------
// session
//------------------------------------------------------------------------------

/*! \brief Creates an unitialized session object
*/
session::session(uint32 index, session *next = NULL)
	: list_item(index, next)
	, first_track_hint(-1)
	, last_track_hint(-1)
	, control(-1)
	, adr(-1)
	, end_lba(0)
{
}

/*! \brief Returns true if the \a first_track_hint member has not been
	set to a legal value yet.
*/
bool
session::first_track_hint_is_set()
{
	return 1 <= first_track_hint && first_track_hint <= 99;
}

/*! \brief Returns true if the \a last_track_hint member has not been
	set to a legal value yet.
*/
bool
session::last_track_hint_is_set()
{
	return 1 <= last_track_hint && last_track_hint <= 99;
}

/*! \brief Returns true if the \a end_lba member has not been
	set to a legal value yet.
	
	The result of this function also signals that the \a control
	and \a adr members have or have not been set, since they are
	set at the same time as \a end_lba.
*/
bool
session::end_lba_is_set()
{
	return end_lba > 0;
}

/*! \brief Returns true if the session is flagged as being audio.

	If the \c control value for the session has not been set, returns
	false.
*/
bool
session::is_audio()
{
	return end_lba_is_set() && !(control & kControlDataTrack);
}

//------------------------------------------------------------------------------
// Disc
//------------------------------------------------------------------------------

/*! \brief Creates a new Disc object by parsing the given table of contents
	entries and checking the resultant data structure for errors and
	warnings.
	
	If successful, subsequent calls to InitCheck() will return \c B_OK,
	elsewise they will return an error code.
*/
Disc::Disc(cdrom_full_table_of_contents_entry entries[], uint32 count)
	: fInitStatus(B_NO_INIT)
{
	TRACE(("%s: Disc::Disc(%p, %ld)\n", kModuleDebugName, entries, count));
	status_t error = ParseTableOfContents(entries, count);
	Dump();
	if (!error) {
		SortAndRemoveDuplicates();
		error = CheckForErrorsAndWarnings();
	}	
	fInitStatus = error;
				
	
	fInitStatus = B_OK;
}

/*! \brief Returns \c B_OK if the object was successfully initialized, or
	an error code if not.
*/
status_t
Disc::InitCheck()
{
	return fInitStatus;
}

/*! \brief Stores the info for the given session (using 0 based indicies) in the
	struct pointed to by \a sessionInfo.
	
	Returns \c B_ENTRY_NOT_FOUND if no such session exists.
*/
status_t
Disc::GetSessionInfo(int32 index, int32 blockSize, session_info *sessionInfo)
{
	TRACE(("%s: Disc::GetSessionInfo(%ld, %ld, %p)\n", kModuleDebugName,
	       index, blockSize, sessionInfo));
	int32 counter = -1;
	for (session *session = (struct session*)fSessionList.First();
	       session;
	         session = (struct session*)session->next)
	{
		if (session->is_audio()) {
			counter++; // only one session per audio session				
			if (counter == index) {
				// Found an audio session. Take the start of the first
				// track with the end of session.
				track *track = (struct track*)session->track_list.First();
				if (track) {
					TRACE(("%s: found session #%ld info (audio session)\n", kModuleDebugName, index));

					off_t start_lba = track->start_lba;
					off_t end_lba = session->end_lba;
					
					sessionInfo->logical_block_size = blockSize;
					sessionInfo->offset = start_lba * blockSize;
					sessionInfo->size = (end_lba - start_lba) * blockSize;
					sessionInfo->index = index;
					sessionInfo->flags = 0;
					
					return B_OK;									
				} else {
					TRACE(("%s: Disc::GetSessionInfo: session #%ld is an audio "
					       "session with no tracks\n", kModuleDebugName, index));
					return B_ENTRY_NOT_FOUND;
				}			
			}			
		} else {
			for (track *track = (struct track*)session->track_list.First();
			       track;
			         track = (struct track*)track->next)
			{
				counter++;
				if (counter == index) {
					TRACE(("%s: found session #%ld info (data session)\n", kModuleDebugName, index));

					off_t start_lba = track->start_lba;
					off_t end_lba = track->next
					                  ? ((struct track*)track->next)->start_lba
					                    : session->end_lba;
					
					sessionInfo->logical_block_size = blockSize;
					sessionInfo->offset = start_lba * blockSize;
					sessionInfo->size = (end_lba - start_lba) * blockSize;
					sessionInfo->index = index;
					sessionInfo->flags = B_DATA_SESSION;
					
					return B_OK;														
				}			
			}		
		}		
	}
	
	return B_ENTRY_NOT_FOUND;
}

/*! \brief Dumps a printout of the disc using TRACE.
*/
void
Disc::Dump()
{
	TRACE(("%s: Disc dump:\n", kModuleDebugName));
	session *session = (struct session*)fSessionList.First();
	while (session) {
		TRACE(("session %ld:\n", session->index));
		TRACE(("  first track hint: %d\n", session->first_track_hint));
		TRACE(("  last track hint:  %d\n", session->last_track_hint));
		TRACE(("  end_lba:          %lld\n", session->end_lba));
		TRACE(("  control:          %d (%s session, copy %s)\n", session->control,
		       (session->control & kControlDataTrack ? "data" : "audio"),
		       (session->control & kControlCopyPermitted ? "permitted" : "prohibited")));
		TRACE(("  adr:              %d\n", session->adr));
		track *track = (struct track*)session->track_list.First();
		while (track) {
			TRACE(("  track %ld:\n", track->index));
			TRACE(("    start_lba: %lld\n", track->start_lba));
			track = (struct track*)track->next;
		}
		session = (struct session*)session->next;
	}
}

/*! \brief Reads through the given table of contents data and creates an unsorted,
	unverified (i.e. non-error-checked) list of sessions and tracks.
*/
status_t
Disc::ParseTableOfContents(cdrom_full_table_of_contents_entry entries[], uint32 count)
{
	TRACE(("%s: Disc::ParseTableOfContents(%p, %ld)\n", kModuleDebugName, entries, count));
	for (uint32 i = 0; i < count; i++) {
		// Find or create the appropriate session
		uint8 session_index = entries[i].session;
		session *session = (struct session*)fSessionList.Find(session_index);
		if (!session) {
			session = new struct session(session_index);
			if (!session) {
				return B_NO_MEMORY;
			} else {
				fSessionList.Add(session);
			}
		}
		
		uint8 point = entries[i].point;
		
		switch (point) {
		
			// first track hint
			case 0xA0:
				if (!session->first_track_hint_is_set()) {
					int8 first_track_hint = entries[i].pminutes;
					if (1 <= first_track_hint && first_track_hint <= 99) {
						session->first_track_hint = first_track_hint;
					} else {
						WARN(("%s: warning: illegal first track hint %d found "
						      "for session %d\n", kModuleDebugName, first_track_hint,
						      session_index));
					}
				} else {
					WARN(("%s: warning: duplicated first track hint values found for"
					      "session %d; using first value encountered: %d", kModuleDebugName,
					      session_index, session->first_track_hint));		     
				}						      
				break;
				
			// last track hint
			case 0xA1:
				if (!session->last_track_hint_is_set()) {
					int8 last_track_hint = entries[i].pminutes;
					if (1 <= last_track_hint && last_track_hint <= 99) {
						session->last_track_hint = last_track_hint;
					} else {
						WARN(("%s: warning: illegal last track hint %d found "
						      "for session %d\n", kModuleDebugName, last_track_hint,
						      session_index));
					}
				} else {
					WARN(("%s: warning: duplicate last track hint values found for"
					      "session %d; using first value encountered: %d", kModuleDebugName,
					      session_index, session->last_track_hint));					     
				}						      
				break;
					
			// end of session address
			case 0xA2:
				if (!session->end_lba_is_set()) {
					off_t end_lba = msf_to_lba(make_msf_address(
								                          entries[i].pminutes,
								                          entries[i].pseconds,
								                          entries[i].pframes));
					if (end_lba > 0) {
						session->end_lba = end_lba;
						// We also grab the session's control and adr values
						// from this entry
						session->control = entries[i].control;
						session->adr = entries[i].adr;
					} else {
						WARN(("%s: warning: illegal end lba %lld found "
						      "for session %d\n", kModuleDebugName, end_lba,
						      session_index));
					}
				} else {
					WARN(("%s: warning: duplicate end lba values found for"
					      "session %d; using first value encountered: %lld",
					      kModuleDebugName, session_index, session->end_lba));					     
				}						      
				break;
				
				// Valid, but uninteresting, points
				case 0xB0:
				case 0xB1:
				case 0xB2:
				case 0xB3:
				case 0xB4:
				case 0xC0:
				case 0xC1:
					break;
				
			default:
				// Anything else had better be a valid track number,
				// or it's an invalid point
				if (1 <= point && point <= 99) {
					// Create and add the track. We'll weed out any duplicates later.
					uint8 track_index = point;
					off_t start_lba = msf_to_lba(make_msf_address(
								                          entries[i].pminutes,
								                          entries[i].pseconds,
								                          entries[i].pframes));
					// The control and adr values grabbed here are only used later on
					// to signal a warning if they don't match the corresponding values
					// of the parent session. 
					track *track = new(nothrow) struct track(track_index, start_lba,
					               entries[i].control, entries[i].adr);
					if (!track) {
						return B_NO_MEMORY;
					} else {
						session->track_list.Add(track);
					}
				} else {
					WARN(("%s: warning: illegal point 0x%2x found in table of contents\n",
					      kModuleDebugName, entries[i].point));
				}
				break;		
		}
	}
	return B_OK;
}

/*! \brief Bubble sorts the session list and each session's track lists,
	removing all but the first of any duplicates (by index) found along
	the way.
*/
void
Disc::SortAndRemoveDuplicates()
{
	fSessionList.SortAndRemoveDuplicates();
	session *session = (struct session*)fSessionList.First();
	while (session) {
		session->track_list.SortAndRemoveDuplicates();
		session = (struct session*)session->next;
	}
}

/*	\brief Checks the sessions and tracks for any anomalies.

	Errors will return an error code, warnings will return B_OK.
	Both will print a notification using TRACE.

	Anomalies that result in errors:
	- Sessions with no end_lba set
	- Sessions with no tracks
	
	Anomalies that result in warnings:
	- Inaccurate first_track_hint and/or last_track_hint values
	- Sequences of sessions or tracks that do not start at 1,
	  do not end at or before 99, or are not strictly ascending.
	  (all tracks are checked as a single sequence, since track
	  numbering does not restart with each session).
	- Tracks with different control and/or adr values than their
	  parent session
	  
	Anomalies that are currently *not* checked:
	- First Track Hint or Last Track Hint control and adr values
	  that do not match the values for their session; Ingo's copy
	  of the BeOS R5 CD is like this, but I don't believe it's
	  a matter we need to worry about. This could certainly be
	  changed in the future if needed.
*/	
status_t
Disc::CheckForErrorsAndWarnings() {
	int32 lastSessionIndex = 0;
	int32 lastTrackIndex = 0;
	
	for (session *session = (struct session*)fSessionList.First();
	       session;
	         session = (struct session*)session->next)
	{
		// Check for errors
		//-----------------
		
		// missing end lba
		if (!session->end_lba_is_set()) {
			TRACE(("%s: Disc::CheckForErrorsAndWarnings: error: no end of session "
			       "address for session #%ld\n", kModuleDebugName, session->index));
			return B_ERROR;
		}
		
		// empty track list
		track *track = (struct track*)session->track_list.First();
		if (!track) {
			TRACE(("%s: Disc::CheckForErrorsAndWarnings: error: session #%ld has no"
			       "tracks\n", kModuleDebugName, session->index));
			return B_ERROR;
		}
		
		// Check for warnings
		//-------------------
		
		// incorrect first track hint
		if (session->first_track_hint_is_set()
		    && session->first_track_hint != track->index)
		{
			TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: session #%ld: first "
			       "track hint (%d) doesn't match actual first track (%ld)\n", kModuleDebugName,
			       session->index, session->first_track_hint, track->index));
		}

		// incorrect last track hint
		struct track *last = (struct track*)session->track_list.Last();
		if (session->last_track_hint_is_set() && last
		    && session->last_track_hint != last->index)
		{
			TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: session #%ld: last "
			       "track hint (%d) doesn't match actual last track (%ld)\n", kModuleDebugName,
			       session->index, session->last_track_hint, last->index));
		}
			
		// invalid session sequence
		if (lastSessionIndex+1 != session->index) {
			TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: index for session #%ld "
			       "is out of sequence (should have been #%ld)\n", kModuleDebugName,
			       session->index, lastSessionIndex));
		}
		lastSessionIndex = session->index;
		
		for ( ; track; track = (struct track*)track->next) {
			// invalid track sequence
			if (lastTrackIndex+1 != track->index) {
				TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: index for track #%ld "
				       "is out of sequence (should have been #%ld)\n", kModuleDebugName,
				       track->index, lastTrackIndex));
			}
			lastTrackIndex = track->index;

			// mismatched control
			if (track->control != session->control) {
				TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: control for track #%ld "
				       "(%d, %s track, copy %s) does not match control for parent session #%ld "
				       "(%d, %s session, copy %s)\n", kModuleDebugName, track->index, track->control,
	       		       (track->control & kControlDataTrack ? "data" : "audio"),
		       		   (track->control & kControlCopyPermitted ? "permitted" : "prohibited"),
		       		   session->index, session->control,
	       		       (session->control & kControlDataTrack ? "data" : "audio"),
		       		   (session->control & kControlCopyPermitted ? "permitted" : "prohibited")));
			}

			// mismatched adr
			if (track->adr != session->adr) {
				TRACE(("%s: Disc::CheckForErrorsAndWarnings: warning: adr for track #%ld "
				       "(adr = %d) does not match adr for parent session #%ld (adr = %d)\n", kModuleDebugName,
				       track->index, track->adr, session->index, session->adr));
			}
		}		
	}

	return B_OK;
}

//------------------------------------------------------------------------------
// C functions
//------------------------------------------------------------------------------

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
		TRACE(("%s: Unable to get device geometry\n", kModuleDebugName));
	return result;
}

// cdrom_session_get_nth_info
/*! \todo Check that read_toc buffer was large enough on success
*/
static
status_t
cdrom_session_get_nth_info(int deviceFD, int32 index, off_t deviceSize,
                   int32 blockSize, struct session_info *sessionInfo)
{
	status_t error = sessionInfo && index >= 0 ? B_OK : B_BAD_VALUE;
	uchar data[2048];
	int32 session = index+1;
	
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
		int count;

		header = (cdrom_table_of_contents_header*)data;
		entries = (cdrom_full_table_of_contents_entry*)(data+4);
		header->length = B_BENDIAN_TO_HOST_INT16(header->length);
		
		count = (header->length-2) / sizeof(cdrom_full_table_of_contents_entry);
		
		// Check for a valid session index
		if (session < 1 || session > 99)
			error = B_ENTRY_NOT_FOUND;
		
		// Extract the data of interest
		if (!error) {
			Disc disc(entries, count);
			error = disc.InitCheck();
			if (!error) 
				error = disc.GetSessionInfo(index, blockSize, sessionInfo);
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

