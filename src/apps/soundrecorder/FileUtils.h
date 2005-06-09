/*******************************************************************************
/
/	File:			FileUtils.h
/
/   Description:	Utility functions for copying file data and attributes.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if ! defined( _FileUtils_h )
#define _FileUtils_h

#include <File.h>

status_t CopyFile(BFile& dest, BFile& src);
status_t CopyFileData(BFile& dst, BFile& src);
status_t CopyAttributes(BNode& dst, BNode& src);

#endif /* _FileUtils_h */
