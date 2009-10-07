/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MOUNT_SERVER_H
#define _MOUNT_SERVER_H

#include <SupportDefs.h>


const uint32 kMountVolume 			= 'mntv';
const uint32 kMountAllNow			= 'mntn';
const uint32 kSetAutomounterParams 	= 'pmst';
const uint32 kGetAutomounterParams 	= 'gpms';
const uint32 kVolumeMounted			= 'vmtd';
const uint32 kUnmountVolume			= 'umnt';

#define kMountServerSignature "application/x-vnd.Haiku-mount_server"


#endif // _MOUNT_SERVER_H
