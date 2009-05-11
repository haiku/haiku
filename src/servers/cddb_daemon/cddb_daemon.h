/*
 * Copyright 2008-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */
 
#ifndef _CDDB_DAEMON_H
#define _CDDB_DAEMON_H

#include <Application.h>

#include <scsi_cmds.h>

struct ReadResponseData;
struct QueryResponseData;

class BList;
class BMessage;
class BVolumeRoster;

class CDDBDaemon : public BApplication {
public:
						CDDBDaemon();
	virtual				~CDDBDaemon();

	virtual void		MessageReceived(BMessage* message);

private:
	status_t			_Lookup(const dev_t device);
	bool				_CanLookup(const dev_t device, uint32* cddbId,
							scsi_toc_toc* toc) const;
	QueryResponseData*	_SelectResult(BList* response) const;
	status_t			_WriteCDData(dev_t device, QueryResponseData* diskData,
							ReadResponseData* readResponse);

	BVolumeRoster*		fVolumeRoster;
};

#endif  // _CDDB_DAEMON_H
