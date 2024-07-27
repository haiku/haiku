/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * solvversion.h
 * 
 */

#ifndef LIBSOLV_SOLVVERSION_H
#define LIBSOLV_SOLVVERSION_H

#define LIBSOLV_VERSION_STRING "0.3.0"
#define LIBSOLV_VERSION_MAJOR 0
#define LIBSOLV_VERSION_MINOR 3
#define LIBSOLV_VERSION_PATCH 0
#define LIBSOLV_VERSION (LIBSOLV_VERSION_MAJOR * 10000 + LIBSOLV_VERSION_MINOR * 100 + LIBSOLV_VERSION_PATCH)

extern const char solv_version[];
extern int solv_version_major;
extern int solv_version_minor;
extern int solv_version_patch;

#endif
