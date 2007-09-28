/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CDDB_H
#define CDDB_H


#include <scsi_cmds.h>


uint32 compute_cddb_disc_id(scsi_toc_toc &toc);

#endif	// CDDB_H
