/*
 * Copyright 2008-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */
#ifndef _CDDB_SERVER_H
#define _CDDB_SERVER_H


#include <NetAddress.h>
#include <NetEndpoint.h>
#include <ObjectList.h>
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
	BString cddbID;
	BString artist;
	BString title;
};


// Read command response.
struct ReadResponseData {
	BString title;
	BString artist;
	BString genre;
	uint32 year;
	BObjectList<TrackData> tracks;

	ReadResponseData()
		:
		tracks(20, true)
	{
	}
};


typedef BObjectList<QueryResponseData> QueryResponseList;


class CDDBServer {
public:
								CDDBServer(const BString& cddbServer);

	// CDDB commands interface.
			status_t			Query(uint32 cddbID, const scsi_toc_toc* toc,
									QueryResponseList& queryResponses);
			status_t			Read(const QueryResponseData& diskData,
									ReadResponseData& readResponse,
									bool verbose = false);
			status_t			Read(const BString& category,
									const BString& cddbID,
									const BString& artist,
									ReadResponseData& readResponse,
									bool verbose = false);

private:
			status_t 			_ParseAddress(const BString& cddbServer);

			status_t			_OpenConnection();
			void				_CloseConnection();

			status_t			_SendCommand(const BString& command,
									BString& output);
			TrackData*			_Track(ReadResponseData& response,
									uint32 track) const;

private:
			BString				fLocalHostName;
			BString				fLocalUserName;
			BNetAddress			fServerAddress;
			BNetEndpoint		fConnection;
			bool				fInitialized;
			bool				fConnected;
};


#endif  // _CDDB_SERVER_H
