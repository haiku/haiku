/*******************************************************************************
/
/	File:		fs_query.h
/
/	Description:	C interface to the BeOS file system query mechanism.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _FS_QUERY_H
#define _FS_QUERY_H

#include <BeBuild.h>
#include <OS.h>
#include <dirent.h>


/* flags for fs_open_query() */

#define		B_LIVE_QUERY		0x00000001


#ifdef  __cplusplus
extern "C" {
#endif


_IMPEXP_ROOT DIR   	       *fs_open_query(dev_t device, const char *query, uint32 flags);
_IMPEXP_ROOT DIR           *fs_open_live_query(dev_t device, const char *query,
								  uint32 flags, port_id port, int32 token);
_IMPEXP_ROOT int            fs_close_query(DIR *d);
_IMPEXP_ROOT struct dirent *fs_read_query(DIR *d);

_IMPEXP_ROOT status_t		get_path_for_dirent(struct dirent *dent, char *buf,
												size_t len);


#ifdef  __cplusplus
}
#endif


#endif /* _FS_QUERY_H */
