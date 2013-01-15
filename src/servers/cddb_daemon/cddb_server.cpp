/*
 * Copyright 2008-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */
 
#include "cddb_server.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static const char* kDefaultLocalHostName = "unknown";
static const uint32 kDefaultPortNumber = 80;

static const uint32 kFramesPerSecond = 75;
static const uint32 kFramesPerMinute = kFramesPerSecond * 60;


CDDBServer::CDDBServer(const BString& cddbServer):
	fInitialized(false),
	fConnected(false)
{

	// Set up local host name.
	char localHostName[MAXHOSTNAMELEN + 1];
	if (gethostname(localHostName,  MAXHOSTNAMELEN + 1) == 0) {
		fLocalHostName = localHostName;
	} else {
		fLocalHostName = kDefaultLocalHostName;
	}
	
	// Set up local user name.
	char* user = getenv("USER");
	if (user == NULL) {
		fLocalUserName = "unknown";
	} else {
		fLocalUserName = user;
	}

	// Set up server address;
	if (_ParseAddress(cddbServer) == B_OK)
		fInitialized = true;		
}


status_t
CDDBServer::Query(uint32 cddbId, const scsi_toc_toc* toc, BList* queryResponse)
{
	if (_OpenConnection() != B_OK)
		return B_ERROR;

	// Convert CDDB id to hexadecimal format.
	char hexCddbId[9];
	sprintf(hexCddbId, "%08" B_PRIx32 "", cddbId);
	
	// Assemble the Query command.
	int32 numTracks = toc->last_track + 1 - toc->first_track;
	
	BString cddbCommand("cddb query ");
	cddbCommand << hexCddbId << " " << numTracks << " ";

	// Add track offsets in frames.
	for (int32 i = 0; i < numTracks; ++i) {
		const scsi_cd_msf& start = toc->tracks[i].start.time;
		
		uint32 startFrameOffset = start.minute * kFramesPerMinute +
			start.second * kFramesPerSecond + start.frame;
		
		cddbCommand << startFrameOffset << " ";
	}
	
	// Add total disc time in seconds. Last track is lead-out.
	const scsi_cd_msf& lastTrack = toc->tracks[numTracks].start.time;
	uint32 totalTimeInSeconds = lastTrack.minute * 60 + lastTrack.second;
	cddbCommand << totalTimeInSeconds;	

	BString output;
	status_t result;
	result = _SendCddbCommand(cddbCommand, &output);
	
	if (result == B_OK) {
		// Remove the header from the reply.
		output.Remove(0, output.FindFirst("\r\n\r\n") + 4);
		
		// Check status code.
		BString statusCode;
		output.MoveInto(statusCode, 0, 3);
		if (statusCode == "210" || statusCode == "211") {
			// TODO(bga): We can get around with returning the first result
			// in case of multiple matches, but we most definitely needs a
			// better handling of inexact matches.
			if (statusCode == "211")
				printf("Warning : Inexact match found.\n");

			// Multiple results, remove the first line and parse the others.
			output.Remove(0, output.FindFirst("\r\n") + 2);
		} else if (statusCode == "200") {
			// Remove the first char which is a left over space.
			output.Remove(0, 1);
		} else if (statusCode == "202") {
			// No match found.
			printf("Error : CDDB entry for id %s not found.\n", hexCddbId);
			
			return B_ENTRY_NOT_FOUND;
		} else {
			// Something bad happened.
			if (statusCode.Trim() != "") {
				printf("Error : CDDB server status code is %s.\n",
					statusCode.String());
			} else {
				printf("Error : Could not find any status code.\n");
			}

			return B_ERROR;
		}
		
		// Process all entries.
		bool done = false;
		while (!done) {
			QueryResponseData* responseData = new QueryResponseData;
			
			output.MoveInto(responseData->category, 0, output.FindFirst(" "));
			output.Remove(0, 1);
			
			output.MoveInto(responseData->cddbId, 0, output.FindFirst(" "));
			output.Remove(0, 1);

			output.MoveInto(responseData->artist, 0, output.FindFirst(" / "));
			output.Remove(0, 3);			

			output.MoveInto(responseData->title, 0, output.FindFirst("\r\n"));
			output.Remove(0, 2);
			
			queryResponse->AddItem(responseData);
			
			if (output == "" || output == ".\r\n") {
				// All returned data was processed exit the loop.
				done = true;
			}
		}
	} else {
		printf("Error sending CDDB command : \"%s\".\n", cddbCommand.String());
	}

	_CloseConnection();
	return result;
}


status_t
CDDBServer::Read(QueryResponseData* diskData, ReadResponseData* readResponse)
{
	if (_OpenConnection() != B_OK)
		return B_ERROR;

	// Assemble the Read command.
	BString cddbCommand("cddb read ");
	cddbCommand << diskData->category << " " << diskData->cddbId;

	BString output;
	status_t result;
	result = _SendCddbCommand(cddbCommand, &output);

	if (result == B_OK) {
		// Remove the header from the reply.
		output.Remove(0, output.FindFirst("\r\n\r\n") + 4);
		
		// Check status code.
		BString statusCode;
		output.MoveInto(statusCode, 0, 3);
		if (statusCode == "210") {
			// Remove first line and parse the others.
			output.Remove(0, output.FindFirst("\r\n") + 2);
		} else {
			// Something bad happened.
			return B_ERROR;
		}
		
		// Process all entries.
		bool done = false;
		while (!done) {
			if (output[0] == '#') {
				// Comment. Remove it.
				output.Remove(0, output.FindFirst("\r\n") + 2);
				continue;
			}
			
			// Extract one line to reduce the scope of processing to it.
			BString line;
			output.MoveInto(line, 0, output.FindFirst("\r\n"));
			output.Remove(0, 2);

			// Obtain prefix.
			BString prefix;
			line.MoveInto(prefix, 0, line.FindFirst("="));
			line.Remove(0, 1);
			
			if (prefix == "DTITLE") {
				// Disk title.
				BString artist;
				line.MoveInto(artist, 0, line.FindFirst(" / "));
				line.Remove(0, 3);
				readResponse->title = line;
				readResponse->artist = artist;
			} else if (prefix == "DYEAR") {
				// Disk year.
				char* firstInvalid;
				errno = 0;
				uint32 year = strtoul(line.String(), &firstInvalid, 10);
				if ((errno == ERANGE &&
					(year == (uint32)LONG_MAX || year == (uint32)LONG_MIN))
					|| (errno != 0 && year == 0)) {
					// Year out of range.
					printf("Year out of range: %s\n", line.String());
					return B_ERROR;
				}
				
				if (firstInvalid == line.String()) {
					printf("Invalid year: %s\n", line.String());
					return B_ERROR;
				}
							
				readResponse->year = year;
			} else if (prefix == "DGENRE") {
				// Disk genre.
				readResponse->genre = line;
			} else if (prefix.FindFirst("TTITLE") == 0) {
				// Track title.
				BString index;					
				prefix.MoveInto(index, 6, prefix.Length() - 6);

				TrackData* trackData = new TrackData;

				char* firstInvalid;
				errno = 0;
				uint32 track = strtoul(index.String(), &firstInvalid, 10);
				if ((errno == ERANGE &&
					(track == (uint32)LONG_MAX || track == (uint32)LONG_MIN))
					|| (errno != 0 && track == 0)) {
					// Track out of range.
					printf("Track out of range: %s\n", index.String());
					delete trackData;
					return B_ERROR;
				}
				
				if (firstInvalid == index.String()) {
					printf("Invalid track: %s\n", index.String());
					delete trackData;
					return B_ERROR;	
				}

				trackData->trackNumber = track;

				int32 pos = line.FindFirst(" / " );
				if (pos != B_ERROR) {
					// We have track specific artist information.
					BString artist;
					line.MoveInto(artist, 0, pos);
					line.Remove(0, 3);
					trackData->artist = artist;
				} else {
					trackData->artist = diskData->artist;
				}

				trackData->title = line;
					
				(readResponse->tracks).AddItem(trackData);
			}
			
			if (output == "" || output == ".\r\n") {
				// All returned data was processed exit the loop.
				done = true;
			}			
		}		
	} else {
		printf("Error sending CDDB command : \"%s\".\n", cddbCommand.String());		
	}	

	_CloseConnection();
	return B_OK;
}


status_t
CDDBServer::_ParseAddress(const BString& cddbServer)
{
	// Set up server address.
	int32 pos = cddbServer.FindFirst(":");
	if (pos == B_ERROR) {
		// It seems we do not have the address:port format. Use hostname as-is.
		fCddbServerAddr.SetTo(cddbServer.String(), kDefaultPortNumber);
		if (fCddbServerAddr.InitCheck() == B_OK)
			return B_OK;
	} else {
		// Parse address:port format.
		int32 port;
		BString newCddbServer(cddbServer);
		BString portString;
		newCddbServer.MoveInto(portString, pos + 1,
			newCddbServer.CountChars() - pos + 1);
		if (portString.CountChars() > 0) {
			char* firstInvalid;
			errno = 0;
			port = strtol(portString.String(), &firstInvalid, 10);
			if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
				|| (errno != 0 && port == 0)) {
				return B_ERROR;
			}
			if (firstInvalid == portString.String()) {
				return B_ERROR;
			}
			
			newCddbServer.RemoveAll(":");
			fCddbServerAddr.SetTo(newCddbServer.String(), port);
			if (fCddbServerAddr.InitCheck() == B_OK)
				return B_OK;
		} 
	}

	return B_ERROR;
}


status_t
CDDBServer::_OpenConnection()
{
	if (!fInitialized)
		return B_ERROR;

	if (fConnected)
		return B_OK;

	if (fConnection.Connect(fCddbServerAddr) == B_OK) {
		fConnected = true;
		return B_OK;
	}
	
	return B_ERROR;
}


void
CDDBServer::_CloseConnection()
{
	if (!fConnected)
		return;
		
	fConnection.Close();
	fConnected = false;
}


status_t
CDDBServer::_SendCddbCommand(const BString& command, BString* output)
{
	if (!fConnected)
		return B_ERROR;

	// Assemble full command string.
	BString fullCommand;
	fullCommand << command << "&hello=" << fLocalUserName << " " <<
		fLocalHostName << " cddb_daemon 1.0&proto=6";

	// Replace spaces by + signs.
	fullCommand.ReplaceAll(" ", "+");

	// And now add command header and footer. 
	fullCommand.Prepend("GET /~cddb/cddb.cgi?cmd=");
	fullCommand << " HTTP 1.0\n\n";
	
	int32 result = fConnection.Send((void*)fullCommand.String(),
		fullCommand.Length());
	if (result == fullCommand.Length()) {
		BNetBuffer netBuffer;
		while (fConnection.Receive(netBuffer, 1024) != 0) {
			// Do nothing. Data is automatically appended to the NetBuffer.
		}
		
		// AppendString automatically adds the terminating \0.
		netBuffer.AppendString("");
	
		output->SetTo((char*)netBuffer.Data(), netBuffer.Size());
		return B_OK;
	}

	return B_ERROR;
}
