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
#include <fs_attr.h>
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
typedef struct TrackRecord
{
	int8	unused[5];
	
	int8	min;
	int8	sec;
	int8	frame;
};


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


CDDBQuery::CDDBQuery(const char *server, int32 port)
	:	fServerName(server),
		fPort(port),
		fConnected(false),
		fState(kInitial),
		fDiscID(-1)
{
}

CDDBQuery::~CDDBQuery(void)
{
	kill_thread(fThread);
}

void 
CDDBQuery::SetToSite(const char *server, int32 port)
{
	Disconnect();
	fServerName = server;
	fPort = port;
	fState = kInitial;
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
	
	// Calculate the disc's CDDB ID
	if (fState == kInitial) 
	{
		fDiscID = GetDiscID(&toc);
		fTrackCount = GetTrackCount(&toc);
		
		cdaudio_time time = GetDiscTime(&toc);
		fDiscLength = (time.minutes * 60) + time.seconds;
		fFrameOffsetString = OffsetsToString(&toc);
		GetTrackTimes(&toc, fTrackTimes);
	}
	else
	{
		int32 discID = GetDiscID(&toc);
		
		if (fDiscID == discID)
			return;
		
		fDiscID = discID;
		fTrackCount = GetTrackCount(&toc);
		
		cdaudio_time time = GetDiscTime(&toc);
		fDiscLength = (time.minutes * 60) + time.seconds;
		fFrameOffsetString = OffsetsToString(&toc);
		GetTrackTimes(&toc, fTrackTimes);
	}
	
	result = B_OK;
	fState = kReading;
	fThread = spawn_thread(&CDDBQuery::QueryThread, "CDDBLookup", B_NORMAL_PRIORITY, this);
	
	if (fThread >= 0)
		resume_thread(fThread);
	else 
	{
		fState = kError;
		result = fThread;
	}
}

static void
DoNothing(int)
{
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
	Connector connection(this);
	BString tmp;
	
	tmp = "sites\n";
	STRACE((">%s", tmp.String()));

	ThrowIfNotSize( fSocket.Send(tmp.String(), tmp.Length()) );
	ReadLine(tmp);

	if (tmp.FindFirst("210") == -1)
		throw (status_t)B_ERROR;

	for (;;) 
	{
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
	while (fState == kReading) 
	{
		snooze(50000);
		if (system_time() > deadline)
			break;
	}
	if (fState != kDone)
		return false;

	if (resultingTitle)
	{
		*resultingTitle = fArtist;
		resultingTitle->Append(" / ");
		resultingTitle->Append(fTitle);
	}
	
	if (tracks)
		*tracks = fTrackNames;
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

int32 
CDDBQuery::GetDiscID(const scsi_toc *toc)
{
	// The SCSI TOC data has a 4-byte header and then each track record starts
	TrackRecord *tocData = (TrackRecord*)&(toc->toc_data[4]);

	int32 numTracks = toc->toc_data[3] - toc->toc_data[2] + 1;
	
	int32 sum1 = 0;
	int32 sum2 = 0;
	for (int index = 0; index < numTracks; index++) 
	{
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
	for (int index = 0; index < numTracks; index++)
	{
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

cdaudio_time
CDDBQuery::GetDiscTime(const scsi_toc *toc)
{
	int16 trackcount = toc->toc_data[3] - toc->toc_data[2] + 1;
	TrackRecord *desc = (TrackRecord*)&(toc->toc_data[4]);
	
	cdaudio_time disc;
	disc.minutes = desc[trackcount].min;
	disc.seconds = desc[trackcount].sec;
	
	return disc;
}

void
CDDBQuery::GetTrackTimes(const scsi_toc *toc, vector<cdaudio_time> &times)
{
	TrackRecord *tocData = (TrackRecord*)&(toc->toc_data[4]);
	int16 trackCount = toc->toc_data[3] - toc->toc_data[2] + 1;
	
	for (int index = 0; index < trackCount+1; index++)
	{
		cdaudio_time cdtime;
		cdtime.minutes = tocData[index].min;
		cdtime.seconds = tocData[index].sec;
		times.push_back(cdtime);
	}
}

status_t
CDDBQuery::ReadFromServer(BString &data)
{
	// This function queries the given CDDB server for the existence of the disc's data and
	// saves the data to file once obtained.
	
	// Query for the existence of the disc in the database
	char idString[10];
	sprintf(idString, "%08lx", fDiscID);
	
	BString query;
	query << "cddb query " << idString << ' ' << fTrackCount << ' ' 
		<< fFrameOffsetString << ' ' << fDiscLength << '\n';
	
	STRACE((">%s", query.String()));

	ThrowIfNotSize( fSocket.Send(query.String(), query.Length()) );

	BString tmp;
	ReadLine(tmp);
	
	BString category;
	BString queryDiscID(idString);
	
	if(tmp.FindFirst("200") != 0)
	{
		if(tmp.FindFirst("211") == 0)
		{
			// A 211 means that the query was not exact. To make sure that we don't
			// have a problem with this in the future, we will choose the first entry that
			// the server returns. This may or may not be wise, but in my experience, the first
			// one has been the right one.
			
			ReadLine(tmp);
			
			// Get the category from the what the server returned
			GetToken(tmp.String(), category);
			
			// Now we will get the disc ID for the CD. We will need this when we query for
			// the track name list. If we send the track name query with the real discID, nothing
			// will be returned. However, if we send the one from the entry, we'll get the names
			// and we can take these names attach them to the disc that we have.
			GetToken(tmp.String() + category.CountChars(),queryDiscID);
			
			// This is to suck up any more search results that the server sends us.
			BString throwaway;
			ReadLine(throwaway);
			while(throwaway.ByteAt(0) != '.')
				ReadLine(throwaway);
		}
		else
		{
			// We get here for any time the CDDB server does not recognize the CD, amongst other things
			STRACE(("CDDB lookup error: %s\n",tmp.String()));
			fCategory = "misc";
			return B_NAME_NOT_FOUND;
		}
	}
	else
	{
		GetToken(tmp.String() + 3, category);
	}
	
	if (!category.Length())
		category = "misc";

	// Query for the disc's data - artist, album, tracks, etc.
	query = "";
	query << "cddb read " << category << ' ' << queryDiscID << '\n' ;
	ThrowIfNotSize( fSocket.Send(query.String(), query.Length()) );
	
	while(true)
	{
		ReadLine(tmp);
		
		if (tmp == "." || tmp == ".\n")
			break;
		
		data << tmp << '\n';
	}
	
	return B_OK;
}

void
CDDBQuery::Connect()
{
	BNetAddress address(fServerName.String(), fPort);
	InitCheck(&address);

	ThrowOnError( fSocket.Connect(address) );
	fConnected = true;

	BString tmp;
	ReadLine(tmp);
	IdentifySelf();
}

bool 
CDDBQuery::IsConnected() const
{
	return fConnected;
}

void 
CDDBQuery::Disconnect()
{
	fSocket.Close();
	fConnected = false;
}

void 
CDDBQuery::ReadLine(BString &buffer)
{
	buffer = "";
	char ch;
	for (;;) 
	{
		if (fSocket.Receive(&ch, 1) <= 0)
			break;
		if (ch >= ' ')
			buffer += ch;
		if (ch == '\n')
			break;
	}
	STRACE(("<%s\n", buffer.String()));
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

	STRACE((">%s", tmp.String()));
	ThrowIfNotSize( fSocket.Send(tmp.String(), tmp.Length()) );
	
	ReadLine(tmp);
}

bool 
CDDBQuery::OpenContentFile(const int32 &discID)
{
	// Makes sure that the lookup has a valid file to work with for the CD content.
	// Returns true if there is an existing file, false if a lookup is required.
	
	BFile file;
	BString predicate;
	predicate << "CD:key == " << discID;
	entry_ref ref;
		
	BVolumeRoster roster;
	BVolume volume;
	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) 
	{
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
		
		if(query.GetNextRef(&ref) == B_OK) 
		{
			file.SetTo(&ref, B_READ_ONLY);
			break;
		}
	}
	
	if(file.InitCheck() == B_NO_INIT)
		return false;
	
	// Getting this far means that we have a file. Now we parse the data in it and assign it
	// to the appropriate members of the object
	attr_info info;
	
	if(file.GetAttrInfo("CD:tracks",&info)==B_OK && info.size > 0)
	{
		char trackdata[info.size+2];
		
		if(file.ReadAttr("CD:tracks",B_STRING_TYPE,0,trackdata,info.size)>0)
		{
			trackdata[info.size] = 0;
			BString tmp = GetLineFromString(trackdata);
			char *index;
			
			fTrackNames.clear();
			fArtist = tmp;
			fArtist.Truncate(fArtist.FindFirst(" - "));
			
			fTitle = strchr(tmp.String(),'-') + 2;
			
			index = strchr(trackdata,'\n') + 1;
			while(*index)
			{
				tmp = GetLineFromString(index);
				
				fTrackNames.push_back(tmp);
				index = strchr(index,'\n') + 1;
			}
			
			return true;
		}
	}
	
	// If we got this far, it means the file is corrupted, so nuke it and get a new one
	BEntry entry(&ref);
	entry.Remove();
	return false;
}

int32 
CDDBQuery::QueryThread(void *owner)
{
	CDDBQuery *query = (CDDBQuery *)owner;
	
	try 
	{
		signal(kTerminatingSignal, DoNothing);
	
		if(!query->OpenContentFile(query->fDiscID)) 
		{
			// new content file, read it in from the server
			Connector connection(query);
			BString data;
			
			query->ReadFromServer(data);
			query->ParseData(data);
			query->WriteFile();
		}
	
		query->fState = kDone;
		query->fThread = -1;
		query->fResult = B_OK;
	}
	catch (status_t error) 
	{
		query->fState = kError;
		query->fResult = error;
	}
	return 0;
}

void
CDDBQuery::WriteFile(void)
{
	BPath path;
	if(find_directory(B_USER_DIRECTORY, &path, true)!=B_OK)
		return;
	
	path.Append("cd");
	create_directory(path.Path(), 0755);
	
	BString filename(path.Path());
	filename << "/" << fArtist << " - " << fTitle;
	
	if(filename.Compare("Artist")==0)
		filename << "." << fDiscID;
	
	BFile file(filename.String(), B_READ_WRITE | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if(file.InitCheck() != B_OK)
		return;
	
	BString entry;
	char timestring[10];
	
	sprintf(timestring,"%.2ld:%.2ld",fDiscLength / 60, fDiscLength % 60);
	
	entry << fArtist << " - " << fTitle << "\t" << timestring << "\n";
	file.Write(entry.String(),entry.Length());
	
	BString tracksattr(fArtist);
	tracksattr << " - " << fTitle << "\n";
	
	for(int32 i=0; i<fTrackCount; i++)
	{
		entry = fTrackNames[i];
		
		sprintf(timestring,"%.2ld:%.2ld",fTrackTimes[i+1].minutes-fTrackTimes[i].minutes,
				fTrackTimes[i].seconds);
		
		entry << "\t" << timestring << "\n";
		file.Write(entry.String(),entry.Length());
		
		tracksattr << fTrackNames[i] << "\n";
	}
	
	file.WriteAttr("CD:key", B_INT32_TYPE, 0, &fDiscID, sizeof(int32));
	file.WriteAttr("CD:tracks", B_STRING_TYPE, 0, tracksattr.String(), tracksattr.Length());
}

void
CDDBQuery::ParseData(const BString &data)
{
	fTitle = "";
	fTrackNames.clear();
	
	if(data.CountChars()<1)
	{
		// This case occurs when the CDDB lookup fails. On these occasions, we need to generate
		// the file ourselves. This is actually pretty easy.
		fArtist = "Artist";
		fTitle = "Audio CD";
		fCategory = "misc";
		
		for(int32 i=0; i<fTrackCount; i++)
		{
			BString trackname("Track ");
			
			if(i<9)
				trackname << "0";
			
			trackname << i+1;
			fTrackNames.push_back(trackname.String());
		}
		
	}
	
	// TODO: This function is dog slow, but it works. Optimize.
	
	// Ideally, the search should be done sequentially using GetLineFromString() and strchr().
	// Order: genre(category), frame offsets, disc length, artist/album, track titles
	
	int32 pos;
	
	pos = data.FindFirst("DTITLE=");
	if(pos > 0)
	{
		fTitle = fArtist = GetLineFromString(data.String() + sizeof("DTITLE") + pos);
		
		pos = fArtist.FindFirst(" / ");
		if(pos > 0)
			fArtist.Truncate(pos);
		
		fTitle = fTitle.String() + pos + sizeof(" / ") - 1;
	}
	
	
	fCategory = GetLineFromString(data.String() + 4);
	pos = fCategory.FindFirst(" ");
	if(pos > 0)
		fCategory.Truncate(pos);
	
	
	pos = data.FindFirst("Disc length: ");
	if(pos > 0)
	{
		BString discLengthString = GetLineFromString(data.String() + pos);
		pos = discLengthString.FindFirst(" seconds");
		if(pos > 0)
		{
			discLengthString.Truncate(pos);
		
			discLengthString = discLengthString.String() + sizeof("Disc length:");
			fDiscLength = atoi(discLengthString.String());
		}
		else
			fDiscLength = -1;
	}

	pos = data.FindFirst("TTITLE0=");
	if(pos > 0)
	{
		int32 trackCount=0;
		BString searchString="TTITLE0=";
		
		while(pos > 0)
		{
			BString trackName = data.String() + pos + searchString.Length();
			trackName.Truncate(trackName.FindFirst("\n"));
			fTrackNames.push_back(trackName);
			
			trackCount++;
			searchString = "TTITLE";
			searchString << trackCount << "=";
			pos = data.FindFirst(searchString.String(),pos);
		}
		fTrackCount = trackCount;
	}
}

BString
CDDBQuery::GetLineFromString(const char *string)
{
	if(!string)
		return NULL;
	
	BString out(string);
	
	int32 linefeed = out.FindFirst("\n");
	
	if(linefeed > 0)
		out.Truncate(linefeed);
	
	return out;
}
