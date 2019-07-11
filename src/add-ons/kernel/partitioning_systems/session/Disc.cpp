/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tyler Akidau, haiku@akidau.net
 */


/*!	\file Disc.cpp

	Disc class implementation, used to enumerate the CD/DVD sessions.

	The protocols followed in this module are based on information
	taken from the "SCSI-3 Multimedia Commands" draft, revision 10A.

	The SCSI command of interest is "READ TOC/PMA/ATIP", command
	number \c 0x43.

	The format of interest for said command is "Full TOC", format
	number \c 0x2.
*/


#include "Disc.h"

#include <DiskDeviceDefs.h>
#include <DiskDeviceTypes.h>

#include "Debug.h"


DBG(static const char* kModuleDebugName = "session");


/*! \brief An item that can be stored in a List object.
*/
struct list_item {
public:
	list_item(uint32 index, list_item* next = NULL)
		:
		index(index),
		next(next)
	{
	}

	int32		index;
	list_item*	next;
};


/*! \brief A simple, singly linked list.
*/
class List {
public:
	List();
	~List();

	list_item* Find(int32 index) const;
	void Add(list_item* item);
	void Clear();
	void SortAndRemoveDuplicates();

	list_item* First() const;
	list_item* Last() const;

private:
	list_item*	fFirst;
	list_item*	fLast;
};


/*! \brief Keeps track of track information.
*/
struct track : public list_item {
public:
	track(uint32 index, off_t startLBA, uint8 control, uint8 adr,
			track* next = NULL)
		:
		list_item(index, next),
		start_lba(startLBA),
		control(control),
		adr(adr)
	{
	}

	off_t	start_lba;
	uint8	control;
		// Used to check for Yellow/Red Book mixed-mode CDs.
	uint8	adr;
		// only used to give what are probably useless warnings
};


/*! \brief Keeps track of session information.
*/
struct session : public list_item {
public:
	session(uint32 index, session* next = NULL);

	bool first_track_hint_is_set();
	bool last_track_hint_is_set();
	bool end_lba_is_set();	// also implies control and adr are set

	bool is_audio();

	int8	first_track_hint;
	int8	last_track_hint;
	int8	control;
	int8	adr;
	off_t	end_lba;

	List	track_list;
};


//	#pragma mark - Helper functions


#ifdef DEBUG
/*
static void
dump_scsi_command(raw_device_command* cmd)
{
	int i;
	uint j;
	scsi_table_of_contents_command* scsi_command
		= (scsi_table_of_contents_command*)(&(cmd->command));

	for (i = 0; i < cmd->command_length; i++)
		TRACE(("%.2x,", cmd->command[i]));
	TRACE(("\n"));

	TRACE(("raw_device_command:\n"));
	TRACE(("  command:\n"));
	TRACE(("    command = %d (0x%.2x)\n", scsi_command->command,
		scsi_command->command));
	TRACE(("    msf     = %d\n", scsi_command->msf));
	TRACE(("    format  = %d (0x%.2x)\n", scsi_command->format,
		scsi_command->format));
	TRACE(("    number  = %d\n", scsi_command->number));
	TRACE(("    length  = %d\n",
		B_BENDIAN_TO_HOST_INT16(scsi_command->length)));
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

		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
			|| ('0' <= c && c <= '9'))
			TRACE(("\\%c,", c));
		else
			TRACE(("%.2x,", c));
	}
	TRACE(("\n"));
	TRACE(("sense_data dump:\n"));
	for (j = 0; j < cmd->sense_data_length; j++) {
		uchar c = ((uchar*)cmd->sense_data)[j];
		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
			|| ('0' <= c && c <= '9'))
			TRACE(("%c", c));
		else if (c == 0)
			TRACE(("_"));
		else
			TRACE(("-"));
	}
	TRACE(("\n"));
}
*/


static void
dump_full_table_of_contents(uchar* data, uint16 dataLength)
{
	cdrom_table_of_contents_header* header
		= (cdrom_table_of_contents_header*)data;
	cdrom_full_table_of_contents_entry* entries
		= (cdrom_full_table_of_contents_entry*)(data + 4);
	int headerLength = B_BENDIAN_TO_HOST_INT16(header->length);

	if (dataLength < headerLength) {
		TRACE(("dump_full_table_of_contents: warning, data buffer not large "
			"enough (%d < %d)\n", dataLength, headerLength));
		headerLength = dataLength;
	}

	TRACE(("%s: table of contents dump:\n", kModuleDebugName));
	TRACE(("--------------------------------------------------\n"));
	TRACE(("header:\n"));
	TRACE(("  length = %d\n", headerLength));
	TRACE(("  first  = %d\n", header->first));
	TRACE(("  last   = %d\n", header->last));

	int count = (headerLength - 2) / sizeof(cdrom_full_table_of_contents_entry);
	TRACE(("\n"));
	TRACE(("entry count = %d\n", count));

	for (int i = 0; i < count; i++) {
		TRACE(("\n"));
		TRACE(("entry #%d:\n", i));
		TRACE(("  session  = %d\n", entries[i].session));
		TRACE(("  adr      = %d\n", entries[i].adr));
		TRACE(("  control  = %d (%s track, copy %s)\n", entries[i].control,
			(entries[i].control & kControlDataTrack ? "data" : "audio"),
			(entries[i].control & kControlCopyPermitted
				? "permitted" : "prohibited")));
		TRACE(("  tno      = %d\n", entries[i].tno));
		TRACE(("  point    = %d (0x%.2x)\n", entries[i].point,
			entries[i].point));
		TRACE(("  minutes  = %d\n", entries[i].minutes));
		TRACE(("  frames   = %d\n", entries[i].seconds));
		TRACE(("  seconds  = %d\n", entries[i].frames));
		TRACE(("  zero     = %d\n", entries[i].zero));
		TRACE(("  pminutes = %d\n", entries[i].pminutes));
		TRACE(("  pseconds = %d\n", entries[i].pseconds));
		TRACE(("  pframes  = %d\n", entries[i].pframes));
		TRACE(("  lba      = %" B_PRId64 "\n",
			msf_to_lba(make_msf_address(entries[i].pminutes,
			entries[i].pseconds, entries[i].pframes))));
	}
	TRACE(("--------------------------------------------------\n"));
}
#endif	// DEBUG


static status_t
read_table_of_contents(int deviceFD, uint32 first_session, uchar* buffer,
	uint16 buffer_length, bool msf)
{
	scsi_table_of_contents_command scsi_command;
	raw_device_command raw_command;
	const uint32 sense_data_length = 1024;
	uchar sense_data[sense_data_length];
	status_t error = buffer ? B_OK : B_BAD_VALUE;

	DEBUG_INIT_ETC(NULL, ("fd: %d, buffer: %p, buffer_length: %d",
		deviceFD, buffer, buffer_length));

	if (error)
		return error;

	// This does not always work on the first try, so do it twice just in case.
	for (int attempt = 0; attempt < 2; attempt++) {
		// Init the scsi command and copy it into the "raw scsi command"
		// ioctl struct
		memset(raw_command.command, 0, 16);
		scsi_command.command = 0x43;
		scsi_command.msf = 1;
		scsi_command.format = kFullTableOfContentsFormat;
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

		if (ioctl(deviceFD, B_RAW_DEVICE_COMMAND, &raw_command,
				sizeof(raw_command)) == 0) {
			if (raw_command.scsi_status == 0 && raw_command.cam_status == 1) {
				// SUCCESS!!!
				DBG(dump_full_table_of_contents(buffer, buffer_length));
				return B_OK;
			} else {
				error = B_FILE_ERROR;
				TRACE(("%s: scsi ioctl succeeded, but scsi command failed\n",
					kModuleDebugName));
			}
		} else {
			error = errno;
			TRACE(("%s: scsi command failed with error 0x%" B_PRIx32 "\n",
				kModuleDebugName, error));
		}
	}

	return error;
}


//	#pragma mark - List
// TODO: get rid of this, and use the standard DoublyLinkedList


/*! \brief Creates an empty list.
*/
List::List()
	:
	fFirst(NULL),
	fLast(NULL)
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
	list_item* item = fFirst;
	while (item && item->index != index) {
		item = item->next;
	}
	return item;
}


/*! \brief Adds the given item to the end of the list.

	\param item The item to add (may not be NULL)
*/
void
List::Add(list_item* item)
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
	list_item* item = fFirst;
	while (item) {
		list_item* next = item->next;
		delete item;
		item = next;
	}
	fFirst = fLast = NULL;
}


/*! \brief Bubble sorts the list by index, removing any duplicates
	(the first instance is kept).

	\todo I believe duplicate removal is actually unnecessary, but
		I need to verify that.
*/
void
List::SortAndRemoveDuplicates()
{
	bool sorted = false;
	while (!sorted) {
		sorted = true;

		list_item* prev = NULL;
		list_item* item = fFirst;
		list_item* next = NULL;
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
				TRACE(("%s: List::SortAndRemoveDuplicates: duplicate indicies "
					"found (#%" B_PRId32 "); keeping first instance\n",
					kModuleDebugName, item->index));
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
List::First() const
{
	return fFirst;
}


/*! \brief Returns the last item in the list, or NULL if empty
*/
list_item*
List::Last() const
{
	return fLast;
}


//	#pragma mark - session


/*! \brief Creates an unitialized session object
*/
session::session(uint32 index, session* next)
	:
	list_item(index, next),
	first_track_hint(-1),
	last_track_hint(-1),
	control(-1),
	adr(-1),
	end_lba(0)
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


//	#pragma mark - Disc


/*! \brief Creates a new Disc object by parsing the given table of contents
	entries and checking the resultant data structure for errors and
	warnings.

	If successful, subsequent calls to InitCheck() will return \c B_OK,
	elsewise they will return an error code.
*/
Disc::Disc(int fd)
	:
	fInitStatus(B_NO_INIT),
	fSessionList(new List)
{
	DEBUG_INIT_ETC("Disc", ("fd: %d", fd));

	uchar data[kBlockSize];
/*
	if (!error)
		error = sessionInfo && index >= 0 ? B_OK : B_BAD_VALUE;
	int32 session = index+1;
		// Check for a valid session index
		if (session < 1 || session > 99)
			error = B_ENTRY_NOT_FOUND;
*/

	status_t error = fSessionList ? B_OK : B_NO_MEMORY;

	// Attempt to read the table of contents, first in lba mode, then in msf
	// mode
	if (!error)
		error = read_table_of_contents(fd, 1, data, kBlockSize, false);
	if (error) {
		TRACE(("%s: lba read_toc failed, trying msf instead\n",
			kModuleDebugName));
		error = read_table_of_contents(fd, 1, data, kBlockSize, true);
	}

	// Interpret the data returned, if successful
	if (!error) {
		cdrom_table_of_contents_header* header;
		cdrom_full_table_of_contents_entry* entries;
		int count;

		header = (cdrom_table_of_contents_header*)data;
		entries = (cdrom_full_table_of_contents_entry*)(data + 4);
		header->length = B_BENDIAN_TO_HOST_INT16(header->length);

		count = (header->length - 2)
			/ sizeof(cdrom_full_table_of_contents_entry);

		count = _AdjustForYellowBook(entries, count);
		error = _ParseTableOfContents(entries, count);
//		Dump();
		if (!error) {
			_SortAndRemoveDuplicates();
			error = _CheckForErrorsAndWarnings();
		}
	}

	PRINT(("Setting init status to 0x%" B_PRIx32 ", `%s'\n", error,
		strerror(error)));
	fInitStatus = error;
}


/*! \brief Destroys the Disc's internal list.
*/
Disc::~Disc()
{
	delete fSessionList;
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
Session*
Disc::GetSession(int32 index)
{
	DEBUG_INIT_ETC("Disc", ("index: %" B_PRId32, index));
	int32 counter = -1;
	for (session* session = (struct session*)fSessionList->First(); session;
			session = (struct session*)session->next) {
		if (session->is_audio()) {
			counter++;
				// only one session per audio session
			if (counter == index) {
				// Found an audio session. Take the start of the first
				// track with the end of session.
				track* track = (struct track*)session->track_list.First();
				if (track != NULL) {
					PRINT(("found session #%" B_PRId32 " info (audio session)"
							"\n", index));

					off_t startLBA = track->start_lba;
					off_t endLBA = session->end_lba;

					off_t offset = startLBA * kBlockSize;
					off_t size = (endLBA - startLBA) * kBlockSize;

					Session* result = new Session(offset, size, kBlockSize,
						index, B_PARTITION_READ_ONLY,
						kPartitionTypeAudioSession);
					if (result == NULL) {
						PRINT(("Error allocating new Session object; out of "
							"memory!\n"));
					}
					return result;
				} else {
					PRINT(("Error: session #%" B_PRId32 " is an audio session "
						"with no tracks!\n", index));
					return NULL;
				}
			}
		} else {
			for (track* track = (struct track*)session->track_list.First();
					track; track = (struct track*)track->next) {
				counter++;
				if (counter == index) {
					PRINT(("found session #%" B_PRId32 " info (data session)\n",
						index));

					off_t startLBA = track->start_lba;
					if (startLBA < 0) {
						WARN(("%s: warning: invalid negative start LBA of %"
							B_PRId64 " for data track assuming 0\n",
							kModuleDebugName, startLBA));
						startLBA = 0;
					}

					off_t endLBA = track->next
						? ((struct track*)track->next)->start_lba
						: session->end_lba;

					off_t offset = startLBA * kBlockSize;
					off_t size = (endLBA - startLBA) * kBlockSize;

					Session* result = new Session(offset, size, kBlockSize,
						index, B_PARTITION_READ_ONLY,
						kPartitionTypeDataSession);
					if (result == NULL) {
						PRINT(("Error allocating new Session object; out of "
							"memory!\n"));
					}
					return result;
				}
			}
		}
	}

	PRINT(("no session #%" B_PRId32 " found!\n", index));
	return NULL;
}


/*! \brief Dumps a printout of the disc using TRACE.
*/
void
Disc::Dump()
{
	TRACE(("%s: Disc dump:\n", kModuleDebugName));
	session* session = (struct session*)fSessionList->First();
	while (session != NULL) {
		TRACE(("session %" B_PRId32 ":\n", session->index));
		TRACE(("  first track hint: %d\n", session->first_track_hint));
		TRACE(("  last track hint:  %d\n", session->last_track_hint));
		TRACE(("  end_lba:          %" B_PRId64 "\n", session->end_lba));
		TRACE(("  control:          %d (%s session, copy %s)\n",
			session->control, (session->control & kControlDataTrack
				? "data" : "audio"),
			(session->control & kControlCopyPermitted
				? "permitted" : "prohibited")));
		TRACE(("  adr:              %d\n", session->adr));
		track* track = (struct track*)session->track_list.First();
		while (track != NULL) {
			TRACE(("  track %" B_PRId32 ":\n", track->index));
			TRACE(("    start_lba: %" B_PRId64 "\n", track->start_lba));
			track = (struct track*)track->next;
		}
		session = (struct session*)session->next;
	}
}


/*! \brief Checks for Yellow Book data tracks in audio sessions and if found
	inserts them as a new data session.
*/
uint32
Disc::_AdjustForYellowBook(cdrom_full_table_of_contents_entry entries[],
	uint32 count)
{
	uint8 foundCount = 0;
	uint8 endLBAEntry = 0;
	uint8 trackTwo = 0;

	// Make sure TOC has only one session and that it is audio.
	bool sessionIsAudio = true;
	for (uint32 i = 0; i < count; i++) {
		if (entries[i].point == 0xa2) {
			if ((entries[i].control & kControlDataTrack) != 0) {
				sessionIsAudio = false;
				break;
			}
			foundCount++;
			endLBAEntry = i;
		}
	}
	if (!sessionIsAudio || foundCount != 1)
		return count;

	TRACE(("%s: Single audio session, checking for data track\n",
		kModuleDebugName));

	// See if there are any data tracks.
	for (uint32 i = 0; i < count; i++) {
		if (entries[i].point > 0 && entries[i].point < 100
			&& (entries[i].control & kControlDataTrack) != 0) {
			if (entries[i].point == 1) {
				// Create a new endLBA point for session one.
				entries[count] = entries[endLBAEntry];
				entries[count].control = entries[i].control;

				// Get track two and use it's start as
				// the end of our new session.
				for (uint8 j = 0; j < count; j++) {
					if (entries[j].point == 2) {
						trackTwo = j;
						break;
					}
				}
				entries[count].pminutes = entries[trackTwo].pminutes;
				entries[count].pseconds = entries[trackTwo].pseconds;
				entries[count].pframes = entries[trackTwo].pframes;

				// Change the other points to session two.
				for (uint32 j = 0; j < count; j++) {
					entries[j].session = 2;
				}
				entries[i].session = 1;

				count++;
				TRACE(("%s: first track is data, adjusted TOC\n",
					kModuleDebugName));
				break;
			} else {
				// Change the track to session two.
				entries[i].session = 2;

				// Create a new endLBA point for session two.
				entries[count] = entries[endLBAEntry];
				entries[count].session = 2;
				entries[count].control = entries[i].control;

				// Use the beginning of the data track as the
				// end of the previous session.
				entries[endLBAEntry].pminutes = entries[i].pminutes;
				entries[endLBAEntry].pseconds = entries[i].pseconds;
				entries[endLBAEntry].pframes = entries[i].pframes;

				count++;
				TRACE(("%s: last track is data, adjusted TOC\n",
					kModuleDebugName));
				break;
			}
		}
	}
	return count;
}


/*! \brief Reads through the given table of contents data and creates an
	unsorted, unverified (i.e. non-error-checked) list of sessions and tracks.
*/
status_t
Disc::_ParseTableOfContents(cdrom_full_table_of_contents_entry entries[],
	uint32 count)
{
	DEBUG_INIT_ETC("Disc", ("entries: %p, count: %" B_PRIu32, entries, count));

	for (uint32 i = 0; i < count; i++) {
		// Find or create the appropriate session
		uint8 sessionIndex = entries[i].session;
		session* session = (struct session*)fSessionList->Find(sessionIndex);
		if (session == NULL) {
			session = new struct session(sessionIndex);
			if (session == NULL)
				return B_NO_MEMORY;

			fSessionList->Add(session);
		}

		uint8 point = entries[i].point;

		switch (point) {
			// first track hint
			case 0xA0:
				if (!session->first_track_hint_is_set()) {
					int8 firstTrackHint = entries[i].pminutes;
					if (1 <= firstTrackHint && firstTrackHint <= 99) {
						session->first_track_hint = firstTrackHint;
					} else {
						WARN(("%s: warning: illegal first track hint %d found "
							"for session %d\n", kModuleDebugName,
							firstTrackHint, sessionIndex));
					}
				} else {
					WARN(("%s: warning: duplicated first track hint values "
						"found for session %d; using first value "
						"encountered: %d", kModuleDebugName, sessionIndex,
						session->first_track_hint));
				}
				break;

			// last track hint
			case 0xA1:
				if (!session->last_track_hint_is_set()) {
					int8 lastTrackHint = entries[i].pminutes;
					if (1 <= lastTrackHint && lastTrackHint <= 99) {
						session->last_track_hint = lastTrackHint;
					} else {
						WARN(("%s: warning: illegal last track hint %d found "
							"for session %d\n", kModuleDebugName,
							lastTrackHint, sessionIndex));
					}
				} else {
					WARN(("%s: warning: duplicate last track hint values found "
						"for session %d; using first value encountered: %d",
						kModuleDebugName, sessionIndex,
						session->last_track_hint));
				}
				break;

			// end of session address
			case 0xA2:
				if (!session->end_lba_is_set()) {
					off_t endLBA = msf_to_lba(make_msf_address(
						entries[i].pminutes, entries[i].pseconds,
						entries[i].pframes));
					if (endLBA > 0) {
						session->end_lba = endLBA;
						// We also grab the session's control and adr values
						// from this entry
						session->control = entries[i].control;
						session->adr = entries[i].adr;
					} else {
						WARN(("%s: warning: illegal end lba %" B_PRId64 " found"
							" for session %d\n", kModuleDebugName, endLBA,
							sessionIndex));
					}
				} else {
					WARN(("%s: warning: duplicate end lba values found for "
						"session %d; using first value encountered: %" B_PRId64,
						kModuleDebugName, sessionIndex, session->end_lba));
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
					// Create and add the track. We'll weed out any duplicates
					// later.
					uint8 trackIndex = point;
					off_t startLBA = msf_to_lba(make_msf_address(
						entries[i].pminutes, entries[i].pseconds,
						entries[i].pframes));
					// The control and adr values grabbed here are only used
					// later on to signal a warning if they don't match the
					// corresponding values of the parent session.
					track* track = new(std::nothrow) struct track(trackIndex,
						startLBA, entries[i].control, entries[i].adr);
					if (track == NULL)
						return B_NO_MEMORY;

					session->track_list.Add(track);
				} else {
					WARN(("%s: warning: illegal point 0x%2x found in table of "
						"contents\n", kModuleDebugName, point));
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
Disc::_SortAndRemoveDuplicates()
{
	fSessionList->SortAndRemoveDuplicates();
	session* session = (struct session*)fSessionList->First();
	while (session != NULL) {
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
Disc::_CheckForErrorsAndWarnings() {
	int32 lastSessionIndex = 0;
	int32 lastTrackIndex = 0;

	for (session* session = (struct session*)fSessionList->First(); session;
			session = (struct session*)session->next) {
		// Check for errors

		// missing end lba
		if (!session->end_lba_is_set()) {
			TRACE(("%s: Disc::_CheckForErrorsAndWarnings: error: no end of "
				"session address for session #%" B_PRId32 "\n",
				kModuleDebugName, session->index));
			return B_ERROR;
		}

		// empty track list
		track* track = (struct track*)session->track_list.First();
		if (track == NULL) {
			TRACE(("%s: Disc::_CheckForErrorsAndWarnings: error: session #%"
				B_PRId32 "has no tracks\n", kModuleDebugName, session->index));
			return B_ERROR;
		}

		// Check for warnings

		// incorrect first track hint
		if (session->first_track_hint_is_set()
			&& session->first_track_hint != track->index) {
			TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: session "
				"#%" B_PRId32 ": first track hint (%d) doesn't match actual "
				"first track (%" B_PRId32 ")\n", kModuleDebugName,
				session->index, session->first_track_hint, track->index));
		}

		// incorrect last track hint
		struct track* last = (struct track*)session->track_list.Last();
		if (session->last_track_hint_is_set() && last
			&& session->last_track_hint != last->index) {
			TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: session "
				"#%" B_PRId32 ": last track hint (%d) doesn't match actual "
				"last track (%" B_PRId32 ")\n", kModuleDebugName,
				session->index, session->last_track_hint, last->index));
		}

		// invalid session sequence
		if (lastSessionIndex + 1 != session->index) {
			TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: index for "
				"session #%" B_PRId32 " is out of sequence (should have been #%"
				B_PRId32 ")\n",	kModuleDebugName, session->index,
				lastSessionIndex));
		}
		lastSessionIndex = session->index;

		for (; track; track = (struct track*)track->next) {
			// invalid track sequence
			if (lastTrackIndex + 1 != track->index) {
				TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: index "
					"for track #%" B_PRId32 " is out of sequence (should have "
					"been #%" B_PRId32 ")\n", kModuleDebugName, track->index,
					lastTrackIndex));
			}
			lastTrackIndex = track->index;

			// mismatched control
			if (track->control != session->control) {
				TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: control "
					"for track #%" B_PRId32 " (%d, %s track, copy %s) does not "
					"match control for parent session #%" B_PRId32 " (%d, %s "
					"session, copy %s)\n", kModuleDebugName, track->index,
					track->control,
					(track->control & kControlDataTrack ? "data" : "audio"),
					(track->control & kControlCopyPermitted
						? "permitted" : "prohibited"),
					session->index, session->control,
					(session->control & kControlDataTrack ? "data" : "audio"),
					(session->control & kControlCopyPermitted
						? "permitted" : "prohibited")));
			}

			// mismatched adr
			if (track->adr != session->adr) {
				TRACE(("%s: Disc::_CheckForErrorsAndWarnings: warning: adr "
					"for track #%" B_PRId32 " (adr = %d) does not match adr "
					"for parent session #%" B_PRId32 " (adr = %d)\n",
					kModuleDebugName, track->index, track->adr, session->index,
					session->adr));
			}
		}
	}

	return B_OK;
}


//	#pragma mark - Session


Session::Session(off_t offset, off_t size, uint32 blockSize, int32 index,
		uint32 flags, const char* type)
	:
	fOffset(offset),
	fSize(size),
	fBlockSize(blockSize),
	fIndex(index),
	fFlags(flags),
	fType(strdup(type))
{
}


Session::~Session()
{
	free(fType);
}
