/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
 * netconfig.h
 * Copyright (c) 1995 Be, Inc.	All Rights Reserved 
 *
 * Stuff for reading info out of the /boot/system/netconfig file
 */
#ifndef _NETCONFIG_H
#define _NETCONFIG_H

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <net_settings.h>

#define NC_MAXCLASSLEN	256
#define NC_MAXNAMELEN	256
#define NC_MAXVALLEN	256


static const char NETCONFIGFILE[] =  "/network";
static const char NETCONFIGFILE_TMP[] =  "/network.tmp";


typedef struct _netconfig {
	FILE *ncfile;
	char ncbuf[1020];
} NETCONFIG;
	
typedef char nc_data_t[NC_MAXVALLEN];

typedef struct nc_private {
	nc_data_t key;
	nc_data_t value;
} nc_private_t;

_IMPEXP_NET NETCONFIG *_netconfig_open(const char *heading);
_IMPEXP_NET char *_netconfig_find(const char *heading, const char *name, char *value,
					 int nbytes);
_IMPEXP_NET int _netconfig_set(const char *heading, const char *name, const char *value);


_IMPEXP_NET net_settings *_net_settings_open(const char *fname);

_IMPEXP_NET void _net_settings_makedefault(net_settings *ncw);
_IMPEXP_NET int _net_settings_save(net_settings *ncw);
_IMPEXP_NET int _net_settings_save_as(net_settings *ncw, const char *fname);
_IMPEXP_NET void _net_settings_close(net_settings *ncw);

_IMPEXP_NET int _netconfig_getnext(
								   NETCONFIG *netconfig,
								   char **name,
								   char **value
								   );

_IMPEXP_NET void _netconfig_close(NETCONFIG *netconfig);


#define net_settings_isdirty(ncw) (ncw)->_dirty
#define net_settings_makedirty(ncw, mkdirty) (ncw)->_dirty = mkdirty
#define net_settings_makedefault _net_settings_makedefault
#define netconfig_open _netconfig_open
#define net_settings_open _net_settings_open
#define netconfig_find _netconfig_find
#define netconfig_set _netconfig_set
#define net_settings_save _net_settings_save
#define net_settings_save_as _net_settings_save_as
#define net_settings_close _net_settings_close
#define netconfig_getnext _netconfig_getnext
#define netconfig_close _netconfig_close

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* _NETCONFIG_H */
