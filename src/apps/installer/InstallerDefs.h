/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSTALLER_DEFS_H
#define INSTALLER_DEFS_H


#include <SupportDefs.h>


static const uint32 MSG_STATUS_MESSAGE = 'iSTM';
static const uint32 MSG_INSTALL_FINISHED = 'iIFN';
static const uint32 MSG_RESET = 'iRSI';
static const uint32 MSG_WRITE_BOOT_SECTOR = 'iWBS';

extern const char* const kPackagesDirectoryPath;
extern const char* const kSourcesDirectoryPath;


#endif	// INSTALLER_DEFS_H
