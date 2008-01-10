/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	Interface between ide and scsi bus manager
*/


#ifndef __IDE_SIM_H__
#define __IDE_SIM_H__

#include "scsi_cmds.h"

extern scsi_for_sim_interface *scsi;
extern scsi_sim_interface ide_sim_module;

#endif
