//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file storage_support.h
	Interface declarations for miscellaneous internal
	Storage Kit support functions.
*/

#ifndef _sk_storage_support_h_
#define _sk_storage_support_h_


namespace StorageKit {

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

} // namespace StorageKit

#endif	// _sk_storage_support_h_
