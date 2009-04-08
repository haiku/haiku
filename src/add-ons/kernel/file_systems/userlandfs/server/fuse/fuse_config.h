/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/
#ifndef USERLAND_FS_FUSE_CONFIG_H
#define USERLAND_FS_FUSE_CONFIG_H

#include "fuse_api.h"


struct fuse_config {
	unsigned int uid;
	unsigned int gid;
	unsigned int  umask;
	double entry_timeout;
	double negative_timeout;
	double attr_timeout;
	double ac_attr_timeout;
	int ac_attr_timeout_set;
	int debug;
	int hard_remove;
	int use_ino;
	int readdir_ino;
	int set_mode;
	int set_uid;
	int set_gid;
	int direct_io;
	int kernel_cache;
	int auto_cache;
	int intr;
	int intr_signal;
	int help;
	char *modules;
};


#ifdef __cplusplus
extern "C" {
#endif

int fuse_parse_lib_config_args(struct fuse_args* args,
	struct fuse_config* config);

int fuse_parse_mount_config_args(struct fuse_args* args);

#ifdef __cplusplus
}
#endif


#endif	// USERLAND_FS_FUSE_CONFIG_H
