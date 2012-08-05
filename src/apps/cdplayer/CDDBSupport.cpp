// Copyright 1992-2000, Be Incorporated, All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
//
// send comments/suggestions/feedback to pavel@be.com
//

#include "CDDBSupport.h"

#include <Alert.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <NetAddress.h>
#include <Path.h>
#include <Query.h>
#include <UTF8.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>


//#define DEBUG_CDDB

#ifdef DEBUG_CDDB
#define STRACE(x) printf x
#else
#define STRACE(x) /* nothing */
#endif

const int kTerminatingSignal = SIGINT; // SIGCONT;

// The SCSI table of contents consists of a 4-byte header followed by 100 track
// descriptors, which are each 8 bytes. We don't really need the first 5 bytes
// of the track descriptor, so we'll just ignore them. All we really want is the
// length of each track, which happen to be the last 3 bytes of the descriptor.
struct TrackRecord {
	int8	unused[5];

	int8	min;
	int8	sec;
	int8	frame;
};


static void
DoNothing(int)
{
}


static int32
cddb_sum(int n)
{
	char buf[12];
	int32 ret = 0;

	sprintf(buf, "%u", n);
	for (const char *p = buf; *p != '\0'; p++)
		ret += (*p - '0');
	return ret;
}


BString
GetLineFromString(const char *string)
{
	if (!string)
		return NULL;

	BString out(string);

	int32 lineFeed = out.FindFirst("\n");

	if (lineFeed > 0)
		out.Truncate(lineFeed);

	return out;
}


//	#pragma mark -


CDDBData::CDDBData(int32 discId)
	:
	fDiscID(discId),
	fYear(-1)
{
}


CDDBData::CDDBData(const CDDBData &from)
	:
	fDiscID(from.fDiscID),
 	fArtist(from.fArtist),
 	fAlbum(from.fAlbum),
 	fGenre(from.fGenre),
 	fDiscTime(from.fDiscTime),
	fYear(from.fYear)
{
	STRACE(("CDDBData::Copy Constructor\n"));

	for (int32 i = 0; i < from.fTrackList.CountItems(); i++) {
		BString *string = (BString*)from.fTrackList.ItemAt(i);
		CDAudioTime *time = (CDAudioTime*)from.fTimeList.ItemAt(i);

		if (!string || !time)
			continue;

		AddTrack(string->String(), *time);
	}
}


CDDBData::~CDDBData()
{
	STRACE(("CDDBData::~CDDBData\n"));
	_EmptyLists();
}


CDDBData &
CDDBData::operator=(const CDDBData &from)
{
	_EmptyLists();
	fDiscID = from.fDiscID;
 	fArtist = from.fArtist;
 	fAlbum = from.fAlbum;
 	fGenre = from.fGenre;
 	fDiscTime = from.fDiscTime;
	fYear = from.fYear;

	for (int32 i = 0; i < from.fTrackList.CountItems(); i++) {
		BString *string = (BString*)from.fTrackList.ItemAt(i);
		CDAudioTime *time = (CDAudioTime*)from.fTimeList.ItemAt(i);

		if (!string || !time)
			continue;

		AddTrack(string->String(),*time);
	}
	return *this;
}


void
CDDBData::MakeEmpty()
{
	STRACE(("CDDBData::MakeEmpty\n"));

	_EmptyLists();
	fDiscID = -1;
	fArtist = "";
	fAlbum = "";
	fGenre = "";
	fDiscTime.SetMinutes(-1);
	fDiscTime.SetSeconds(-1);
	fYear = -1;
}


void
CDDBData::_EmptyLists()
{
	STRACE(("CDDBData::_EmptyLists\n"));
	
	for (int32 i = 0; i < fTrackList.CountItems(); i++) {
		BString *string = (BString*)fTrackList.ItemAt(i);
		delete string;
	}
	fTrackList.MakeEmpty();

	for (int32 j = 0; j < fTimeList.CountItems(); j++) {
		CDAudioTime *time = (CDAudioTime*)fTimeList.ItemAt(j);
		delete time;
	}
	fTimeList.MakeEmpty();
}


status_t
CDDBData::Load(const entry_ref &ref)
{
	STRACE(("CDDBData::Load(%s)\n", ref.name));

	BFile file(&ref, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		STRACE(("CDDBData::Load failed\n"));
		return file.InitCheck();
	}

	attr_info info;

	if (file.GetAttrInfo("Audio:Genre", &info) == B_OK && info.size > 0) {
		char genreData[info.size + 2];

		if (file.ReadAttr("Audio:Genre", B_STRING_TYPE, 0, genreData, info.size) > 0)
			fGenre = genreData;
	}

	if (file.GetAttrInfo("Audio:Year", &info) == B_OK && info.size > 0) {
		int32 yearData;

		if (file.ReadAttr("Audio:Year", B_INT32_TYPE, 0, &yearData, info.size) > 0)
			fYear = yearData;
	}

	// TODO: Attempt reading the file before attempting to read the attributes
	if (file.GetAttrInfo("CD:tracks", &info) == B_OK && info.size > 0) {
		char trackData[info.size + 2];

		if (file.ReadAttr("CD:tracks", B_STRING_TYPE, 0, trackData, info.size) > 0) {
			trackData[info.size] = 0;
			BString tmp = GetLineFromString(trackData);
			char *index;

			if (fTrackList.CountItems() > 0)
				_EmptyLists();

			fArtist = tmp;
			fArtist.Truncate(fArtist.FindFirst(" - "));
			STRACE(("CDDBData::Load: Artist set to %s\n", fArtist.String()));

			fAlbum = tmp.String() + (tmp.FindFirst(" - ") + 3);
			STRACE(("CDDBData::Load: Album set to %s\n", fAlbum.String()));

			index = strchr(trackData, '\n') + 1;
			while (*index) {
				tmp = GetLineFromString(index);

				if (tmp.CountChars() > 0) {
					BString *newTrack = new BString(tmp);
					CDAudioTime *time = new CDAudioTime;
					time->SetMinutes(0);
					time->SetSeconds(0);

					STRACE(("CDDBData::Load: Adding Track %s (%ld:%ld)\n", newTrack->String(),
							time->minutes, time->seconds));

					fTrackList.AddItem(newTrack);
					fTimeList.AddItem(time);
				}

				index = strchr(index,'\n') + 1;
			}

			// We return this so that the caller knows to initialize tracktimes
			return B_NO_INIT;
		}
	}

	return B_ERROR;
}


status_t
CDDBData::Load()
{
	// This uses the default R5 path

	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

	path.Append("cd");
	create_directory(path.Path(), 0755);

	BString filename(path.Path());
	filename << "/" << Artist() << " - " << Album();

	if (filename.Compare("Artist") == 0)
		filename << "." << DiscID();

	BEntry entry(filename.String());
	if (entry.InitCheck() != B_OK)
		return entry.InitCheck();

	entry_ref ref;
	entry.GetRef(&ref);

	return Load(ref);
}


status_t
CDDBData::Save()
{
	// This uses the default R5 path

	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

	path.Append("cd");
	create_directory(path.Path(), 0755);

	BString filename(path.Path());
	filename << "/" << Artist() << " - " << Album();

	if (filename.Compare("Artist")==0)
		filename << "." << DiscID();

	return Save(filename.String());
}


status_t
CDDBData::Save(const char *filename)
{
	if (!filename) {
		STRACE(("CDDBData::Save failed - NULL filename\n"));
		return B_ERROR;
	}

	BFile file(filename, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK) {
		STRACE(("CDDBData::Save failed - couldn't create file %s\n", filename));
		return file.InitCheck();
	}

	BString entry;
	char timestring[10];

	sprintf(timestring,"%.2ld:%.2ld", fDiscTime.GetMinutes(), fDiscTime.GetSeconds());

	entry << fArtist << " - " << fAlbum << "\t" << timestring << "\n";
	file.Write(entry.String(), entry.Length());

	STRACE(("CDDBData::Save: wrote first line: %s", entry.String()));

	BString tracksattr(fArtist);
	tracksattr << " - " << fAlbum << "\n";

	for (int32 i = 0; i < fTrackList.CountItems(); i++) {
		BString *trackstr = (BString *)fTrackList.ItemAt(i);
		CDAudioTime *time= (CDAudioTime*)fTimeList.ItemAt(i);

		if (!trackstr || !time)
			continue;

		entry = *trackstr;

		sprintf(timestring,"%.2ld:%.2ld", time->GetMinutes(), time->GetSeconds());

		entry << "\t" << timestring << "\n";
		file.Write(entry.String(), entry.Length());
		STRACE(("CDDBData::Save: Wrote line: %s", entry.String()));
		
		tracksattr << *trackstr << "\n";
	}

	file.WriteAttr("CD:key", B_INT32_TYPE, 0, &fDiscID, sizeof(int32));
	STRACE(("CDDBData::Save: Wrote CD identifier: %ld(%lx)\n", fDiscID, fDiscID));
	file.WriteAttr("CD:tracks", B_STRING_TYPE, 0, tracksattr.String(), tracksattr.Length() + 1);

	if (fGenre.CountChars() > 0)
		file.WriteAttr("Audio:Genre",B_STRING_TYPE, 0, fGenre.String(), fGenre.Length() + 1);

	if (fYear > 0)
		file.WriteAttr("Audio:Year", B_INT32_TYPE, 0, &fYear, sizeof(int32));

	return B_OK;
}


uint16
CDDBData::CountTracks() const
{
	return fTrackList.CountItems();
}


bool
CDDBData::RenameTrack(const int32 &index, const char *newName)
{
	if (!newName) {
		STRACE(("CDDBData::RenameTrack failed - NULL newName\n"));
		return false;
	}

	BString *name = (BString*)fTrackList.ItemAt(index);
	if (name) {
		STRACE(("CDDBData::RenameTrack(%ld,%s)\n", index, newName));
		name->SetTo(newName);
		return true;
	}

	STRACE(("CDDBData::RenameTrack failed - invalid index\n"));
	return false;
}


void
CDDBData::AddTrack(const char *track, const CDAudioTime &time,
	const int16 &index)
{
	if (!track) {
		STRACE(("CDDBData::AddTrack failed - NULL name\n"));
		return;
	}
	STRACE(("CDDBData::AddTrack(%s, %ld:%.2ld,%d)\n", track, time.minutes, time.seconds, index));

	fTrackList.AddItem(new BString(track));
	fTimeList.AddItem(new CDAudioTime(time));
}


void
CDDBData::RemoveTrack(const int16 &index)
{
	// This breaks the general following of the BList style because
	// removing a track would require returning 2 pointers

	fTrackList.RemoveItem(index);
	fTimeList.RemoveItem(index);

	STRACE(("CDDBData::RemoveTrack(%d)\n", index));
}


const char *
CDDBData::TrackAt(const int16 &index) const
{
	BString *track = (BString*)fTrackList.ItemAt(index);
	if (!track)
		return NULL;

	return track->String();
}


CDAudioTime *
CDDBData::TrackTimeAt(const int16 &index)
{
	return (CDAudioTime *)fTimeList.ItemAt(index);
}


//	#pragma mark -


CDDBQuery::CDDBQuery(const char *server, int32 port)
	:
	fServerName(server),
	fPort(port),
	fConnected(false),
	fState(kInitial)
{
}


CDDBQuery::~CDDBQuery()
{
	kill_thread(fThread);
}


void 
CDDBQuery::SetToSite(const char *server, int32 port)
{
	_Disconnect();
	fServerName = server;
	fPort = port;
	fState = kInitial;
}


void 
CDDBQuery::SetToCD(const char *path)
{
	if (!path)
		return;

	// Get the SCSI table of contents from the device passed to us
	int device = open(path, O_RDONLY);
	if (device < 0)
		return;

	status_t result = ioctl(device, B_SCSI_GET_TOC, &fSCSIData);

	close(device);

	if (result != B_OK)
		return;

	// Calculate the disc's CDDB ID
	if (fState == kInitial) {
		fCDData.SetDiscID(GetDiscID(&fSCSIData));
		fCDData.SetDiscTime(GetDiscTime(&fSCSIData));
	} else {
		int32 discID = GetDiscID(&fSCSIData);

		if (fCDData.DiscID() == discID)
			return;

		fCDData.SetDiscID(discID);
		fCDData.SetDiscTime(GetDiscTime(&fSCSIData));
		fFrameOffsetString = OffsetsToString(&fSCSIData);
	}

	result = B_OK;
	fState = kReading;
	fThread = spawn_thread(&CDDBQuery::_QueryThread, "CDDBLookup", B_NORMAL_PRIORITY, this);

	if (fThread >= 0)
		resume_thread(fThread);
	else {
		fState = kError;
		result = fThread;
	}
}


const char * 
CDDBQuery::_GetToken(const char *stream, BString &result)
{
	result = "";
	while (*stream && *stream <= ' ')
		stream++;
	while (*stream && *stream > ' ')
		result += *stream++;

	return stream;
}


status_t
CDDBQuery::GetSites(bool (*eachFunc)(const char *site, int port, const char *latitude,
		const char *longitude, const char *description, void *state), void *passThru)
{
	if (!_IsConnected())
		_Connect();

	BString tmp;

	tmp = "sites\n";
	STRACE((">%s", tmp.String()));

	if (fSocket.Send(tmp.String(), tmp.Length()) == -1)
		return B_ERROR;

	_ReadLine(tmp);

	if (tmp.FindFirst("210") == -1) {
		_Disconnect();
		return B_ERROR;
	}

	for (;;) {
		BString site;
		int32 sitePort;
		BString latitude;
		BString longitude;
		BString description;

		_ReadLine(tmp);
		if (tmp == ".")
			break;
		const char *scanner = tmp.String();

		scanner = _GetToken(scanner, site);
		BString portString;
		scanner = _GetToken(scanner, portString);
		sitePort = atoi(portString.String());
		scanner = _GetToken(scanner, latitude);
		scanner = _GetToken(scanner, longitude);
		description = scanner;

		if (eachFunc(site.String(), sitePort, latitude.String(),
				longitude.String(),	description.String(), passThru))
			break;
	}
	_Disconnect();
	return B_OK;
}


bool 
CDDBQuery::GetData(CDDBData *data, bigtime_t timeout)
{
	if (!data)
		return false;

	bigtime_t deadline = system_time() + timeout;
	while (fState == kReading) {
		snooze(50000);
		if (system_time() > deadline)
			break;
	}
	if (fState != kDone)
		return false;

	*data = fCDData;

	return true;
}


int32 
CDDBQuery::GetDiscID(const scsi_toc *toc)
{
	// The SCSI TOC data has a 4-byte header and then each track record starts
	TrackRecord *tocData = (TrackRecord*)&(toc->toc_data[4]);

	int32 numTracks = toc->toc_data[3] - toc->toc_data[2] + 1;

	int32 sum1 = 0;
	int32 sum2 = 0;
	for (int index = 0; index < numTracks; index++) {
		sum1 += cddb_sum((tocData[index].min * 60) + tocData[index].sec);

		// the following is probably running over too far
		sum2 +=	(tocData[index + 1].min * 60 + tocData[index + 1].sec) 
				- (tocData[index].min * 60 + tocData[index].sec);
	}
	int32 discID = ((sum1 % 0xff) << 24) + (sum2 << 8) + numTracks;

	return discID;
}


BString
CDDBQuery::OffsetsToString(const scsi_toc *toc)
{
	TrackRecord *tocData = (TrackRecord*)&(toc->toc_data[4]);

	int32 numTracks = toc->toc_data[3] - toc->toc_data[2] + 1;

	BString string;
	for (int index = 0; index < numTracks; index++) {
		string << tocData[index].min * 4500 + tocData[index].sec * 75
			+ tocData[index].frame << ' ';
	}

	return string;
}


int32
CDDBQuery::GetTrackCount(const scsi_toc *toc)
{
	return toc->toc_data[3] - toc->toc_data[2] + 1;
}


CDAudioTime
CDDBQuery::GetDiscTime(const scsi_toc *toc)
{
	int16 trackcount = toc->toc_data[3] - toc->toc_data[2] + 1;
	TrackRecord *desc = (TrackRecord*)&(toc->toc_data[4]);

	CDAudioTime disc;
	disc.SetMinutes(desc[trackcount].min);
	disc.SetSeconds(desc[trackcount].sec);
	
	return disc;
}


void
CDDBQuery::GetTrackTimes(const scsi_toc *toc, vector<CDAudioTime> &times)
{
	TrackRecord *tocData = (TrackRecord*)&(toc->toc_data[4]);
	int16 trackCount = toc->toc_data[3] - toc->toc_data[2] + 1;

	for (int index = 0; index < trackCount + 1; index++) {
		CDAudioTime cdtime;
		cdtime.SetMinutes(tocData[index].min);
		cdtime.SetSeconds(tocData[index].sec);
		times.push_back(cdtime);
	}
}


status_t
CDDBQuery::_ReadFromServer(BString &data)
{
	// This function queries the given CDDB server for the existence of the
	// disc's data and saves the data to file once obtained.

	// Query for the existence of the disc in the database
	char idString[10];
	sprintf(idString, "%08lx", fCDData.DiscID());
	BString query;

	int32 trackCount = GetTrackCount(&fSCSIData);
	BString offsetString = OffsetsToString(&fSCSIData);	
	int32 discLength = (fCDData.DiscTime()->GetMinutes() * 60)
		+ fCDData.DiscTime()->GetSeconds();

	query << "cddb query " << idString << ' ' << trackCount << ' ' 
			<< offsetString << ' ' << discLength << '\n';

	STRACE((">%s", query.String()));

	if (fSocket.Send(query.String(), query.Length()) == -1) {
		_Disconnect();
		return B_ERROR;
	}

	BString tmp;
	_ReadLine(tmp);
	STRACE(("<%s", tmp.String()));

	BString category;
	BString queryDiscID(idString);

	if (tmp.FindFirst("200") != 0) {
		if (tmp.FindFirst("211") == 0) {
			// A 211 means that the query was not exact. To make sure that we
			// don't have a problem with this in the future, we will choose the
			// first entry that the server returns. This may or may not be wise,
			// but in my experience, the first one has been the right one.

			_ReadLine(tmp);

			// Get the category from the what the server returned
			_GetToken(tmp.String(), category);

			// Now we will get the disc ID for the CD. We will need this when we
			// query for the track name list. If we send the track name query
			// with the real discID, nothing will be returned. However, if we
			// send the one from the entry, we'll get the names and we can take
			// these names attach them to the disc that we have.
			_GetToken(tmp.String() + category.CountChars(), queryDiscID);

			// This is to suck up any more search results that the server sends
			// us.
			BString throwaway;
			_ReadLine(throwaway);
			while (throwaway.ByteAt(0) != '.')
				_ReadLine(throwaway);
		} else {
			// We get here for any time the CDDB server does not recognize the
			// CD, amongst other things
			STRACE(("CDDB lookup error: %s\n", tmp.String()));
			fCDData.SetGenre("misc");
			return B_NAME_NOT_FOUND;
		}
	} else {
		_GetToken(tmp.String() + 3, category);
	}
	STRACE(("CDDBQuery::_ReadFromServer: Genre: %s\n", category.String()));
	if (!category.Length()) {
		STRACE(("Set genre to 'misc'\n"));
		category = "misc";
	}

	// Query for the disc's data - artist, album, tracks, etc.
	query = "";
	query << "cddb read " << category << ' ' << queryDiscID << '\n' ;
	if (fSocket.Send(query.String(), query.Length()) == -1) {
		_Disconnect();
		return B_ERROR;
	}

	while (true) {
		_ReadLine(tmp);

		if (tmp == "." || tmp == ".\n")
			break;

		data << tmp << '\n';
	}

	return B_OK;
}


status_t
CDDBQuery::_Connect()
{
	if (fConnected)
		_Disconnect();

	BNetAddress address;

	status_t status = address.SetTo(fServerName.String(), fPort);
	if (status != B_OK)
		return status;

	status = fSocket.Connect(address);
	if (status != B_OK)
		return status;

	fConnected = true;

	BString tmp;
	_ReadLine(tmp);
	_IdentifySelf();

	return B_OK;
}


bool 
CDDBQuery::_IsConnected() const
{
	return fConnected;
}


void 
CDDBQuery::_Disconnect()
{
	if (fConnected) {
		fSocket.Close();
		fConnected = false;
	}
}


void 
CDDBQuery::_ReadLine(BString &buffer)
{
	buffer = "";
	unsigned char ch;

	for (;;) {
		if (fSocket.Receive(&ch, 1) <= 0)
			break;


		// This function is more work than it should have to be. FreeDB lookups
		// can sometimes  be in a non-ASCII encoding, such as Latin-1 or UTF8.
		// The problem lies in Be's implementation of BString, which does not
		// support UTF8 string assignments. The Be Book says we have to flatten
		// the string and adjust the character counts manually. Man, this
		// *really* sucks.
		if (ch > 0x7f) 	{
			// Obviously non-ASCII character detected. Let's see if it's Latin-1
			// or UTF8.
			unsigned char *string, *stringindex;
			int32 length = buffer.Length();

			// The first byte of a UTF8 string will be 110xxxxx
			if ( ((ch & 0xe0) == 0xc0)) {
				// This is UTF8. Get the next byte
				unsigned char ch2;
				if (fSocket.Receive(&ch2, 1) <= 0)
					break;

				if ( (ch2 & 0xc0) == 0x80) {
					string = (unsigned char *)buffer.LockBuffer(length + 10);
					stringindex = string + length;

					stringindex[0] = ch;
					stringindex[1] = ch2;
					stringindex[2] = 0;
					
					buffer.UnlockBuffer();
					
					// We've added the character, so go to the next iteration
					continue;
				}
			}

			// Nope. Just Latin-1. Convert to UTF8 and assign
			char srcstr[2], deststr[5];
			int32 srclen, destlen, state;

			srcstr[0] = ch;
			srcstr[1] = '\0';
			srclen = 1;
			destlen = 5;
			memset(deststr, 0, 5);

			if (convert_to_utf8(B_ISO1_CONVERSION, srcstr, &srclen, deststr,
					&destlen, &state) == B_OK) {
				// We succeeded. Amazing. Now we hack the string into having the
				// character
				length = buffer.Length();

				string = (unsigned char *)buffer.LockBuffer(length + 10);
				stringindex = string + length;

				for (int i = 0; i < 5; i++) {
					stringindex[i] = deststr[i];
					if (!deststr[i])
						break;
				}

				buffer.UnlockBuffer();
			} else {
				// well, we tried. Append the character to the string and live
				// with it
				buffer += ch;
			}
		} else
			buffer += ch;

		if (ch == '\n')
			break;
	}

	buffer.RemoveAll("\r");
	STRACE(("<%s", buffer.String()));
}


status_t 
CDDBQuery::_IdentifySelf()
{
	BString username, hostname;

	username = getenv("USER");
	hostname = getenv("HOSTNAME");

	if (username.Length() < 1)
		username = "baron";

	if (hostname.Length() < 1)
		hostname = "haiku";

	BString tmp;
	tmp << "cddb hello " << username << " " << hostname
		<< " HaikuCDPlayer 1.0\n";

	STRACE((">%s", tmp.String()));
	if (fSocket.Send(tmp.String(), tmp.Length())==-1) {
		_Disconnect();
		return B_ERROR;
	}

	_ReadLine(tmp);

	return B_OK;
}


status_t
CDDBQuery::_OpenContentFile(const int32 &discID)
{
	// Makes sure that the lookup has a valid file to work with for the CD
	// content. Returns true if there is an existing file, false if a lookup is
	// required.

	BFile file;
	BString predicate;
	predicate << "CD:key == " << discID;
	entry_ref ref;

	BVolumeRoster roster;
	BVolume volume;
	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsReadOnly() || !volume.IsPersistent() || !volume.KnowsAttr()
			|| !volume.KnowsQuery())
			continue;

		// make sure the volume we are looking at is indexed right
		fs_create_index(volume.Device(), "CD:key", B_INT32_TYPE, 0);

		BQuery query;
		query.SetVolume(&volume);
		query.SetPredicate(predicate.String());
		if (query.Fetch() != B_OK)
			continue;

		if (query.GetNextRef(&ref) == B_OK) 
			break;
	}

	status_t status = fCDData.Load(ref);
	if (status == B_NO_INIT) {
		// We receive this error when the Load() function couldn't load the
		// track times This just means that we get it from the SCSI data given
		// to us in SetToCD
		vector<CDAudioTime> times;
		GetTrackTimes(&fSCSIData,times);

		for (int32 i = 0; i < fCDData.CountTracks(); i++) {
			CDAudioTime *item = fCDData.TrackTimeAt(i);
			*item = times[i + 1] - times[i];
		}

		status = B_OK;
	}

	return status;
}


int32 
CDDBQuery::_QueryThread(void *owner)
{
	CDDBQuery *query = (CDDBQuery *)owner;

	signal(kTerminatingSignal, DoNothing);

	if (query->_OpenContentFile(query->fCDData.DiscID()) != B_OK) {
		// new content file, read it in from the server
		if (query->_Connect() == B_OK) {
			BString data;

			query->_ReadFromServer(data);
			query->_ParseData(data);
			query->_WriteFile();
		} else {
			// We apparently couldn't connect to the server, so we'll need to
			// handle creating tracknames. Note that we do not save to disk.
			// This is because it should be up to the user what to do.
			query->_SetDefaultInfo();
		}
	}

	query->fState = kDone;
	query->fThread = -1;
	query->fResult = B_OK;

	return 0;
}


void
CDDBQuery::_WriteFile()
{
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path, true) != B_OK)
		return;

	path.Append("cd");
	create_directory(path.Path(), 0755);

	BString filename(path.Path());
	filename << "/" << fCDData.Artist() << " - " << fCDData.Album();

	if (filename.Compare("Artist") == 0)
		filename << "." << fCDData.DiscID();

	fCDData.Save(filename.String());
}


void
CDDBQuery::_SetDefaultInfo()
{
	for (int16 i = fCDData.CountTracks(); i >= 0; i--)
		fCDData.RemoveTrack(i);

	vector<CDAudioTime> trackTimes;
	GetTrackTimes(&fSCSIData, trackTimes);
	int32 trackCount = GetTrackCount(&fSCSIData);

	fCDData.SetArtist("Artist");
	fCDData.SetAlbum("Audio CD");
	fCDData.SetGenre("Misc");

	for (int32 i = 0; i < trackCount; i++) {
		BString trackname("Track ");

		if (i < 9)
			trackname << "0";

		trackname << i + 1;

		CDAudioTime time = trackTimes[i + 1] - trackTimes[i];
		fCDData.AddTrack(trackname.String(), time);
	}
}			


void
CDDBQuery::_ParseData(const BString &data)
{
	// Can't simply call MakeEmpty() because the thread is spawned when the
	// discID kept in fCDData is not the same as what's in the drive and
	// MakeEmpty invalidates *everything* in the object. Considering that we
	// reassign everything here, simply emptying the track list should be sufficient
	for (int16 i = fCDData.CountTracks(); i >= 0; i--)
		fCDData.RemoveTrack(i);

	vector<CDAudioTime> trackTimes;
	GetTrackTimes(&fSCSIData,trackTimes);
	int32 trackCount = GetTrackCount(&fSCSIData);

	if (data.CountChars() < 1) {
		// This case occurs when the CDDB lookup fails. On these occasions, we
		// need to generate the file ourselves. This is actually pretty easy.
		fCDData.SetArtist("Artist");
		fCDData.SetAlbum("Audio CD");
		fCDData.SetGenre("Misc");

		for (int32 i = 0; i < trackCount; i++) 	{
			BString trackname("Track ");

			if (i < 9)
				trackname << "0";

			trackname << i + 1;

			CDAudioTime time = trackTimes[i + 1] - trackTimes[i];
			fCDData.AddTrack(trackname.String(), time);
		}
	}

	// TODO: This function is dog slow, but it works. Optimize.

	// Ideally, the search should be done sequentially using GetLineFromString()
	// and strchr(). Order: genre(category), frame offsets, disc length,
	// artist/album, track titles

	int32 pos;

	pos = data.FindFirst("DYEAR=");
	if (pos > 0) {
		BString artist,album;
		artist = album = GetLineFromString(data.String() + sizeof("DYEAR")
			+ pos);

		// TODO: finish, once I find an entry which actually has a year in it
		BAlert *alert = new BAlert("SimplyVorbis", "DYEAR entry found\n", "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		
	}

	pos = data.FindFirst("DTITLE=");
	if (pos > 0) {
		BString artist,album;
		artist = album = GetLineFromString(data.String() + sizeof("DTITLE")
			+ pos);

		pos = artist.FindFirst(" / ");
		if (pos > 0)
			artist.Truncate(pos);
		fCDData.SetArtist(artist.String());

		album = album.String() + pos + sizeof(" / ") - 1;
		fCDData.SetAlbum(album.String());
	}

	BString category = GetLineFromString(data.String() + 4);
	pos = category.FindFirst(" ");
	if (pos > 0)
		category.Truncate(pos);
	fCDData.SetGenre(category.String());

	pos = data.FindFirst("TTITLE0=");
	if (pos > 0) {
		int32 trackCount = 0;
		BString searchString = "TTITLE0=";

		while (pos > 0) {
			BString trackName = data.String() + pos + searchString.Length();
			trackName.Truncate(trackName.FindFirst("\n"));

			CDAudioTime tracktime = trackTimes[trackCount + 1]
				- trackTimes[trackCount];
			fCDData.AddTrack(trackName.String(),tracktime);

			trackCount++;
			searchString = "TTITLE";
			searchString << trackCount << "=";
			pos = data.FindFirst(searchString.String(),pos);
		}
	}
}


void CDDBQuery::SetData(const CDDBData &data)
{
	fCDData = data;
}
