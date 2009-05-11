/*
 * Copyright 2008-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */
 
#ifndef _CDDB_SERVER_H
#define _CDDB_SERVER_H

#include <List.h>
#include <NetAddress.h>
#include <NetEndpoint.h>
#include <String.h>

#include <scsi_cmds.h>


// CD track specific data.
struct TrackData {
	uint32 trackNumber;
	BString title;
	BString artist;
};


// Query command response.
struct QueryResponseData {
	BString category;
	BString cddbId;
	BString artist;
	BString title;
};


// Read command response.
struct ReadResponseData {
	BString title;
	BString artist;
	BString genre;
	uint32 year;
	BList tracks; // TrackData items.
};


class CDDBServer {
public:
				CDDBServer(const BString& cddbServer);

	// CDDB commands interface.
	status_t	Query(uint32 cddbId, const scsi_toc_toc* toc,
		BList* queryResponse);
	status_t	Read(QueryResponseData* diskData,
		ReadResponseData* readResponse);

private:
	status_t 		_ParseAddress(const BString& cddbServer);

	status_t		_OpenConnection();
	void			_CloseConnection();
	
	status_t		_SendCddbCommand(const BString& command, BString* output);

	BString			fLocalHostName;
	BString			fLocalUserName;
	BNetAddress		fCddbServerAddr;
	BNetEndpoint	fConnection;
	bool			fInitialized;
	bool			fConnected;
};

#endif  // _CDDB_SERVER_H
