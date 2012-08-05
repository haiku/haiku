/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef OPENSTATE_H
#define OPENSTATE_H


#include <lock.h>
#include <SupportDefs.h>
#include <util/KernelReferenceable.h>

#include "NFS4Object.h"


struct OpenState : public NFS4Object, public KernelReferenceable {
							OpenState();
							~OpenState();

			uint64			fClientID;

			int				fMode;
			mutex			fLock;

			uint32			fStateID[3];
			uint32			fStateSeq;

			bool			fOpened;

			status_t		Reclaim(uint64 newClientID);

			status_t		Close();
};


#endif	// OPENSTATE_H

