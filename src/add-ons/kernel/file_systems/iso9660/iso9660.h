/* 
 * Copyright 2002, Tyler Dauwalder.
 * This file may be used under the terms of the MIT License.
 */

#ifndef _ISO9660_H
#define _ISO9660_H

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

	bool is_valid();

	void set_iso9660_volume_name(const char *name, uint32 length);
	void set_joliet_volume_name(const char *name, uint32 length);

	const char* get_preferred_volume_name();

	char *iso9660_volume_name;
	char *joliet_volume_name;
	
	off_t maxBlocks;

	void set_string(char **string, const char *new_string, uint32 new_length);
};

status_t iso9660_fs_identify(int deviceFD, iso9660_info *info);

#endif

