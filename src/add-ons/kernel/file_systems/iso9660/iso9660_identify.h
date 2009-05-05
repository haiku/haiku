/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Tyler Dauwalder.
 *
 * This file may be used under the terms of the MIT License.
 */
#ifndef ISO9660_IDENTIFY_H
#define ISO9660_IDENTIFY_H


#if FS_SHELL
#	include "fssh_api_wrapper.h"
#else
#	include <SupportDefs.h>
#endif


/*! \brief Contains all the info of interest pertaining to an
	iso9660 volume.

	Currently supported character set encoding styles (in decreasing
	order of precedence):
	- Joliet (UCS-12 (16-bit unicode), which is converted to UTF-8)
	- iso9660 (some absurdly tiny character set, but we actually allow UTF-8)
*/
struct iso9660_info {
	iso9660_info();
	~iso9660_info();

	bool IsValid();

	void SetISO9660Name(const char *name, uint32 length);
	void SetJolietName(const char *name, uint32 length);

	const char* PreferredName();

	char *iso9660_name;
	char *joliet_name;

	off_t max_blocks;

private:
	void _SetString(char **string, const char *newString, uint32 newLength);
};

status_t iso9660_fs_identify(int deviceFD, iso9660_info *info);

#endif	// ISO9660_IDENTIFY_H

