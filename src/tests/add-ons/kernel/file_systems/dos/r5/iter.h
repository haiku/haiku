/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_ITER_H_
#define _DOSFS_ITER_H_

struct _nspace;

/* csi keeps track of current cluster and sector info */
struct csi
{
	struct _nspace *vol;
	uint32	cluster;
	uint32	sector;
};

int init_csi(struct _nspace *vol, uint32 cluster, uint32 sector, struct csi *csi);
int iter_csi(struct csi *csi, int sectors);
uint8 *csi_get_block(struct csi *csi);
status_t csi_release_block(struct csi *csi);
status_t csi_mark_block_dirty(struct csi *csi);
status_t csi_read_blocks(struct csi *csi, uint8 *buffer, ssize_t len);
status_t csi_write_blocks(struct csi *csi, uint8 *buffer, ssize_t len);
status_t csi_write_block(struct csi *csi, uint8 *buffer);

/* directory entry iterator */
#define DIRI_MAGIC '!duM'
struct diri
{
	uint32	magic;
	struct csi csi;
	uint32 starting_cluster;
	uint32 current_index;
	uint8 *current_block;
};

uint8 *diri_init(struct _nspace *vol, uint32 cluster, uint32 index, struct diri *diri);
int diri_free(struct diri *diri);
uint8 *diri_current_entry(struct diri *diri);
uint8 *diri_next_entry(struct diri *diri);
uint8 *diri_rewind(struct diri *diri);
void diri_mark_dirty(struct diri *diri);

int check_diri_magic(struct diri *t, char *funcname);

#endif
