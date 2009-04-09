/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#include "fuse_config.h"

#include <string.h>

enum {
	KEY_KERN_FLAG,
	KEY_KERN_OPT,
	KEY_FUSERMOUNT_OPT,
	KEY_SUBTYPE_OPT,
	KEY_MTAB_OPT,
	KEY_ALLOW_ROOT,
	KEY_RO,
	KEY_HELP,
	KEY_VERSION,
};

struct mount_opts {
	int allow_other;
	int allow_root;
	int ishelp;
	int flags;
	int nonempty;
	int blkdev;
	char *fsname;
	char *subtype;
	char *subtype_opt;
	char *mtab_opts;
	char *fusermount_opts;
	char *kernel_opts;
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

#define FUSE_MOUNT_OPT(t, p) { t, offsetof(struct mount_opts, p), 1 }

static const struct fuse_opt fuse_mount_opts[] = {
	FUSE_MOUNT_OPT("allow_other",	    allow_other),
	FUSE_MOUNT_OPT("allow_root",	    allow_root),
	FUSE_MOUNT_OPT("nonempty",	    nonempty),
	FUSE_MOUNT_OPT("blkdev",	    blkdev),
	FUSE_MOUNT_OPT("fsname=%s",	    fsname),
	FUSE_MOUNT_OPT("subtype=%s",	    subtype),
	FUSE_OPT_KEY("allow_other",	    KEY_KERN_OPT),
	FUSE_OPT_KEY("allow_root",	    KEY_ALLOW_ROOT),
	FUSE_OPT_KEY("nonempty",	    KEY_FUSERMOUNT_OPT),
	FUSE_OPT_KEY("blkdev",		    KEY_FUSERMOUNT_OPT),
	FUSE_OPT_KEY("fsname=",		    KEY_FUSERMOUNT_OPT),
	FUSE_OPT_KEY("subtype=",	    KEY_SUBTYPE_OPT),
	FUSE_OPT_KEY("large_read",	    KEY_KERN_OPT),
	FUSE_OPT_KEY("blksize=",	    KEY_KERN_OPT),
	FUSE_OPT_KEY("default_permissions", KEY_KERN_OPT),
	FUSE_OPT_KEY("max_read=",	    KEY_KERN_OPT),
	FUSE_OPT_KEY("max_read=",	    FUSE_OPT_KEY_KEEP),
	FUSE_OPT_KEY("user=",		    KEY_MTAB_OPT),
	FUSE_OPT_KEY("-r",		    KEY_RO),
	FUSE_OPT_KEY("ro",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("rw",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("suid",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("nosuid",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("dev",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("nodev",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("exec",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("noexec",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("async",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("sync",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("dirsync",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("atime",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("noatime",		    KEY_KERN_FLAG),
	FUSE_OPT_KEY("-h",		    KEY_HELP),
	FUSE_OPT_KEY("--help",		    KEY_HELP),
	FUSE_OPT_KEY("-V",		    KEY_VERSION),
	FUSE_OPT_KEY("--version",	    KEY_VERSION),
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
fuse_parse_lib_config_args(struct fuse_args* args, struct fuse_config* config)
{
	return fuse_opt_parse(args, config, fuse_lib_opts, fuse_lib_opt_proc) == 0;
}


int
fuse_is_lib_option(const char* opt)
{
	return /*fuse_lowlevel_is_lib_option(opt) ||*/
		fuse_opt_match(fuse_lib_opts, opt);
}


#if 0
struct mount_flags {
	const char *opt;
	unsigned long flag;
	int on;
};

static struct mount_flags mount_flags[] = {
	{"rw",	    MS_RDONLY,	    0},
	{"ro",	    MS_RDONLY,	    1},
	{"suid",    MS_NOSUID,	    0},
	{"nosuid",  MS_NOSUID,	    1},
	{"dev",	    MS_NODEV,	    0},
	{"nodev",   MS_NODEV,	    1},
	{"exec",    MS_NOEXEC,	    0},
	{"noexec",  MS_NOEXEC,	    1},
	{"async",   MS_SYNCHRONOUS, 0},
	{"sync",    MS_SYNCHRONOUS, 1},
	{"atime",   MS_NOATIME,	    0},
	{"noatime", MS_NOATIME,	    1},
	{"dirsync", MS_DIRSYNC,	    1},
	{NULL,	    0,		    0}
};
#endif	// 0

static void set_mount_flag(const char *s, int *flags)
{
#if 0
	int i;

	for (i = 0; mount_flags[i].opt != NULL; i++) {
		const char *opt = mount_flags[i].opt;
		if (strcmp(opt, s) == 0) {
			if (mount_flags[i].on)
				*flags |= mount_flags[i].flag;
			else
				*flags &= ~mount_flags[i].flag;
			return;
		}
	}
	fprintf(stderr, "fuse: internal error, can't find mount flag\n");
	abort();
#endif
}

static int fuse_mount_opt_proc(void *data, const char *arg, int key,
			       struct fuse_args *outargs)
{
	struct mount_opts *mo = data;

	switch (key) {
	case KEY_ALLOW_ROOT:
		if (fuse_opt_add_opt(&mo->kernel_opts, "allow_other") == -1 ||
		    fuse_opt_add_arg(outargs, "-oallow_root") == -1)
			return -1;
		return 0;

	case KEY_RO:
		arg = "ro";
		/* fall through */
	case KEY_KERN_FLAG:
		set_mount_flag(arg, &mo->flags);
		return 0;

	case KEY_KERN_OPT:
		return fuse_opt_add_opt(&mo->kernel_opts, arg);

	case KEY_FUSERMOUNT_OPT:
		return fuse_opt_add_opt(&mo->fusermount_opts, arg);

	case KEY_SUBTYPE_OPT:
		return fuse_opt_add_opt(&mo->subtype_opt, arg);

	case KEY_MTAB_OPT:
		return fuse_opt_add_opt(&mo->mtab_opts, arg);

	case KEY_HELP:
//		mount_help();
		mo->ishelp = 1;
		break;

	case KEY_VERSION:
//		mount_version();
		mo->ishelp = 1;
		break;
	}
	return 1;
}


int
fuse_parse_mount_config_args(struct fuse_args* args)
{
	struct mount_opts mo;
	memset(&mo, 0, sizeof(mo));

	return args == 0
		|| fuse_opt_parse(args, &mo, fuse_mount_opts, fuse_mount_opt_proc) == 0;
}
