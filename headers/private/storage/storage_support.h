//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file storage_support.h
	Interface declarations for miscellaneous internal
	Storage Kit support functions.
*/

#ifndef _STORAGE_SUPPORT_H
#define _STORAGE_SUPPORT_H

#include <string>

namespace BPrivate {
namespace Storage {

//! Returns whether the supplied path is absolute.
bool is_absolute_path(const char *path);

//!	splits a path name into directory path and leaf name
status_t split_path(const char *fullPath, char *&path, char *&leaf);

//!	splits a path name into directory path and leaf name
status_t split_path(const char *fullPath, char **path, char **leaf);

//! Parses the first component of a path name.
status_t parse_first_path_component(const char *path, int32& length,
									int32& nextComponent);

//! Parses the first component of a path name.
status_t parse_first_path_component(const char *path, char *&component,
									int32& nextComponent);

//! Checks whether an entry name is a valid entry name.
status_t check_entry_name(const char *entry);

//! Checks whether a path name is a valid path name.
status_t check_path_name(const char *path);

//! Returns a copy of \c str in which all alphabetic characters are lowercase.
std::string to_lower(const char *str);

//! Places a copy of \c str in \c result in which all alphabetic characters are lowercase.
void to_lower(const char *str, std::string &result);

//! Copies \c str into \c result, converting any uppercase alphabetics to lowercase.
void to_lower(const char *str, char *result);

//! Converts \c str to lowercase.
void to_lower(char *str);

};	// namespace Storage
};	// namespace BPrivate

#endif	// _STORAGE_SUPPORT_H


