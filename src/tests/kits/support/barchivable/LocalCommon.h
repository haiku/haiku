//------------------------------------------------------------------------------
//	LocalCommon.h
//
//------------------------------------------------------------------------------

#ifndef LOCALCOMMON_H
#define LOCALCOMMON_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#ifdef SYSTEM_TEST
#include <be/support/Archivable.h>
#else
#include "../../../../source/lib/support/headers/Archivable.h"
#endif

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
extern const char* gInvalidClassName;
extern const char* gInvalidSig;
extern const char* gLocalClassName;
extern const char* gLocalSig;
extern const char* gRemoteClassName;
extern const char* gRemoteSig;
extern const char* gValidSig;


#endif	//LOCALCOMMON_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

