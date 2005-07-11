// Copyright 1992-2000, Be Incorporated, All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
//
// send comments/suggestions/feedback to pavel@be.com
//

#include "CDDBSupport.h"

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NetAddress.h>
#include <Path.h>
#include <Query.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <errno.h>
#include <fs_index.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


// #pragma mark -
// some glue

template <class InitCheckable>
void
InitCheck(InitCheckable *item)
{
	if (!item)
		throw B_ERROR;
	status_t error = item->InitCheck();
	if (error != B_OK)
		throw error;
}

inline void
ThrowOnError(status_t error)
{
	if (error != B_OK)
		throw error; 
}

inline void
ThrowIfNotSize(ssize_t size)
{
	if (size < 0)
		throw (status_t)size; 
}


// #pragma mark -

CDDBQuery::CDDBQuery(const char *server, int32 port, bool log)
	:	log(log),
		serverName(server),
		port(port),
		connected(false),
		discID(-1),
		state(kInitial)
{
}

void 
CDDBQuery::SetToSite(const char *server, int32 port)
{
	Disconnect();
	serverName = server;
	port = port;
	state = kInitial;
}

void 
CDDBQuery::SetToCD(const char *path)
{
	if(!path)
		return;
	
	// Get the SCSI table of contents from the device passed to us
	int device = open(path, O_RDONLY);
	if(device < 0)
		return;
	
	scsi_toc toc;
	status_t result = ioctl(device, B_SCSI_GET_TOC, &toc);
	
	close(device);
	
	if(result != B_OK)
		return;
	
	
	if (state == kInitial) 
	{
		GetDiscID(&toc, discID, numTracks, discLength, frameOffsetString, discIDStr);
	}
	else
	{
		int32 tmpDiscID;
		int32 tmpDiscLength;
		int32 tmpNumTracks;
		BString tmpFrameOffsetString;
		BString tmpDiscIDStr;
		
		GetDiscID(&toc, tmpDiscID, tmpNumTracks, tmpDiscLength, tmpFrameOffsetString,
			tmpDiscIDStr);
		
		if (discID == tmpDiscID && discLength == tmpDiscLength && numTracks == tmpNumTracks
			&& frameOffsetString == frameOffsetString)
			return;

		discID = tmpDiscID;
		discLength = tmpDiscLength;
		numTracks = tmpNumTracks;
		frameOffsetString = tmpFrameOffsetString;
		discIDStr = tmpDiscIDStr;
	}
	
	// ToDo:
	// add interrupting here
	
	result = B_OK;
	state = kReading;
	thread = spawn_thread(&CDDBQuery::LookupBinder, "CDDBLookup", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		resume_thread(thread);
	else {
		state = kError;
		result = thread;
	}
}

const int kTerminatingSignal = SIGINT; // SIGCONT;

static void
DoNothing(int)
{
}

int32 
CDDBQuery::LookupBinder(void *castToThis)
{
	CDDBQuery *query = (CDDBQuery *)castToThis;

	BFile file;
	entry_ref ref;
	bool newFile = false;

	try {
		signal(kTerminatingSignal, DoNothing);
	
		if (!query->FindOrCreateContentFileForDisk(&file, &ref, query->discID)) {
			// new content file, read it in from the server
			Connector connection(query);
			query->ReadFromServer(&file);
			file.Seek(0, SEEK_SET);
			newFile = true;
		}
		query->ParseResult(&file);
	
		query->state = kDone;
		query->thread = -1;
		query->result = B_OK;
		
		if (newFile) {
			BString newTitle(query->title);
			int32 length = newTitle.Length();
			for (int32 index = 0; index < length; index++) {
				if (newTitle[index] == '/' || newTitle[index] == ':')
					newTitle[index] = '-';
			}
			
			file.Sync();
			BEntry entry(&ref);
			entry.Rename(newTitle.String(), false);
		}

	} catch (status_t error) {
		query->state = kError;
		query->result = error;
	
		// the cached file is messed up, remove it
		BEntry entry(&ref);
		BPath path;
		entry.GetPath(&path);
		printf("removing bad CD content file %s\n", path.Path());
		entry.Remove();
	}
	return 0;
}


const char * 
CDDBQuery::GetToken(const char *stream, BString &result)
{
	result = "";
	while (*stream && *stream <= ' ')
		stream++;
	while (*stream && *stream > ' ')
		result += *stream++;
	
	return stream;
}


void 
CDDBQuery::GetSites(bool (*eachFunc)(const char *site, int port, const char *latitude,
		const char *longitude, const char *description, void *state), void *passThru)
{
	// ToDo:
	// add interrupting here

	Connector connection(this);
	BString tmp;
	
	tmp = "sites\n";
	if (log)
		printf(">%s", tmp.String());

	ThrowIfNotSize( socket.Send(tmp.String(), tmp.Length()) );
	ReadLine(tmp);

	if (tmp.FindFirst("210") == -1)
		throw (status_t)B_ERROR;

	for (;;) {
		BString site;
		int32 sitePort;
		BString latitude;
		BString longitude;
		BString description;

		ReadLine(tmp);
		if (tmp == ".")
			break;
		const char *scanner = tmp.String();

		scanner = GetToken(scanner, site);
		BString portString;
		scanner = GetToken(scanner, portString);
		sitePort = atoi(portString.String());
		scanner = GetToken(scanner, latitude);
		scanner = GetToken(scanner, longitude);
		description = scanner;
		
		if (eachFunc(site.String(), sitePort, latitude.String(), longitude.String(),
			description.String(), passThru))
			break;
	}
}

bool 
CDDBQuery::GetTitles(BString *resultingTitle, vector<BString> *tracks, bigtime_t timeout)
{
	bigtime_t deadline = system_time() + timeout;
	while (state == kReading) {
		snooze(50000);
		if (system_time() > deadline)
			break;
	}
	if (state != kDone)
		return false;

	if (resultingTitle)
		*resultingTitle = title;
	
	if (tracks)
		*tracks = trackNames;
	return true;
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

struct ConvertedToc {
	int32 min;
	int32 sec;
	int32 frame;
};

int32 
CDDBQuery::GetDiscID(const scsi_toc *toc)
{
	int32 tmpDiscID;
	int32 tmpDiscLength;
	int32 tmpNumTracks;
	BString tmpFrameOffsetString;
	BString tmpDiscIDStr;
	
	GetDiscID(toc, tmpDiscID, tmpNumTracks, tmpDiscLength, tmpFrameOffsetString,
		tmpDiscIDStr);
	
	return tmpDiscID;
}

void 
CDDBQuery::GetDiscID(const scsi_toc *toc, int32 &id, int32 &numTracks,
	int32 &length, BString &frameOffsetsString, BString &discIDString)
{
	ConvertedToc tocData[100];

	// figure out the disc ID
	for (int index = 0; index < 100; index++) {
		tocData[index].min = toc->toc_data[9 + 8*index];
		tocData[index].sec = toc->toc_data[10 + 8*index];
		tocData[index].frame = toc->toc_data[11 + 8*index];
	}
	numTracks = toc->toc_data[3] - toc->toc_data[2] + 1;
	
	int32 sum1 = 0;
	int32 sum2 = 0;
	for (int index = 0; index < numTracks; index++) {
		sum1 += cddb_sum((tocData[index].min * 60) + tocData[index].sec);
		// the following is probably running over too far
		sum2 +=	(tocData[index + 1].min * 60 + tocData[index + 1].sec) -
			(tocData[index].min * 60 + tocData[index].sec);
	}
	id = ((sum1 % 0xff) << 24) + (sum2 << 8) + numTracks;
	discIDString = "";
	
	sprintf(discIDString.LockBuffer(10), "%08lx", id);
	discIDString.UnlockBuffer();

	// compute the total length of the CD.
	length = tocData[numTracks].min * 60 + tocData[numTracks].sec;
	
	for (int index = 0; index < numTracks; index++) 
		frameOffsetsString << tocData[index].min * 4500 + tocData[index].sec * 75
			+ tocData[index].frame << ' ';
}

void
CDDBQuery::ReadFromServer(BDataIO *stream)
{
	// Format the query
	BString query;
	query << "cddb query " << discIDStr << ' ' << numTracks << ' ' 
		// Add frame offsets
		<< frameOffsetString << ' '
		// Finish it off with the total CD length.
		<< discLength << '\n';
	
	if (log)
		printf(">%s", query.String());

	// Send it off.
	ThrowIfNotSize( socket.Send(query.String(), query.Length()) );

	BString tmp;
	ReadLine(tmp);
	if(tmp.FindFirst("200") != 0)
	{
		if(tmp.FindFirst("211") == 0)
		{
			while(tmp.CountChars() > 0)
			{
				printf("%s\n",tmp.String());
				ReadLine(tmp);
			}
		}
		else
			printf("Error: %s\n",tmp.String());
		return;
	}
	BString category;
	GetToken(tmp.String() + 3, category);
	if (!category.Length())
		category = "misc";

	query = "";
	query << "cddb read " << category << ' ' << discIDStr << '\n' ;
	ThrowIfNotSize( socket.Send(query.String(), query.Length()) );

	for (;;) {
		BString tmp;
		ReadLine(tmp);
		tmp += '\n';
		ThrowIfNotSize( stream->Write(tmp.String(), tmp.Length()) );
		if (tmp == "." || tmp == ".\n")
			break;
	}
	state = kDone;
}

void 
CDDBQuery::ParseResult(BDataIO *source)
{
	title = "";
	trackNames.clear();
	for (;;) {
		BString tmp;
		ReadLine(source, tmp);
		if (tmp == ".")
			break;
			
		if (tmp == "")
			throw (status_t)B_ERROR;
		
		if (tmp.FindFirst("DTITLE=") == 0)
			title = tmp.String() + sizeof("DTITLE");
		else if (tmp.FindFirst("TTITLE") == 0) {
			int32 afterIndex = tmp.FindFirst('=');
			if (afterIndex > 0) {
				BString trackName(tmp.String() + afterIndex + 1);
				trackNames.push_back(trackName);
			}
		}
	}
	state = kDone;
}


void
CDDBQuery::Connect()
{
	BNetAddress address(serverName.String(), port);
	InitCheck(&address);

	ThrowOnError( socket.Connect(address) );
	connected = true;

	BString tmp;
	ReadLine(tmp);
	IdentifySelf();
}

bool 
CDDBQuery::IsConnected() const
{
	return connected;
}

void 
CDDBQuery::Disconnect()
{
	socket.Close();
	connected = false;
}

void 
CDDBQuery::ReadLine(BString &buffer)
{
	buffer = "";
	char ch;
	for (;;) {
		if (socket.Receive(&ch, 1) <= 0)
			break;
		if (ch >= ' ')
			buffer += ch;
		if (ch == '\n')
			break;
	}
	if (log)
		printf("<%s\n", buffer.String());
}

void 
CDDBQuery::ReadLine(BDataIO *stream, BString &buffer)
{
	// this is super-lame, should use better buffering
	// ToDo:
	// redo using LockBuffer, growing the buffer by 2k and reading into it

	buffer = "";
	char ch;
	for (;;) {
		ssize_t result = stream->Read(&ch, 1);

		if (result < 0)
			// read error
			throw (status_t)result;
		if (result == 0)
			break;
		if (ch >= ' ')
			buffer += ch;
		if (ch == '\n')
			break;
	}
}


void 
CDDBQuery::IdentifySelf()
{
	char username[256];
	if (!getusername(username,256))
		strcpy(username, "unknown");
	
	char hostname[MAXHOSTNAMELEN + 1];
	if (gethostname(hostname, MAXHOSTNAMELEN) == -1)
		strcpy(hostname, "unknown");
	
	BString tmp;
	tmp << "cddb hello " << username << " " << hostname << " Haiku_CD_Player v1.0\n";

	if (log)
		printf(">%s", tmp.String());
	ThrowIfNotSize( socket.Send(tmp.String(), tmp.Length()) );
	
	ReadLine(tmp);
}

bool 
CDDBQuery::FindOrCreateContentFileForDisk(BFile *file, entry_ref *fileRef, int32 discID)
{
	BString predicate;
	predicate << "cddb:discID == " << discID;
	entry_ref ref;
		
	BVolumeRoster roster;
	BVolume volume;
	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsReadOnly() || !volume.IsPersistent()
			|| !volume.KnowsAttr() || !volume.KnowsQuery())
			continue;
		
		// make sure the volume we are looking at is indexed right
		fs_create_index(volume.Device(), "cddb:discID", B_INT32_TYPE, 0);

		BQuery query;
		query.SetVolume(&volume);
		query.SetPredicate(predicate.String());
		if (query.Fetch() != B_OK)
			continue;
		
		while (query.GetNextRef(&ref) == B_OK) {
			// we have one already, return 
			file->SetTo(&ref, O_RDONLY);
			*fileRef = ref;
			return true;
		}
	}

	BPath path;
	ThrowOnError( find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) );
	path.Append("CDContentFiles");
	ThrowOnError( create_directory(path.Path(), 0755) );
	
	BDirectory dir(path.Path());
	BString name("CDContent");
	for (int32 index = 0; ;index++) {
		if (dir.CreateFile(name.String(), file, true) != B_FILE_EXISTS) {
			BEntry entry(&dir, name.String());
			entry.GetRef(fileRef);
			file->WriteAttr("cddb:discID", B_INT32_TYPE, 0, &discID, sizeof(int32));
			break;
		}
		name = "CDContent";
		name << index;
	}
	return false;
}

/*

#include "CDEngine.h"

bool
PrintSite(const char *site, int port, const char *latitude,
	const char *longitude, const char *description, void *)
{
	printf("server:%s port:%d latitude:%s longitude:%s description: %s\n",
		site, port, latitude, longitude, description);
	return false;
}

int
main()
{
		CDDBQuery query("us.freedb.org", 888, true);

		int32 device = CDEngine::FindCDPlayerDevice();
		if (!device) 
		{
			printf("no device %s\n", strerror(device));
			return 0;
		}
		
		scsi_toc toc;

		ioctl(device, B_SCSI_GET_TOC, &toc);
		int32 discID = query.GetDiscID(&toc);
		
		query.SetToCD(&toc);

		BString title;
		vector<BString> trackTitles;
		query.GetTitles(&title, &trackTitles, 10000000);
		
		printf("CD Title: %s, ID: %lx\n", title.String(), discID);
		int32 index = 0;
		for (vector<BString>::iterator iterator = trackTitles.begin();
			iterator != trackTitles.end(); index++, iterator++)
			printf("%ld: %s\n", index, (*iterator).String());
	
	return 0;
}

*/
