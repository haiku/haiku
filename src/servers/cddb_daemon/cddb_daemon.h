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
#include <Message.h>
#include <VolumeRoster.h>

#include <scsi_cmds.h>

class CDDBDaemon : public BApplication {
public:
	CDDBDaemon();
	virtual ~CDDBDaemon();

	virtual void MessageReceived(BMessage* message);

private:
	bool CanLookup(const dev_t device, uint32* cddbId,
		scsi_toc_toc* toc) const;

	BVolumeRoster* fVolumeRoster;
};

#endif  // _CDDB_DAEMON_H
