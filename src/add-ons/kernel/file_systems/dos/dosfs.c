/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <KernelExport.h>
#include <Drivers.h>
#include <driver_settings.h>

#include <scsi.h>

#include <fsproto.h>
#ifndef COMPILE_IN_BEOS
#include <fs_volume.h>
#endif
#include <lock.h>
#include <cache.h>

#include "dosfs.h"
#include "attr.h"
#include "dir.h"
#include "dlist.h"
#include "fat.h"
#include "file.h"
#include "iter.h"
#include "util.h"
#include "vcache.h"

extern const char *build_time, *build_date;

/* debug levels */
int debug_attr = 0, debug_dir = 0, debug_dlist = 0, debug_dosfs = 0,
		debug_encodings = 0, debug_fat = 0, debug_file = 0,
		debug_iter = 0, debug_vcache = 0;

#define DPRINTF(a,b) if (debug_dosfs > (a)) dprintf b

CHECK_MAGIC(vnode,struct vnode,VNODE_MAGIC)
CHECK_MAGIC(nspace,struct _nspace, NSPACE_MAGIC)

static status_t get_fsinfo(nspace *vol, uint32 *free_count, uint32 *last_allocated);

#if DEBUG

int32 instances = 0;

int debug_dos(int argc, char **argv)
{
	int i;
	for (i=1;i<argc;i++) {
		nspace *vol = (nspace *)strtoul(argv[i], NULL, 0);
		if (vol == NULL)
			continue;

		kprintf("dos nspace @ %p\n", vol);
		kprintf("magic: %lx\n", vol->magic);
		kprintf("id: %lx, fd: %x, device: %s, flags %lx\n",
				vol->id, vol->fd, vol->device, vol->flags);
		kprintf("bytes/sector = %lx, sectors/cluster = %lx, reserved sectors = %lx\n",
				vol->bytes_per_sector, vol->sectors_per_cluster,
				vol->reserved_sectors);
		kprintf("%lx fats, %lx root entries, %lx total sectors, %lx sectors/fat\n",
				vol->fat_count, vol->root_entries_count, vol->total_sectors,
				vol->sectors_per_fat);
		kprintf("media descriptor %x, fsinfo sector %x, %lx clusters, %lx free\n",
				vol->media_descriptor, vol->fsinfo_sector, vol->total_clusters,
				vol->free_clusters);
		kprintf("%x-bit fat, mirrored %x, active %x\n",
				vol->fat_bits, vol->fat_mirrored, vol->active_fat);
		kprintf("root start %lx, %lx root sectors, root vnode @ %p\n",
				vol->root_start, vol->root_sectors, &(vol->root_vnode));
		kprintf("label entry %lx, label %s\n", vol->vol_entry, vol->vol_label);
		kprintf("data start %lx, last allocated %lx\n",
				vol->data_start, vol->last_allocated);
		kprintf("last fake vnid %Lx, vnid cache %lx entries @ (%p %p)\n",
				vol->vcache.cur_vnid, vol->vcache.cache_size,
				vol->vcache.by_vnid, vol->vcache.by_loc);
		kprintf("dlist entries: %lx/%lx @ %p\n",
				vol->dlist.entries, vol->dlist.allocated, vol->dlist.vnid_list);
	}
	return B_OK;
}

int debug_dvnode(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		kprintf("dvnode vnode\n");
		return B_OK;
	}

	for (i=1;i<argc;i++) {
		vnode *n = (vnode *)strtoul(argv[i], NULL, 0);
		if (!n) continue;

		kprintf("vnode @ %p", n);
#if TRACK_FILENAME
		kprintf(" (%s)", n->filename);
#endif
		kprintf("\nmagic %lx, vnid %Lx, dir vnid %Lx\n",
				n->magic, n->vnid, n->dir_vnid);
		kprintf("iteration %lx, si=%lx, ei=%lx, cluster=%lx\n",
				n->iteration, n->sindex, n->eindex, n->cluster);
		kprintf("mode %lx, size %Lx, time %lx\n",
				n->mode, n->st_size, n->st_time);
		kprintf("end cluster = %lx\n", n->end_cluster);
		if (n->mime) kprintf("mime type %s\n", n->mime);
	}

	return B_OK;
}

int debug_dc2s(int argc, char **argv)
{
	int i;
	nspace *vol;

	if (argc < 3) {
		kprintf("dc2s nspace cluster\n");
		return B_OK;
	}

	vol = (nspace *)strtoul(argv[1], NULL, 0);
	if (vol == NULL)
		return B_OK;

	for (i=2;i<argc;i++) {
		uint32 cluster = strtoul(argv[i], NULL, 0);
		kprintf("cluster %lx = block %Lx\n", cluster, vol->data_start +
				(off_t)(cluster - 2) * vol->sectors_per_cluster);
	}

	return B_OK;
}

#endif

static int lock_removable_device(int fd, bool state)
{
	return ioctl(fd, B_SCSI_PREVENT_ALLOW, &state, sizeof(state));
}

static status_t mount_fat_disk(const char *path, nspace_id nsid,
		const int flags, nspace** newVol, int fs_flags, int op_sync_mode)
{
	nspace		*vol = NULL;
	uint8		buf[512];
	int			i;
	device_geometry geo;
	status_t err;

	*newVol = NULL;
	if ((vol = (nspace*)calloc(sizeof(nspace), 1)) == NULL) {
		dprintf("dosfs error: out of memory\n");
		return ENOMEM;
	}
	
	vol->magic = NSPACE_MAGIC;
	vol->flags = B_FS_IS_PERSISTENT | B_FS_HAS_MIME;
	vol->fs_flags = fs_flags;

	// open read-only for now
	if ((err = (vol->fd = open(path, O_RDONLY))) < 0) {
		dprintf("dosfs: unable to open %s (%s)\n", path, strerror(err));
		goto error0;
	}
	
	// get device characteristics
	if (ioctl(vol->fd, B_GET_GEOMETRY, &geo) < 0) {
		struct stat st;
		if ((fstat(vol->fd, &st) >= 0) &&
		    S_ISREG(st.st_mode)) {
			/* support mounting disk images */
			geo.bytes_per_sector = 0x200;
			geo.sectors_per_track = 1;
			geo.cylinder_count = st.st_size / 0x200;
			geo.head_count = 1;
			geo.read_only = !(st.st_mode & S_IWUSR);
			geo.removable = true;
		} else {
			dprintf("dosfs: error getting device geometry\n");
			goto error0;
		}
	}
	if ((geo.bytes_per_sector != 0x200) && (geo.bytes_per_sector != 0x400) && (geo.bytes_per_sector != 0x800)) {
		dprintf("dosfs: unsupported device block size (%lu)\n", geo.bytes_per_sector);
		goto error0;
	}
	if (geo.removable) {
		DPRINTF(0, ("%s is removable\n", path));
		vol->flags |= B_FS_IS_REMOVABLE;
	}
	if (geo.read_only || (flags & B_MOUNT_READ_ONLY)) {
		DPRINTF(0, ("%s is read-only\n", path));
		vol->flags |= B_FS_IS_READONLY;
	} else {
		// reopen it with read/write permissions
		close(vol->fd);
		if ((err = (vol->fd = open(path, O_RDWR))) < 0) {
			dprintf("dosfs: unable to open %s (%s)\n", path, strerror(err));
			goto error0;
		}
		if ((vol->flags & B_FS_IS_REMOVABLE) && (vol->fs_flags & FS_FLAGS_LOCK_DOOR))
			lock_removable_device(vol->fd, true);
	}

	// see if we need to go into op sync mode
	vol->fs_flags &= ~FS_FLAGS_OP_SYNC;
	switch(op_sync_mode) {
		case 1:
			if((vol->flags & B_FS_IS_REMOVABLE) == 0) {
				// we're not removable, so skip op_sync
				break;
			}
		case 2:
			dprintf("dosfs: mounted with op_sync enabled\n");
			vol->fs_flags |= FS_FLAGS_OP_SYNC;
			break;
		case 0:
		default:
			;
	}

	// read in the boot sector
	if ((err = read_pos(vol->fd, 0, (void*)buf, 512)) != 512) {
		dprintf("dosfs: error reading boot sector\n");
		goto error;
	}

	// only check boot signature on hard disks to account for broken mtools
	// behavior
	if (((buf[0x1fe] != 0x55) || (buf[0x1ff] != 0xaa)) && (buf[0x15] == 0xf8))
		goto error;

	if (!memcmp(buf+3, "NTFS    ", 8) || !memcmp(buf+3, "HPFS    ", 8)) {
		dprintf("%4.4s, not FAT\n", buf+3);
		goto error;
	}

	// first fill in the universal fields from the bpb
	vol->bytes_per_sector = read16(buf,0xb);
	if ((vol->bytes_per_sector != 0x200) && (vol->bytes_per_sector != 0x400) && (vol->bytes_per_sector != 0x800)) {
		dprintf("dosfs error: unsupported bytes per sector (%lu)\n",
				vol->bytes_per_sector);
		goto error;
	}
	
	vol->sectors_per_cluster = i = buf[0xd];
	if ((i != 1) && (i != 2) && (i != 4) && (i != 8) && 
		(i != 0x10) && (i != 0x20) && (i != 0x40) && (i != 0x80)) {
		dprintf("dosfs: sectors/cluster = %d\n", i);
		goto error;
	}

	vol->reserved_sectors = read16(buf,0xe);

	vol->fat_count = buf[0x10];
	if ((vol->fat_count == 0) || (vol->fat_count > 8)) {
		dprintf("dosfs: unreasonable fat count (%lu)\n", vol->fat_count);
		goto error;
	}

	vol->media_descriptor = buf[0x15];
	// check media descriptor versus known types
	if ((buf[0x15] != 0xF0) && (buf[0x15] < 0xf8)) {
		dprintf("dosfs error: invalid media descriptor byte\n");
		goto error;
	}
	
	vol->vol_entry = -2;	// for now, assume there is no volume entry
	memset(vol->vol_label, ' ', 11);

	// now become more discerning citizens
	vol->sectors_per_fat = read16(buf,0x16);
	if (vol->sectors_per_fat == 0) {
		// fat32 land
		vol->fat_bits = 32;
		vol->sectors_per_fat = read32(buf,0x24);
		vol->total_sectors = read32(buf,0x20);
		
		vol->fsinfo_sector = read16(buf, 0x30);
		if ((vol->fsinfo_sector != 0xffff) && (vol->fsinfo_sector >= vol->reserved_sectors)) {
			dprintf("dosfs: fsinfo sector too large (%x)\n", vol->fsinfo_sector);
			goto error;
		}

		vol->fat_mirrored = !(buf[0x28] & 0x80);
		vol->active_fat = (vol->fat_mirrored) ? (buf[0x28] & 0xf) : 0;

		vol->data_start = vol->reserved_sectors + vol->fat_count*vol->sectors_per_fat;
		vol->total_clusters = (vol->total_sectors - vol->data_start) / vol->sectors_per_cluster;

		vol->root_vnode.cluster = read32(buf,0x2c);
		if (vol->root_vnode.cluster >= vol->total_clusters) {
			dprintf("dosfs: root vnode cluster too large (%lx)\n", vol->root_vnode.cluster);
			goto error;
		}
	} else {
		// fat12 & fat16
		if (vol->fat_count != 2) {
			dprintf("dosfs error: claims %ld fat tables\n", vol->fat_count);
			goto error;
		}

		vol->root_entries_count = read16(buf,0x11);
		if (vol->root_entries_count % (vol->bytes_per_sector / 0x20)) {
			dprintf("dosfs error: invalid number of root entries\n");
			goto error;
		}

		vol->fsinfo_sector = 0xffff;
		vol->total_sectors = read16(buf,0x13); // partition size
		if (vol->total_sectors == 0)
			vol->total_sectors = read32(buf,0x20);

		{
			/*
			  Zip disks that were formatted at iomega have an incorrect number
			  of sectors.  They say that they have 196576 sectors but they
			  really only have 196192.  This check is a work-around for their
			  brain-deadness.
			*/  
			unsigned char bogus_zip_data[] = {
				0x00, 0x02, 0x04, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00,
				0xf8, 0xc0, 0x00, 0x20, 0x00, 0x40, 0x00, 0x20, 0x00, 0x00
			};
			
			if (memcmp(buf+0x0b, bogus_zip_data, sizeof(bogus_zip_data)) == 0 &&
				vol->total_sectors == 196576 &&
				((off_t)geo.sectors_per_track *
				 (off_t)geo.cylinder_count *
				 (off_t)geo.head_count) == 196192) {

				vol->total_sectors = 196192;
			}
		}


		if (buf[0x26] == 0x29) {
			// fill in the volume label
			if (memcmp(buf+0x2b, "           ", 11)) {
				memcpy(vol->vol_label, buf+0x2b, 11);
				vol->vol_entry = -1;
			}
		}

		vol->fat_mirrored = true;
		vol->active_fat = 0;

		vol->root_start = vol->reserved_sectors + vol->fat_count * vol->sectors_per_fat;
		vol->root_sectors = vol->root_entries_count * 0x20 / vol->bytes_per_sector;
		vol->root_vnode.cluster = 1;
		vol->root_vnode.end_cluster = 1;
		vol->root_vnode.st_size = vol->root_sectors * vol->bytes_per_sector;

		vol->data_start = vol->root_start + vol->root_sectors;
		vol->total_clusters = (vol->total_sectors - vol->data_start) / vol->sectors_per_cluster;	
		
		// XXX: uncertain about border cases; win32 sdk says cutoffs are at
		//      at ff6/ff7 (or fff6/fff7), but that doesn't make much sense
		if (vol->total_clusters > 0xff1)
			vol->fat_bits = 16;
		else
			vol->fat_bits = 12;
	}

	/* check that the partition is large enough to contain the file system */
	if (vol->total_sectors > geo.sectors_per_track * geo.cylinder_count *
			geo.head_count) {
		dprintf("dosfs: volume extends past end of partition\n");
		err = B_PARTITION_TOO_SMALL;
		goto error;
	}

	// perform sanity checks on the FAT
		
	// the media descriptor in active FAT should match the one in the BPB
	if ((err = read_pos(vol->fd, vol->bytes_per_sector*(vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat), (void *)buf, 0x200)) != 0x200) {
		dprintf("dosfs: error reading FAT\n");
		goto error;
	}

	if (buf[0] != vol->media_descriptor) {
		dprintf("dosfs error: media descriptor mismatch (%x != %x)\n", buf[0], vol->media_descriptor);
		goto error;
	}

	if (vol->fat_mirrored) {
		uint32 i;
		uint8 buf2[512];
		for (i=0;i<vol->fat_count;i++) {
			if (i != vol->active_fat) {
				DPRINTF(1, ("checking fat #%ld\n", i));
				buf2[0] = ~buf[0];
				if ((err = read_pos(vol->fd, vol->bytes_per_sector*(vol->reserved_sectors + vol->sectors_per_fat*i), (void *)buf2, 0x200)) != 0x200) {
					dprintf("dosfs: error reading FAT %ld\n", i);
					goto error;
				}

				if (buf2[0] != vol->media_descriptor) {
					dprintf("dosfs error: media descriptor mismatch in fat # %ld (%x != %x)\n", i, buf2[0], vol->media_descriptor);
					goto error;
				}
#if 0			
				// checking for exact matches of fats is too
				// restrictive; allow these to go through in
				// case the fat is corrupted for some reason
				if (memcmp(buf, buf2, 0x200)) {
					dprintf("dosfs error: fat %d doesn't match active fat (%d)\n", i, vol->active_fat);
					goto error;
				}
#endif
			}
		}
	}

	// now we are convinced of the drive's validity

	vol->id = nsid;
	strncpy(vol->device,path,256);

	// this will be updated later if fsinfo exists
	vol->last_allocated = 2;

	vol->beos_vnid = INVALID_VNID_BITS_MASK;
	{
		void *handle;
		handle = load_driver_settings("dos");
		vol->respect_disk_image =
				get_driver_boolean_parameter(handle, "respect", true, true);
		unload_driver_settings(handle);
	}

	// initialize block cache
	if (init_cache_for_device(vol->fd, (off_t)vol->total_sectors) < 0) {
		dprintf("error initializing block cache\n");
		goto error;
	}
	
	// as well as the vnode cache
	if (init_vcache(vol) != B_OK) {
		dprintf("error initializing vnode cache\n");
		goto error1;
	}

	// and the dlist cache
	if (dlist_init(vol) != B_OK) {
		dprintf("error initializing dlist cache\n");
		goto error2;
	}

	if (vol->flags & B_FS_IS_READONLY)
		vol->free_clusters = 0;
	else {
		uint32 free_count, last_allocated;
		err = get_fsinfo(vol, &free_count, &last_allocated);
		if (err >= 0) {
			if (free_count < vol->total_clusters) 
				vol->free_clusters = free_count;
			else {
				dprintf("free cluster count from fsinfo block invalid %lx\n", free_count);
				err = -1;
			}
			if (last_allocated < vol->total_clusters) 
				vol->last_allocated = last_allocated; //update to a closer match
		}
		if (err < 0) {
			if ((err = count_free_clusters(vol)) < 0) {
				dprintf("error counting free clusters (%s)\n", strerror(err));
				goto error3;
			}
			vol->free_clusters = err;
		}
	}

	DPRINTF(0, ("built at %s on %s\n", build_time, build_date));
	DPRINTF(0, ("mounting %s (id %lx, device %x, media descriptor %x)\n", vol->device, vol->id, vol->fd, vol->media_descriptor));
	DPRINTF(0, ("%lx bytes/sector, %lx sectors/cluster\n", vol->bytes_per_sector, vol->sectors_per_cluster));
	DPRINTF(0, ("%lx reserved sectors, %lx total sectors\n", vol->reserved_sectors, vol->total_sectors));
	DPRINTF(0, ("%lx %d-bit fats, %lx sectors/fat, %lx root entries\n", vol->fat_count, vol->fat_bits, vol->sectors_per_fat, vol->root_entries_count));
	DPRINTF(0, ("root directory starts at sector %lx (cluster %lx), data at sector %lx\n", vol->root_start, vol->root_vnode.cluster, vol->data_start));
	DPRINTF(0, ("%lx total clusters, %lx free\n", vol->total_clusters, vol->free_clusters));
	DPRINTF(0, ("fat mirroring is %s, fs info sector at sector %x\n", (vol->fat_mirrored) ? "on" : "off", vol->fsinfo_sector));
	DPRINTF(0, ("last allocated cluster = %lx\n", vol->last_allocated));

	if (vol->fat_bits == 32) {
		// now that the block cache has been initialised, we can figure
		// out the length of the root directory with count_clusters()
		vol->root_vnode.st_size = count_clusters(vol, vol->root_vnode.cluster) * vol->bytes_per_sector * vol->sectors_per_cluster;
		vol->root_vnode.end_cluster = get_nth_fat_entry(vol, vol->root_vnode.cluster, vol->root_vnode.st_size / vol->bytes_per_sector / vol->sectors_per_cluster - 1);
	}

	// initialize root vnode
	vol->root_vnode.magic = VNODE_MAGIC;
	vol->root_vnode.vnid = vol->root_vnode.dir_vnid = GENERATE_DIR_CLUSTER_VNID(vol->root_vnode.cluster,vol->root_vnode.cluster);
	vol->root_vnode.sindex = vol->root_vnode.eindex = 0xffffffff;
	vol->root_vnode.mode = FAT_SUBDIR;
	time(&(vol->root_vnode.st_time));
	vol->root_vnode.mime = NULL;
	vol->root_vnode.dirty = false;
	dlist_add(vol, vol->root_vnode.vnid);

	// find volume label (supercedes any label in the bpb)
	{
		struct diri diri;
		uint8 *buffer;
		buffer = diri_init(vol, vol->root_vnode.cluster, 0, &diri);
		for (;buffer;buffer=diri_next_entry(&diri)) {
			if ((buffer[0x0b] & FAT_VOLUME) && (buffer[0x0b] != 0xf) && (buffer[0] != 0xe5)) {
				vol->vol_entry = diri.current_index;
				memcpy(vol->vol_label, buffer, 11);
				break;
			}
		}
		diri_free(&diri);
	}

	DPRINTF(0, ("root vnode id = %Lx\n", vol->root_vnode.vnid));
	DPRINTF(0, ("volume label [%11.11s] (%lx)\n", vol->vol_label, vol->vol_entry));

	// steal a trick from bfs
	if (!memcmp(vol->vol_label, "__RO__     ", 11)) {
		vol->flags |= B_FS_IS_READONLY;
	}
	
	*newVol = vol;
	return B_NO_ERROR;

error3:
	dlist_uninit(vol);
error2:
	uninit_vcache(vol);
error1:
	remove_cached_device_blocks(vol->fd, NO_WRITES);
error:
	if (!(vol->flags & B_FS_IS_READONLY) && (vol->flags & B_FS_IS_REMOVABLE) && (vol->fs_flags & FS_FLAGS_LOCK_DOOR))
		lock_removable_device(vol->fd, false);
error0:
	close(vol->fd);
	free(vol);
	return (err >= B_NO_ERROR) ? EINVAL : err;
}

static int dosfs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **data, vnode_id *vnid)
{
	int	result;
	nspace	*vol;
	void *handle;
	int op_sync_mode;
	int fs_flags = 0;

	handle = load_driver_settings("dos");
		debug_attr = strtoul(get_driver_parameter(handle, "debug_attr", "0", "0"), NULL, 0);
		debug_dir = strtoul(get_driver_parameter(handle, "debug_dir", "0", "0"), NULL, 0);
		debug_dlist = strtoul(get_driver_parameter(handle, "debug_dlist", "0", "0"), NULL, 0);
		debug_dosfs = strtoul(get_driver_parameter(handle, "debug_dosfs", "0", "0"), NULL, 0);
		debug_encodings = strtoul(get_driver_parameter(handle, "debug_encodings", "0", "0"), NULL, 0);
		debug_fat = strtoul(get_driver_parameter(handle, "debug_fat", "0", "0"), NULL, 0);
		debug_file = strtoul(get_driver_parameter(handle, "debug_file", "0", "0"), NULL, 0);
		debug_iter = strtoul(get_driver_parameter(handle, "debug_iter", "0", "0"), NULL, 0);
		debug_vcache = strtoul(get_driver_parameter(handle, "debug_vcache", "0", "0"), NULL, 0);
	
		op_sync_mode = strtoul(get_driver_parameter(handle, "op_sync_mode", "0", "0"), NULL, 0);
		if (op_sync_mode < 0 || op_sync_mode > 2) {
			op_sync_mode = 0;
		}
		if (strcasecmp(get_driver_parameter(handle, "lock_device", "true", "true"), "false") == 0) {
			dprintf("dosfs: mounted with lock_device = false\n");
		} else {
			dprintf("dosfs: mounted with lock_device = true\n");
			fs_flags |= FS_FLAGS_LOCK_DOOR;
		}
		
	unload_driver_settings(handle);

	/* parms and len are command line options; dosfs doesn't use any so
	   we can ignore these arguments */
	TOUCH(parms); TOUCH(len);

#if __RO__
	// make it read-only
	flags |= 1;
#endif

	if (data == NULL) {
		dprintf("dosfs_mount passed NULL data pointer\n");
		return EINVAL;
	}

	// Try and mount volume as a FAT volume
	if ((result = mount_fat_disk(device, nsid, flags, &vol, fs_flags, op_sync_mode)) == B_NO_ERROR) {
		char name[32];

		if (check_nspace_magic(vol, "dosfs_mount")) return EINVAL;
		
		*vnid = vol->root_vnode.vnid;
		*data = (void*)vol;
		
		// You MUST do this. Create the vnode for the root.
		result = new_vnode(nsid, *vnid, (void*)&(vol->root_vnode));
		if (result != B_NO_ERROR) {
			dprintf("error creating new vnode (%s)\n", strerror(result));
			goto error;
		}
		sprintf(name, "fat lock %lx", vol->id);
		if ((result = new_lock(&(vol->vlock), name)) != 0) {
			dprintf("error creating lock (%s)\n", strerror(result));
			goto error;
		}

#if DEBUG
		load_driver_symbols("dos");

		if (atomic_add(&instances, 1) == 0) {
			add_debugger_command("dos", debug_dos, "dump a dos nspace structure");
			add_debugger_command("dvnode", debug_dvnode, "dump a dos vnode structure");
			add_debugger_command("dfvnid", debug_dfvnid, "find a vnid in the vnid cache");
			add_debugger_command("dfloc", debug_dfloc, "find a loc in the vnid cache");
			add_debugger_command("dc2s", debug_dc2s, "calculate sector for cluster");
		}
#endif
	}

	return result;

error:
	remove_cached_device_blocks(vol->fd, NO_WRITES);
	dlist_uninit(vol);
	uninit_vcache(vol);
	free(vol);
	return EINVAL;
}

static void update_fsinfo(nspace *vol)
{
	if ((vol->fat_bits == 32) && (vol->fsinfo_sector != 0xffff) &&
		((vol->flags & B_FS_IS_READONLY) == false)) {
		uchar *buffer;
		if ((buffer = (uchar *)get_block(vol->fd, vol->fsinfo_sector, vol->bytes_per_sector)) != NULL) {
			if ((read32(buffer,0) == 0x41615252) && (read32(buffer,0x1e4) == 0x61417272) && (read16(buffer,0x1fe) == 0xaa55)) {
				//number of free clusters
				buffer[0x1e8] = (vol->free_clusters & 0xff);
				buffer[0x1e9] = ((vol->free_clusters >> 8) & 0xff);
				buffer[0x1ea] = ((vol->free_clusters >> 16) & 0xff);
				buffer[0x1eb] = ((vol->free_clusters >> 24) & 0xff);
				//cluster number of most recently allocated cluster 
				buffer[0x1ec] = (vol->last_allocated & 0xff);
				buffer[0x1ed] = ((vol->last_allocated >> 8) & 0xff);
				buffer[0x1ee] = ((vol->last_allocated >> 16) & 0xff);
				buffer[0x1ef] = ((vol->last_allocated >> 24) & 0xff);
				mark_blocks_dirty(vol->fd, vol->fsinfo_sector, 1);
			} else {
				dprintf("update_fsinfo: fsinfo block has invalid magic number\n");
			}
			release_block(vol->fd, vol->fsinfo_sector);
		} else {
			dprintf("update_fsinfo: error getting fsinfo sector %x\n", vol->fsinfo_sector);
		}
	}
}

static status_t get_fsinfo(nspace *vol, uint32 *free_count, uint32 *last_allocated)
{
	uchar *buffer;
	int32 result;

	if ((vol->fat_bits != 32) || (vol->fsinfo_sector == 0xffff))
		return B_ERROR;
		
	if ((buffer = (uchar *)get_block(vol->fd, vol->fsinfo_sector, vol->bytes_per_sector)) == NULL) {
		dprintf("get_fsinfo: error getting fsinfo sector %x\n", vol->fsinfo_sector);
		return EIO;
	}
	
	if ((read32(buffer,0) == 0x41615252) && (read32(buffer,0x1e4) == 0x61417272) && (read16(buffer,0x1fe) == 0xaa55)) {
		*free_count = read32(buffer,0x1e8);
		*last_allocated = read32(buffer,0x1ec);
		result = B_OK;
	} else {
		dprintf("get_fsinfo: fsinfo block has invalid magic number\n");
		result = B_ERROR;
	}

	release_block(vol->fd, vol->fsinfo_sector);
	return result;
}

static int dosfs_unmount(void *_vol)
{
	int result = B_NO_ERROR;

	nspace* vol = (nspace*)_vol;

	LOCK_VOL(vol);
	
	if (check_nspace_magic(vol, "dosfs_unmount")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}
	
	DPRINTF(0, ("dosfs_unmount volume %lx\n", vol->id));

	update_fsinfo(vol);
	flush_device(vol->fd, 0);
	remove_cached_device_blocks(vol->fd, ALLOW_WRITES);

#if DEBUG
	if (atomic_add(&instances, -1) == 1) {
		remove_debugger_command("dos", debug_dos);
		remove_debugger_command("dvnode", debug_dvnode);
		remove_debugger_command("dfvnid", debug_dfvnid);
		remove_debugger_command("dfloc", debug_dfloc);
		remove_debugger_command("dc2s", debug_dc2s);
	}
#endif

	dlist_uninit(vol);
	uninit_vcache(vol);

	if (!(vol->flags & B_FS_IS_READONLY) && (vol->flags & B_FS_IS_REMOVABLE) && (vol->fs_flags & FS_FLAGS_LOCK_DOOR))
		lock_removable_device(vol->fd, false);
	result = close(vol->fd);
	free_lock(&(vol->vlock));
	vol->magic = ~VNODE_MAGIC;
	free(vol);

#if USE_DMALLOC
	check_mem();
#endif

	return result;
}

// dosfs_rfsstat - Fill in fs_info struct for device.
static int dosfs_rfsstat(void *_vol, struct fs_info * fss)
{
	nspace* vol = (nspace*)_vol;
	int i;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_rfsstat")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(1, ("dosfs_rfsstat called\n"));

	// fss->dev and fss->root filled in by kernel
	
	// File system flags.
	fss->flags = vol->flags;
	
	// FS block size.
	fss->block_size = vol->bytes_per_sector * vol->sectors_per_cluster;

	// IO size - specifies buffer size for file copying
	fss->io_size = 65536;
	
	// Total blocks
	fss->total_blocks = vol->total_clusters;

	// Free blocks
	fss->free_blocks = vol->free_clusters;

	// Device name.
	strncpy(fss->device_name, vol->device, sizeof(fss->device_name));

	if (vol->vol_entry > -2)
		strncpy(fss->volume_name, vol->vol_label, sizeof(fss->volume_name));
	else
		strcpy(fss->volume_name, "no name    ");

	// XXX: should sanitize name as well
	for (i=10;i>0;i--)
		if (fss->volume_name[i] != ' ')
			break;
	fss->volume_name[i+1] = 0;
	for (;i>=0;i--) {
		if ((fss->volume_name[i] >= 'A') && (fss->volume_name[i] <= 'Z'))
			fss->volume_name[i] += 'a' - 'A';
	}

	// File system name
	strcpy(fss->fsh_name, "fat");
	
	UNLOCK_VOL(vol);

	return 0;
}

static int dosfs_wfsstat(void *_vol, struct fs_info * fss, long mask)
{
	int result = B_ERROR;
	nspace* vol = (nspace*)_vol;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_wfsstat")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_wfsstat called\n"));

	/* if it's a r/o file system and not the special hack, then don't allow
	 * volume renaming */
	if ((vol->flags & B_FS_IS_READONLY) && memcmp(vol->vol_label, "__RO__     ", 11)) {
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (mask & WFSSTAT_NAME) {
		// sanitize name
		char name[11];
		int i,j;
		memset(name, ' ', 11);
		DPRINTF(1, ("wfsstat: setting name to %s\n", fss->volume_name));
		for (i=j=0;(i<11)&&(fss->volume_name[j]);j++) {
			static char acceptable[] = "!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
			char c = fss->volume_name[j];
			if ((c >= 'a') && (c <= 'z')) c += 'A' - 'a';
			// spaces acceptable in volume names
			if (strchr(acceptable, c) || (c == ' '))
				name[i++] = c;
		}
		if (i == 0) { // bad name, kiddo
			result = EINVAL;
			goto bi;
		}
		DPRINTF(1, ("wfsstat: sanitized to [%11.11s]\n", name));

		if (vol->vol_entry == -1) {
			// stored in the bpb
			uchar *buffer;
			if ((buffer = get_block(vol->fd, 0, vol->bytes_per_sector)) == NULL) {
				result = EIO;
				goto bi;
			}
			if ((buffer[0x26] != 0x29) || memcmp(buffer + 0x2b, vol->vol_label, 11)) {
				dprintf("dosfs_wfsstat: label mismatch\n");
				result = B_ERROR;
			} else {
				memcpy(buffer + 0x2b, name, 11);
				mark_blocks_dirty(vol->fd, 0, 1);
				result = 0;
			}
			release_block(vol->fd, 0);
		} else if (vol->vol_entry >= 0) {
			struct diri diri;
			uint8 *buffer;
			buffer = diri_init(vol, vol->root_vnode.cluster, vol->vol_entry, &diri);

			// check if it is the same as the old volume label
			if ((buffer == NULL) || (memcmp(buffer, vol->vol_label, 11))) {
				dprintf("dosfs_wfsstat: label mismatch\n");
				diri_free(&diri);
				result = B_ERROR;
				goto bi;
			}
			memcpy(buffer, name, 11);
			diri_mark_dirty(&diri);
			diri_free(&diri);
			result = 0;
		} else {
			uint32 index;
			result = create_volume_label(vol, name, &index);
			if (result == B_OK) vol->vol_entry = index;
		}

		if (result == 0)
			memcpy(vol->vol_label, name, 11);
	}
	
	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);
	
bi:	UNLOCK_VOL(vol);

	return result;
}

static int dosfs_ioctl(void *_vol, void *_node, void *cookie, int code, 
	void *buf, size_t len)
{
	status_t result = B_OK;
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	TOUCH(cookie); TOUCH(buf); TOUCH(len);

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_ioctl") ||
	    check_vnode_magic(node, "dosfs_ioctl")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	switch (code) {
		case 10002 : /* return real creation time */
				if (buf) *(bigtime_t *)buf = node->st_time;
				break;
		case 10003 : /* return real last modification time */
				if (buf) *(bigtime_t *)buf = node->st_time;
				break;

		case 69666 :
				result = fragment(vol, buf);
				break;

		case 100000 :
			dprintf("built at %s on %s\n", build_time, build_date);
			dprintf("vol info: %s (device %x, media descriptor %x)\n", vol->device, vol->fd, vol->media_descriptor);
			dprintf("%lx bytes/sector, %lx sectors/cluster\n", vol->bytes_per_sector, vol->sectors_per_cluster);
			dprintf("%lx reserved sectors, %lx total sectors\n", vol->reserved_sectors, vol->total_sectors);
			dprintf("%lx %d-bit fats, %lx sectors/fat, %lx root entries\n", vol->fat_count, vol->fat_bits, vol->sectors_per_fat, vol->root_entries_count);
			dprintf("root directory starts at sector %lx (cluster %lx), data at sector %lx\n", vol->root_start, vol->root_vnode.cluster, vol->data_start);
			dprintf("%lx total clusters, %lx free\n", vol->total_clusters, vol->free_clusters);
			dprintf("fat mirroring is %s, fs info sector at sector %x\n", (vol->fat_mirrored) ? "on" : "off", vol->fsinfo_sector);
			dprintf("last allocated cluster = %lx\n", vol->last_allocated);
			dprintf("root vnode id = %Lx\n", vol->root_vnode.vnid);
			dprintf("volume label [%11.11s]\n", vol->vol_label);
			break;
			
		case 100001 :
			dprintf("vnode id %Lx, dir vnid = %Lx\n", node->vnid, node->dir_vnid);
			dprintf("si = %lx, ei = %lx\n", node->sindex, node->eindex);
			dprintf("cluster = %lx (%lx), mode = %lx, size = %Lx\n", node->cluster, vol->data_start + vol->sectors_per_cluster * (node->cluster - 2), node->mode, node->st_size);
			dprintf("mime = %s\n", node->mime ? node->mime : "(null)");
			dump_fat_chain(vol, node->cluster);
			break;

		case 100002 :
			{struct diri diri;
			uint8 *buffer;
			uint32 i;
			for (i=0,buffer=diri_init(vol,node->cluster, 0, &diri);buffer;buffer=diri_next_entry(&diri),i++) {
				if (buffer[0] == 0) break;
				dprintf("entry %lx:\n", i);
				dump_directory(buffer);
			}
			diri_free(&diri);}
			break;

		case 100003 :
			dprintf("vcache validation not yet implemented\n");
#if 0
			dprintf("validating vcache for %lx\n", vol->id);
			validate_vcache(vol);
			dprintf("validation complete for %lx\n", vol->id);
#endif
			break;

		case 100004 :
			dprintf("dumping vcache for %lx\n", vol->id);
			dump_vcache(vol);
			break;

		case 100005 :
			dprintf("dumping dlist for %lx\n", vol->id);
			dlist_dump(vol);
			break;

		default :
			dprintf("dosfs_ioctl: vol %lx, vnode %Lx code = %d\n", vol->id, node->vnid, code);
			result = EINVAL;
			break;
	}

	UNLOCK_VOL(vol);

	return result;
}

int _dosfs_sync(nspace *vol)
{
	if (check_nspace_magic(vol, "dosfs_sync"))
		return EINVAL;
	
	update_fsinfo(vol);
	flush_device(vol->fd, 0);

	return 0;
}

static int dosfs_sync(void *_vol)
{
	nspace *vol = (nspace *)_vol;
	int err;

	DPRINTF(0, ("dosfs_sync called on volume %lx\n", vol->id));
	
	LOCK_VOL(vol);
	err = _dosfs_sync(vol);
	UNLOCK_VOL(vol);

	return err;
}

static int dosfs_fsync(void *vol, void *node)
{
	TOUCH(node);

	return dosfs_sync(vol);
}

vnode_ops fs_entry =  {
	&dosfs_read_vnode,
	&dosfs_write_vnode,
	&dosfs_remove_vnode,
	NULL,
	&dosfs_walk,
	&dosfs_access,
	&dosfs_create,
	&dosfs_mkdir,
	NULL,
	NULL,
	&dosfs_rename,
	&dosfs_unlink,
	&dosfs_rmdir,
	&dosfs_readlink,
	&dosfs_opendir,
	&dosfs_closedir,
	&dosfs_free_dircookie,
	&dosfs_rewinddir,
	&dosfs_readdir,
	&dosfs_open,
	&dosfs_close,
	&dosfs_free_cookie,
	&dosfs_read,
	&dosfs_write,
	NULL,
	NULL,
	&dosfs_ioctl,
	NULL,
	&dosfs_rstat,
	&dosfs_wstat,
	&dosfs_fsync,
	NULL,
	&dosfs_mount,
	&dosfs_unmount,
	&dosfs_sync,
	&dosfs_rfsstat,
	&dosfs_wfsstat,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&dosfs_open_attrdir,
	&dosfs_close_attrdir,
	&dosfs_free_attrcookie,
	&dosfs_rewind_attrdir,
	&dosfs_read_attrdir,
	&dosfs_write_attr,
	&dosfs_read_attr,
	NULL,
	NULL,
	&dosfs_stat_attr,
	NULL,
	NULL,
	NULL,
	NULL
};

int32	api_version = B_CUR_FS_API_VERSION;
