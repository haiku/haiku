/***************************************************************************
//
//	File:			Alias.h
//
//	Description:	path->alias->path functions
//
//	Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _ALIAS_H
#define _ALIAS_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <SupportDefs.h>

class	BPath;
class	BDataIO;

_IMPEXP_BE status_t resolve_link(const char *path, BPath *result,
							bool block = false);
_IMPEXP_BE status_t write_alias(const char *path, BDataIO *s,
							size_t *len = NULL);
_IMPEXP_BE status_t write_alias(const char *path, void *buf, size_t *len);

_IMPEXP_BE status_t read_alias(BDataIO *s, BPath *result, size_t *len = NULL,
						bool block = false);
_IMPEXP_BE status_t read_alias(const void *buf, BPath *result, size_t *len,
						bool block = false);

#endif
