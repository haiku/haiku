/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GOOGLEFS_H
#define _GOOGLEFS_H

/* wrappers */
#ifdef __HAIKU__

#include <fs_interface.h>
#include <kernel/lock.h>
#include <fs_info.h>
#include <NodeMonitor.h>
#define lock mutex
#define new_lock mutex_init
#define free_lock mutex_destroy
#define LOCK mutex_lock
#define UNLOCK mutex_unlock
typedef dev_t nspace_id;

#else

#include "fsproto.h"
#include "lock.h"
#define publish_vnode new_vnode

#endif

#include "lists2.h"

#include <Errors.h>
#include <sys/stat.h>
#include <SupportDefs.h>

/* should be more than enough */
//#define GOOGLEFS_NAME_LEN B_OS_NAME_LENGTH
//#define GOOGLEFS_NAME_LEN 64 /* GR_MAX_NAME */
/* some google results are a bit more than 64 */
#define GOOGLEFS_NAME_LEN 70 /* GR_MAX_NAME */

#define GOOGLEFS_NAME "googlefs"
#define GOOGLEFS_PRETTY_NAME "Google Bookmark File System"

#define MAX_VNIDS 5000


struct attr_entry {
	const char *name;
	uint32 type;
	size_t size;
	void *value;
};

#undef ASSERT
#define ASSERT(op) if (!(op)) panic("ASSERT: %s in %s:%s", #op, __FILE__, __FUNCTION__)

struct mount_fs_params
{
	/* no param */
};

struct fs_file_cookie;

struct fs_node
{
	struct fs_node *nlnext; /* node list */
	struct fs_node *qnext; /* query list */
	struct fs_node *next; /* next in folder */
	struct fs_node *parent, *children;
	struct fs_file_cookie *opened; /* opened on this node */
	
	char name[GOOGLEFS_NAME_LEN];
	ino_t vnid;
	lock l;
	
	int is_perm:1;	/* don't delete on last close */
	int deleted:1;
	int qcompleted:1;
	int hidden:1;	/* don't list in readdir */
	struct stat st; /* including S_IFDIR in st_mode */
	struct google_request *request; /* set on root folder for a query */
	struct google_result *result; /* for query results */
	struct attr_entry *attrs_indirect;
	struct attr_entry attrs[10];
	void *data;
	size_t data_size;
};

struct vnidpool;

struct fs_nspace
{
	nspace_id nsid;
	ino_t rootid;
	struct vnidpool *vnids;
	struct fs_node *root; /* fast access for stat time change */
	struct fs_node *nodes;
	struct fs_node *queries;
	int32 nodecount; /* just for statvfs() */
	
	lock l;
};

struct fs_file_cookie
{
	struct fs_file_cookie *next; /* must stay here */
	struct fs_node *node;
	int dir_current; /* current entry for readdir() */
	int omode;
	int type; /* S_IF* of the *cookie* */
	struct attr_entry *attr;
};
/* just for type */
#define     S_IFQUERY                     00000070000

typedef struct attr_entry attr_entry;
typedef struct fs_nspace fs_nspace;
typedef struct fs_node fs_node;
typedef struct fs_file_cookie fs_file_cookie;
/* not much different */
#define fs_dir_cookie fs_file_cookie
#define fs_attr_dir_cookie fs_file_cookie
#define fs_query_cookie fs_file_cookie

ino_t new_vnid(fs_nspace *ns);

int googlefs_event(fs_nspace *ns, fs_node *node, int flags);

/* used by the requester to publish entries in queries
 * result = NULL to notify end of list
 */
extern int googlefs_push_result_to_query(struct google_request *request, struct google_result *result);

#endif
