/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#include "fuse_config.h"


enum  {
	KEY_HELP,
};

#define FUSE_LIB_OPT(t, p, v) { t, offsetof(struct fuse_config, p), v }

static const struct fuse_opt fuse_lib_opts[] = {
	FUSE_OPT_KEY("-h",		      KEY_HELP),
	FUSE_OPT_KEY("--help",		      KEY_HELP),
	FUSE_OPT_KEY("debug",		      FUSE_OPT_KEY_KEEP),
	FUSE_OPT_KEY("-d",		      FUSE_OPT_KEY_KEEP),
	FUSE_LIB_OPT("debug",		      debug, 1),
	FUSE_LIB_OPT("-d",		      debug, 1),
	FUSE_LIB_OPT("hard_remove",	      hard_remove, 1),
	FUSE_LIB_OPT("use_ino",		      use_ino, 1),
	FUSE_LIB_OPT("readdir_ino",	      readdir_ino, 1),
	FUSE_LIB_OPT("direct_io",	      direct_io, 1),
	FUSE_LIB_OPT("kernel_cache",	      kernel_cache, 1),
	FUSE_LIB_OPT("auto_cache",	      auto_cache, 1),
	FUSE_LIB_OPT("noauto_cache",	      auto_cache, 0),
	FUSE_LIB_OPT("umask=",		      set_mode, 1),
	FUSE_LIB_OPT("umask=%o",	      umask, 0),
	FUSE_LIB_OPT("uid=",		      set_uid, 1),
	FUSE_LIB_OPT("uid=%d",		      uid, 0),
	FUSE_LIB_OPT("gid=",		      set_gid, 1),
	FUSE_LIB_OPT("gid=%d",		      gid, 0),
	FUSE_LIB_OPT("entry_timeout=%lf",     entry_timeout, 0),
	FUSE_LIB_OPT("attr_timeout=%lf",      attr_timeout, 0),
	FUSE_LIB_OPT("ac_attr_timeout=%lf",   ac_attr_timeout, 0),
	FUSE_LIB_OPT("ac_attr_timeout=",      ac_attr_timeout_set, 1),
	FUSE_LIB_OPT("negative_timeout=%lf",  negative_timeout, 0),
	FUSE_LIB_OPT("intr",		      intr, 1),
	FUSE_LIB_OPT("intr_signal=%d",	      intr_signal, 0),
	FUSE_LIB_OPT("modules=%s",	      modules, 0),
	FUSE_OPT_END
};


static int
fuse_lib_opt_proc(void *data, const char *arg, int key,
	struct fuse_args *outargs)
{
	(void)data;
	(void)arg;
	(void)key;
	(void)outargs;

	return 1;
}


int
fuse_parse_config_args(struct fuse_args* args, struct fuse_config* config)
{
	return fuse_opt_parse(args, config, fuse_lib_opts, fuse_lib_opt_proc) == 0;
}
