/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/


#include "dosfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <KernelExport.h>
#include <Drivers.h>
#include <driver_settings.h>

#include <scsi.h>

#include <fs_info.h>
#include <fs_interface.h>
#include <fs_cache.h>
#include <fs_volume.h>

#include "attr.h"
#include "dir.h"
#include "dlist.h"
#include "fat.h"
#include "file.h"
#include "iter.h"
#include "mkdos.h"
#include "util.h"
#include "vcache.h"


extern const char *build_time, *build_date;

/* debug levels */
int debug_attr = 0, debug_dir = 0, debug_dlist = 0, debug_dosfs = 0,
		debug_encodings = 0, debug_fat = 0, debug_file = 0,
		debug_iter = 0, debug_vcache = 0;

#define DPRINTF(a,b) if (debug_dosfs > (a)) dprintf b

static status_t get_fsinfo(nspace *vol, uint32 *free_count, uint32 *last_allocated);


#if DEBUG

int32 instances = 0;

static int
debug_fat_nspace(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		nspace *vol = (nspace *)strtoul(argv[i], NULL, 0);
		if (vol == NULL)
			continue;

		kprintf("fat nspace @ %p\n", vol);
		kprintf("id: %" B_PRIdDEV ", fd: %d, device: %s, flags %" B_PRIu32 "\n",
			vol->id, vol->fd, vol->device, vol->flags);
		kprintf("bytes/sector = %" B_PRIu32 ", sectors/cluster = %" B_PRIu32
			", reserved sectors = %" B_PRIu32 "\n", vol->bytes_per_sector,
			vol->sectors_per_cluster, vol->reserved_sectors);
		kprintf("%" B_PRIu32 " fats, %" B_PRIu32 " root entries, %" B_PRIu32
			" total sectors, %" B_PRIu32 " sectors/fat\n", vol->fat_count,
			vol->root_entries_count, vol->total_sectors, vol->sectors_per_fat);
		kprintf("media descriptor %" B_PRIu8 ", fsinfo sector %" B_PRIu16
			", %" B_PRIu32 " clusters, %" B_PRIu32 " free\n",
			vol->media_descriptor, vol->fsinfo_sector, vol->total_clusters,
			vol->free_clusters);
		kprintf("%" B_PRIu8 "-bit fat, mirrored %s, active %" B_PRIu8 "\n",
			vol->fat_bits, vol->fat_mirrored ? "yes" : "no", vol->active_fat);
		kprintf("root start %" B_PRIu8 ", %" B_PRIu8
			" root sectors, root vnode @ %p\n", vol->root_start,
			vol->root_sectors, &(vol->root_vnode));
		kprintf("label entry %" B_PRIu32 ", label %s\n", vol->vol_entry,
			vol->vol_label);
		kprintf("data start %" B_PRIu32 ", last allocated %" B_PRIu32 "\n",
			vol->data_start, vol->last_allocated);
		kprintf("last fake vnid %" B_PRIdINO ", vnid cache %" B_PRIu32
			" entries @ (%p %p)\n", vol->vcache.cur_vnid,
			vol->vcache.cache_size, vol->vcache.by_vnid, vol->vcache.by_loc);
		kprintf("dlist entries: %" B_PRIu32 "/%" B_PRIu32 " @ %p\n",
			vol->dlist.entries, vol->dlist.allocated, vol->dlist.vnid_list);

		dump_vcache(vol);
		dlist_dump(vol);
	}
	return B_OK;
}


static int
debug_dvnode(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		kprintf("dvnode vnode\n");
		return B_OK;
	}

	for (i = 1; i < argc; i++) {
		vnode *n = (vnode *)strtoul(argv[i], NULL, 0);
		if (!n) continue;

		kprintf("vnode @ %p", n);
#if TRACK_FILENAME
		kprintf(" (%s)", n->filename);
#endif
		kprintf("\nvnid %" B_PRIdINO ", dir vnid %" B_PRIdINO "\n", n->vnid,
			n->dir_vnid);
		kprintf("iteration %" B_PRIu32 ", si=%" B_PRIu32 ", ei=%" B_PRIu32
			", cluster=%" B_PRIu32 "\n", n->iteration, n->sindex, n->eindex,
			n->cluster);
		kprintf("mode %#" B_PRIx32 ", size %" B_PRIdOFF ", time %" B_PRIdTIME
			", crtime %" B_PRIdTIME "\n", n->mode, n->st_size, n->st_time,
			n->st_crtim);
		kprintf("end cluster = %" B_PRIu32 "\n", n->end_cluster);
		if (n->mime) kprintf("mime type %s\n", n->mime);
	}

	return B_OK;
}


static int
debug_dc2s(int argc, char **argv)
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
		kprintf("cluster %" B_PRIu32 " = block %" B_PRIdOFF "\n", cluster,
			vol->data_start + (off_t)(cluster - 2) * vol->sectors_per_cluster);
	}

	return B_OK;
}

#endif


static void
dosfs_trim_spaces(char *label)
{
	uint8 index;
	for (index = 10; index > 0; index--) {
		if (label[index] == ' ')
			label[index] = 0;
		else
			break;
	}
}


static bool
dosfs_read_label(bool fat32, uint8 *buffer, char *label)
{
	uint8 check = fat32 ? 0x42 : 0x29;
	uint8 offset = fat32 ? 0x47 : 0x2b;

	if (buffer[check] == 0x29
		&& memcmp(buffer + offset, "           ", 11) != 0) {
		memcpy(label, buffer + offset, 11);
		dosfs_trim_spaces(label);
		return true;
	}

	return false;
}


static nspace*
volume_init(int fd, uint8* buf,
	const int flags, int fs_flags,
	device_geometry *geo)
{
	nspace *vol = NULL;
	uint8 media_buf[512];
	int i;
	status_t err;

	if ((vol = (nspace *)calloc(sizeof(nspace), 1)) == NULL) {
		dprintf("dosfs error: out of memory\n");
		return NULL;
	}

	vol->flags = flags;
	vol->fs_flags = fs_flags;
	vol->fd = fd;

	// only check boot signature on hard disks to account for broken mtools
	// behavior
	if ((buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa) && buf[0x15] == 0xf8)
		goto error;

	if (!memcmp(buf + 3, "NTFS    ", 8) || !memcmp(buf + 3, "HPFS    ", 8)) {
		dprintf("dosfs error: %4.4s, not FAT\n", buf + 3);
		goto error;
	}

	// first fill in the universal fields from the bpb
	vol->bytes_per_sector = read16(buf, 0xb);
	if (vol->bytes_per_sector != 0x200 && vol->bytes_per_sector != 0x400
		&& vol->bytes_per_sector != 0x800 && vol->bytes_per_sector != 0x1000) {
		dprintf("dosfs error: unsupported bytes per sector (%" B_PRIu32 ")\n",
			vol->bytes_per_sector);
		goto error;
	}

	vol->sectors_per_cluster = i = buf[0xd];
	if (i != 1 && i != 2 && i != 4 && i != 8
		&& i != 0x10 && i != 0x20 && i != 0x40 && i != 0x80) {
		dprintf("dosfs error: unsupported sectors per cluster (%d)\n", i);
		goto error;
	}

	vol->reserved_sectors = read16(buf, 0xe);

	vol->fat_count = buf[0x10];
	if (vol->fat_count == 0 || vol->fat_count > 8) {
		dprintf("dosfs error: unreasonable fat count (%" B_PRIu32 ")\n",
			vol->fat_count);
		goto error;
	}

	vol->media_descriptor = buf[0x15];
	// check media descriptor versus known types
	if (buf[0x15] != 0xf0 && buf[0x15] < 0xf8) {
		dprintf("dosfs error: invalid media descriptor byte\n");
		goto error;
	}

	vol->vol_entry = -2;	// for now, assume there is no volume entry
	strcpy(vol->vol_label, "no name");

	// now become more discerning citizens
	vol->sectors_per_fat = read16(buf, 0x16);
	if (vol->sectors_per_fat == 0) {
		// fat32 land
		vol->fat_bits = 32;
		vol->sectors_per_fat = read32(buf, 0x24);
		vol->total_sectors = read32(buf, 0x20);

		vol->fsinfo_sector = read16(buf, 0x30);
		if (vol->fsinfo_sector != 0xffff
			&& vol->fsinfo_sector >= vol->reserved_sectors) {
			dprintf("dosfs error: fsinfo sector too large (0x%x)\n",
				vol->fsinfo_sector);
			goto error;
		}

		vol->fat_mirrored = !(buf[0x28] & 0x80);
		vol->active_fat = !vol->fat_mirrored ? (buf[0x28] & 0xf) : 0;

		vol->data_start = vol->reserved_sectors + vol->fat_count
			* vol->sectors_per_fat;
		vol->total_clusters = (vol->total_sectors - vol->data_start)
			/ vol->sectors_per_cluster;

		vol->root_vnode.cluster = read32(buf, 0x2c);
		if (vol->root_vnode.cluster >= vol->total_clusters) {
			dprintf("dosfs error: root vnode cluster too large (0x%" B_PRIu32
				")\n", vol->root_vnode.cluster);
			goto error;
		}

		if (dosfs_read_label(true, buf, vol->vol_label))
			vol->vol_entry = -1;
	} else {
		// fat12 & fat16
		if (vol->fat_count != 2) {
			dprintf("dosfs error: claims %" B_PRIu32 " fat tables\n",
				vol->fat_count);
			goto error;
		}

		vol->root_entries_count = read16(buf, 0x11);
		if (vol->root_entries_count % (vol->bytes_per_sector / 0x20)) {
			dprintf("dosfs error: invalid number of root entries\n");
			goto error;
		}

		vol->fsinfo_sector = 0xffff;
		vol->total_sectors = read16(buf, 0x13); // partition size
		if (vol->total_sectors == 0)
			vol->total_sectors = read32(buf, 0x20);

		if (geo != NULL) {
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

			if (memcmp(buf + 0x0b, bogus_zip_data, sizeof(bogus_zip_data)) == 0
				&& vol->total_sectors == 196576
				&& ((off_t)geo->sectors_per_track * (off_t)geo->cylinder_count
					* (off_t)geo->head_count) == 196192) {
				vol->total_sectors = 196192;
			}
		}

		vol->fat_mirrored = true;
		vol->active_fat = 0;

		vol->root_start = vol->reserved_sectors + vol->fat_count
			* vol->sectors_per_fat;
		vol->root_sectors = vol->root_entries_count * 0x20
			/ vol->bytes_per_sector;
		vol->root_vnode.cluster = 1;
		vol->root_vnode.end_cluster = 1;
		vol->root_vnode.st_size = vol->root_sectors * vol->bytes_per_sector;

		vol->data_start = vol->root_start + vol->root_sectors;
		vol->total_clusters = (vol->total_sectors - vol->data_start)
			/ vol->sectors_per_cluster;

		// XXX: uncertain about border cases; win32 sdk says cutoffs are at
		//      at ff6/ff7 (or fff6/fff7), but that doesn't make much sense
		if (vol->total_clusters > 0xff1)
			vol->fat_bits = 16;
		else
			vol->fat_bits = 12;

		if (dosfs_read_label(false, buf, vol->vol_label))
			vol->vol_entry = -1;
	}

	// perform sanity checks on the FAT

	// the media descriptor in active FAT should match the one in the BPB
	if ((err = read_pos(vol->fd, vol->bytes_per_sector * (vol->reserved_sectors
			+ vol->active_fat * vol->sectors_per_fat),
			(void *)media_buf, 0x200)) != 0x200) {
		dprintf("dosfs error: error reading FAT\n");
		goto error;
	}

	if (media_buf[0] != vol->media_descriptor) {
		dprintf("dosfs error: media descriptor mismatch (%x != %x)\n", media_buf[0],
			vol->media_descriptor);
		goto error;
	}

	if (vol->fat_mirrored) {
		uint32 i;
		uint8 mirror_media_buf[512];
		for (i = 0; i < vol->fat_count; i++) {
			if (i != vol->active_fat) {
				DPRINTF(1, ("checking fat #%" B_PRIu32 "\n", i));
				mirror_media_buf[0] = ~media_buf[0];
				if ((err = read_pos(vol->fd, vol->bytes_per_sector
						* (vol->reserved_sectors + vol->sectors_per_fat * i),
						(void *)mirror_media_buf, 0x200)) != 0x200) {
					dprintf("dosfs error: error reading FAT %" B_PRIu32 "\n",
						i);
					goto error;
				}

				if (mirror_media_buf[0] != vol->media_descriptor) {
					dprintf("dosfs error: media descriptor mismatch in fat # "
						"%" B_PRIu32 " (%" B_PRIu8 " != %" B_PRIu8 ")\n", i,
						mirror_media_buf[0], vol->media_descriptor);
					goto error;
				}
#if 0
				// checking for exact matches of fats is too
				// restrictive; allow these to go through in
				// case the fat is corrupted for some reason
				if (memcmp(media_buf, mirror_media_buf, 0x200)) {
					dprintf("dosfs error: fat %d doesn't match active fat "
						"(%d)\n", i, vol->active_fat);
					goto error;
				}
#endif
			}
		}
	}

	/* check that the partition is large enough to contain the file system */
	if (geo != NULL
			&& vol->total_sectors >
				geo->sectors_per_track * geo->cylinder_count
				* geo->head_count) {
		dprintf("dosfs: volume extends past end of partition, mounting read-only\n");
		vol->flags |= B_FS_IS_READONLY;
	}

	// now we are convinced of the drive's validity

	// this will be updated later if fsinfo exists
	vol->last_allocated = 2;
	vol->beos_vnid = INVALID_VNID_BITS_MASK;

	// initialize block cache
	vol->fBlockCache = block_cache_create(vol->fd, vol->total_sectors,
		vol->bytes_per_sector, (vol->flags & B_FS_IS_READONLY) != 0);
	if (vol->fBlockCache == NULL) {
		dprintf("dosfs error: error initializing block cache\n");
		goto error;
	}

	// find volume label (supercedes any label in the bpb)
	{
		struct diri diri;
		uint8 *buffer;
		buffer = diri_init(vol, vol->root_vnode.cluster, 0, &diri);
		for (; buffer; buffer = diri_next_entry(&diri)) {
			if ((buffer[0x0b] & FAT_VOLUME) && (buffer[0x0b] != 0xf)
					&& (buffer[0] != 0xe5)) {
				vol->vol_entry = diri.current_index;
				memcpy(vol->vol_label, buffer, 11);
				dosfs_trim_spaces(vol->vol_label);
				break;
			}
		}

		diri_free(&diri);
	}

	DPRINTF(0, ("root vnode id = %" B_PRIdINO "\n", vol->root_vnode.vnid));
	DPRINTF(0, ("volume label [%s] (%" B_PRIu32 ")\n", vol->vol_label,
		vol->vol_entry));

	// steal a trick from bfs
	if (!memcmp(vol->vol_label, "__RO__     ", 11))
		vol->flags |= B_FS_IS_READONLY;

	return vol;

error:
	free(vol);
	return NULL;
}


static void
volume_uninit(nspace *vol)
{
	block_cache_delete(vol->fBlockCache, false);
	free(vol);
}


static void
volume_count_free_cluster(nspace *vol)
{
	status_t err;

	if (vol->flags & B_FS_IS_READONLY)
		vol->free_clusters = 0;
	else {
		uint32 free_count, last_allocated;
		err = get_fsinfo(vol, &free_count, &last_allocated);
		if (err >= 0) {
			if (free_count < vol->total_clusters)
				vol->free_clusters = free_count;
			else {
				dprintf("dosfs error: free cluster count from fsinfo block "
					"invalid %" B_PRIu32 "\n", free_count);
				err = -1;
			}

			if (last_allocated < vol->total_clusters)
				vol->last_allocated = last_allocated; //update to a closer match
		}

		if (err < 0) {
			if ((err = count_free_clusters(vol)) < 0) {
				dprintf("dosfs error: error counting free clusters (%s)\n",
					strerror(err));
				return;
			}
			vol->free_clusters = err;
		}
	}
}


static int
lock_removable_device(int fd, bool state)
{
	return ioctl(fd, B_SCSI_PREVENT_ALLOW, &state, sizeof(state));
}


static status_t
mount_fat_disk(const char *path, fs_volume *_vol, const int flags,
	nspace** newVol, int fs_flags, int op_sync_mode)
{
	nspace *vol = NULL;
	uint8 buf[512];
	device_geometry geo;
	status_t err;
	int fd;
	int vol_flags;

	vol_flags = B_FS_IS_PERSISTENT | B_FS_HAS_MIME;

	// open read-only for now
	if ((err = (fd = open(path, O_RDONLY | O_NOCACHE))) < 0) {
		dprintf("dosfs error: unable to open %s (%s)\n", path, strerror(err));
		goto error0;
	}

	// get device characteristics
	if (ioctl(fd, B_GET_GEOMETRY, &geo, sizeof(device_geometry)) < 0) {
		struct stat st;
		if (fstat(fd, &st) >= 0 && S_ISREG(st.st_mode)) {
			/* support mounting disk images */
			geo.bytes_per_sector = 0x200;
			geo.sectors_per_track = 1;
			geo.cylinder_count = st.st_size / 0x200;
			geo.head_count = 1;
			geo.read_only = !(st.st_mode & S_IWUSR);
			geo.removable = true;
		} else {
			dprintf("dosfs error: error getting device geometry\n");
			goto error1;
		}
	}

	if (geo.bytes_per_sector != 0x200 && geo.bytes_per_sector != 0x400
		&& geo.bytes_per_sector != 0x800 && geo.bytes_per_sector != 0x1000) {
		dprintf("dosfs error: unsupported device block size (%" B_PRIu32 ")\n",
			geo.bytes_per_sector);
		goto error1;
	}

	if (geo.removable) {
		DPRINTF(0, ("%s is removable\n", path));
		vol_flags |= B_FS_IS_REMOVABLE;
	}

	if (geo.read_only || (flags & B_MOUNT_READ_ONLY)) {
		DPRINTF(0, ("%s is read-only\n", path));
		vol_flags |= B_FS_IS_READONLY;
	} else {
		// reopen it with read/write permissions
		close(fd);
		if ((err = (fd = open(path, O_RDWR | O_NOCACHE))) < 0) {
			dprintf("dosfs error: unable to open %s (%s)\n", path,
				strerror(err));
			goto error0;
		}

		if ((vol_flags & B_FS_IS_REMOVABLE)
			&& (fs_flags & FS_FLAGS_LOCK_DOOR))
			lock_removable_device(fd, true);
	}

	// see if we need to go into op sync mode
	fs_flags &= ~FS_FLAGS_OP_SYNC;
	switch (op_sync_mode) {
		case 1:
			if ((vol_flags & B_FS_IS_REMOVABLE) == 0) {
				// we're not removable, so skip op_sync
				break;
			}
			// supposed to fall through

		case 2:
			dprintf("dosfs: mounted with op_sync enabled\n");
			fs_flags |= FS_FLAGS_OP_SYNC;
			break;

		case 0:
		default:
			break;
	}

	// read in the boot sector
	if ((err = read_pos(fd, 0, (void *)buf, 512)) != 512) {
		dprintf("dosfs error: error reading boot sector\n");
		goto error1;
	}

	vol = volume_init(fd, buf, vol_flags, fs_flags, &geo);
	if (vol == NULL) {
		dprintf("dosfs error: failed to initialize volume\n");
		err = B_ERROR;
		goto error1;
	}

	vol->volume = _vol;
	vol->id = _vol->id;
	strncpy(vol->device, path, sizeof(vol->device));

	{
		void *handle;
		handle = load_driver_settings("fat");
		vol->respect_disk_image =
			get_driver_boolean_parameter(handle, "respect", true, true);
		unload_driver_settings(handle);
	}

	// Initialize the vnode cache
	if (init_vcache(vol) != B_OK) {
		dprintf("dosfs error: error initializing vnode cache\n");
		goto error2;
	}

	// and the dlist cache
	if (dlist_init(vol) != B_OK) {
		dprintf("dosfs error: error initializing dlist cache\n");
		goto error3;
	}

	volume_count_free_cluster(vol);

	DPRINTF(0, ("built at %s on %s\n", build_time, build_date));
	DPRINTF(0, ("mounting %s (id %" B_PRIdDEV ", device %u, media descriptor %"
		B_PRIu8 ")\n", vol->device, vol->id, vol->fd, vol->media_descriptor));
	DPRINTF(0, ("%" B_PRIu32 " bytes/sector, %" B_PRIu32 " sectors/cluster\n",
		vol->bytes_per_sector, vol->sectors_per_cluster));
	DPRINTF(0, ("%" B_PRIu32 " reserved sectors, %" B_PRIu32 " total sectors\n",
		vol->reserved_sectors, vol->total_sectors));
	DPRINTF(0, ("%" B_PRIu32 " %" B_PRIu8 "-bit fats, %" B_PRIu32
		" sectors/fat, %" B_PRIu32 " root entries\n", vol->fat_count,
		vol->fat_bits, vol->sectors_per_fat, vol->root_entries_count));
	DPRINTF(0, ("root directory starts at sector %" B_PRIu32 " (cluster %"
		B_PRIu32 "), data at sector %" B_PRIu32 "\n", vol->root_start,
		vol->root_vnode.cluster, vol->data_start));
	DPRINTF(0, ("%" B_PRIu32 " total clusters, %" B_PRIu32 " free\n",
		vol->total_clusters, vol->free_clusters));
	DPRINTF(0, ("fat mirroring is %s, fs info sector at sector %" B_PRIu16 "\n",
		vol->fat_mirrored ? "on" : "off", vol->fsinfo_sector));
	DPRINTF(0, ("last allocated cluster = %" B_PRIu32 "\n",
		vol->last_allocated));

	if (vol->fat_bits == 32) {
		// now that the block cache has been initialised, we can figure
		// out the length of the root directory with count_clusters()
		vol->root_vnode.st_size = count_clusters(vol, vol->root_vnode.cluster)
			* vol->bytes_per_sector * vol->sectors_per_cluster;
		vol->root_vnode.end_cluster = get_nth_fat_entry(vol,
			vol->root_vnode.cluster, vol->root_vnode.st_size
			/ vol->bytes_per_sector / vol->sectors_per_cluster - 1);
	}

	// initialize root vnode
	vol->root_vnode.vnid = vol->root_vnode.dir_vnid = GENERATE_DIR_CLUSTER_VNID(
		vol->root_vnode.cluster, vol->root_vnode.cluster);
	vol->root_vnode.sindex = vol->root_vnode.eindex = 0xffffffff;
	vol->root_vnode.mode = FAT_SUBDIR;
	time(&(vol->root_vnode.st_time));
	vol->root_vnode.st_crtim = vol->root_vnode.st_time;
	vol->root_vnode.mime = NULL;
	vol->root_vnode.dirty = false;
	dlist_add(vol, vol->root_vnode.vnid);


	DPRINTF(0, ("root vnode id = %" B_PRIdINO "\n", vol->root_vnode.vnid));
	DPRINTF(0, ("volume label [%s] (%" B_PRIu32 ")\n", vol->vol_label,
		vol->vol_entry));

	// steal a trick from bfs
	if (!memcmp(vol->vol_label, "__RO__     ", 11))
		vol->flags |= B_FS_IS_READONLY;

	*newVol = vol;
	return B_NO_ERROR;

error3:
	uninit_vcache(vol);
error2:
	if (!(vol->flags & B_FS_IS_READONLY) && (vol->flags & B_FS_IS_REMOVABLE)
		&& (vol->fs_flags & FS_FLAGS_LOCK_DOOR)) {
		lock_removable_device(fd, false);
	}

	volume_uninit(vol);
error1:
	close(fd);
error0:
	return err >= B_NO_ERROR ? EINVAL : err;
}


//	#pragma mark - Scanning


typedef struct identify_cookie {
	uint32 bytes_per_sector;
	uint32 total_sectors;
	char name[12];
} identify_cookie;


static float
dosfs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	uint8 buf[512];
	int i;
	uint32 bytes_per_sector;
	uint32 fatCount;
	uint32 total_sectors;
	uint32 sectors_per_fat;
	char name[12];
	identify_cookie *cookie;

	// read in the boot sector
	if (read_pos(fd, 0, buf, 512) != 512)
		return -1;

	// only check boot signature on hard disks to account for broken mtools
	// behavior
	if ((buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa) && buf[0x15] == 0xf8)
		return -1;
	if (!memcmp(buf + 3, "NTFS    ", 8) || !memcmp(buf + 3, "HPFS    ", 8))
		return -1;

	// first fill in the universal fields from the bpb
	bytes_per_sector = read16(buf, 0xb);
	if (bytes_per_sector != 0x200 && bytes_per_sector != 0x400
		&& bytes_per_sector != 0x800 && bytes_per_sector != 0x1000) {
		return -1;
	}

	// must be a power of two
	i = buf[0xd];
	if (i != 1 && i != 2 && i != 4 && i != 8 && i != 0x10 && i != 0x20
		&& i != 0x40 && i != 0x80)
		return -1;

	fatCount = buf[0x10];
	if (fatCount == 0 || fatCount > 8)
		return -1;

	// check media descriptor versus known types
	if (buf[0x15] != 0xf0 && buf[0x15] < 0xf8)
		return -1;

	strcpy(name, "no name");
	sectors_per_fat = read16(buf, 0x16);
	if (sectors_per_fat == 0) {
		total_sectors = read32(buf, 0x20);
		dosfs_read_label(true, buf, name);
	} else {
		total_sectors = read16(buf, 0x13);
			// partition size
		if (total_sectors == 0)
			total_sectors = read32(buf, 0x20);

		dosfs_read_label(false, buf, name);
	}

	// find volume label (supercedes any label in the bpb)
	{
		nspace *vol;
		vol = volume_init(fd, buf, 0, 0, NULL);
		if (vol != NULL)
		{
			strlcpy(name, vol->vol_label, 12);
			volume_uninit(vol);
		}
	}

	cookie = (identify_cookie *)malloc(sizeof(identify_cookie));
	if (!cookie)
		return -1;

	cookie->bytes_per_sector = bytes_per_sector;
	cookie->total_sectors = total_sectors;
	sanitize_name(name, 12);
	strlcpy(cookie->name, name, 12);
	*_cookie = cookie;

	return 0.8f;
}


static status_t
dosfs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->total_sectors * cookie->bytes_per_sector;
	partition->block_size = cookie->bytes_per_sector;
	partition->content_name = strdup(cookie->name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
dosfs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;
	free(cookie);
}


//	#pragma mark -


static status_t
dosfs_mount(fs_volume *_vol, const char *device, uint32 flags,
	const char *args, ino_t *_rootID)
{
	int	result;
	nspace *vol;
	void *handle;
	int op_sync_mode = 0;
	int fs_flags = 0;

	handle = load_driver_settings("fat");
	if (handle != NULL) {
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
		if (op_sync_mode < 0 || op_sync_mode > 2)
			op_sync_mode = 0;
		if (strcasecmp(get_driver_parameter(handle, "lock_device", "true", "true"), "false") == 0) {
			dprintf("dosfs: mounted with lock_device = false\n");
		} else {
			dprintf("dosfs: mounted with lock_device = true\n");
			fs_flags |= FS_FLAGS_LOCK_DOOR;
		}

		unload_driver_settings(handle);
	}

	/* args is a command line option; dosfs doesn't use any so
	   we can ignore these arguments */
	TOUCH(args);

#if __RO__
	// make it read-only
	flags |= 1;
#endif

	// Try and mount volume as a FAT volume
	if ((result = mount_fat_disk(device, _vol, flags, &vol, fs_flags,
			op_sync_mode)) == B_NO_ERROR) {
		char name[32];

		*_rootID = vol->root_vnode.vnid;
		_vol->private_volume = (void *)vol;
		_vol->ops = &gFATVolumeOps;

		// You MUST do this. Create the vnode for the root.
		result = publish_vnode(_vol, *_rootID, (void*)&(vol->root_vnode),
			&gFATVnodeOps, make_mode(vol, &vol->root_vnode), 0);
		if (result != B_NO_ERROR) {
			dprintf("error creating new vnode (%s)\n", strerror(result));
			goto error;
		}
		sprintf(name, "fat lock %" B_PRIdDEV, vol->id);
		recursive_lock_init_etc(&(vol->vlock), name, MUTEX_FLAG_CLONE_NAME);

#if DEBUG
		if (atomic_add(&instances, 1) == 0) {
			add_debugger_command("fat", debug_fat_nspace, "dump a fat nspace structure");
			add_debugger_command("dvnode", debug_dvnode, "dump a fat vnode structure");
			add_debugger_command("dfvnid", debug_dfvnid, "find a vnid in the vnid cache");
			add_debugger_command("dfloc", debug_dfloc, "find a loc in the vnid cache");
			add_debugger_command("dc2s", debug_dc2s, "calculate sector for cluster");
		}
#endif
	}

	return result;

error:
	block_cache_delete(vol->fBlockCache, false);
	dlist_uninit(vol);
	uninit_vcache(vol);
	free(vol);
	return EINVAL;
}


static void
update_fsinfo(nspace *vol)
{
	if (vol->fat_bits == 32 && vol->fsinfo_sector != 0xffff
		&& (vol->flags & B_FS_IS_READONLY) == 0) {
		uchar *buffer;
		status_t status = block_cache_get_writable_etc(vol->fBlockCache,
				vol->fsinfo_sector, 0, vol->bytes_per_sector, -1,
				(void**)&buffer);
		if (status == B_OK) {
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
			} else {
				dprintf("update_fsinfo: fsinfo block has invalid magic number\n");
				block_cache_set_dirty(vol->fBlockCache, vol->fsinfo_sector,
					false, -1);
			}
			block_cache_put(vol->fBlockCache, vol->fsinfo_sector);
		} else {
			dprintf("update_fsinfo: error getting fsinfo sector %x: %s\n",
				vol->fsinfo_sector, strerror(status));
		}
	}
}


static status_t
get_fsinfo(nspace *vol, uint32 *free_count, uint32 *last_allocated)
{
	uchar *buffer;
	status_t result;

	if ((vol->fat_bits != 32) || (vol->fsinfo_sector == 0xffff))
		return B_ERROR;

	result = block_cache_get_etc(vol->fBlockCache, vol->fsinfo_sector, 0,
		vol->bytes_per_sector, &buffer);
	if (result != B_OK) {
		dprintf("get_fsinfo: error getting fsinfo sector %x: %s\n",
			vol->fsinfo_sector, strerror(result));
		return result;
	}

	if ((read32(buffer,0) == 0x41615252) && (read32(buffer,0x1e4) == 0x61417272) && (read16(buffer,0x1fe) == 0xaa55)) {
		*free_count = read32(buffer,0x1e8);
		*last_allocated = read32(buffer,0x1ec);
		result = B_OK;
	} else {
		dprintf("get_fsinfo: fsinfo block has invalid magic number\n");
		result = B_ERROR;
	}

	block_cache_put(vol->fBlockCache, vol->fsinfo_sector);
	return result;
}


static status_t
dosfs_unmount(fs_volume *_vol)
{
	int result = B_NO_ERROR;

	nspace* vol = (nspace*)_vol->private_volume;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_unmount volume %" B_PRIdDEV "\n", vol->id));

	update_fsinfo(vol);

	// Unlike in BeOS, we need to put the reference to our root node ourselves
	put_vnode(_vol, vol->root_vnode.vnid);
	block_cache_delete(vol->fBlockCache, true);

#if DEBUG
	if (atomic_add(&instances, -1) == 1) {
		remove_debugger_command("fat", debug_fat_nspace);
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
	recursive_lock_destroy(&(vol->vlock));
	free(vol);

#if USE_DMALLOC
	check_mem();
#endif

	return result;
}


// dosfs_read_fs_stat - Fill in fs_info struct for device.
static status_t
dosfs_read_fs_stat(fs_volume *_vol, struct fs_info * fss)
{
	nspace* vol = (nspace*)_vol->private_volume;

	LOCK_VOL(vol);

	DPRINTF(1, ("dosfs_read_fs_stat called\n"));

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
		strlcpy(fss->volume_name, vol->vol_label, sizeof(fss->volume_name));
	else
		strcpy(fss->volume_name, "no name");

	sanitize_name(fss->volume_name, 12);

	// File system name
	strcpy(fss->fsh_name, "fat");

	UNLOCK_VOL(vol);

	return B_OK;
}


static status_t
dosfs_write_fs_stat(fs_volume *_vol, const struct fs_info * fss, uint32 mask)
{
	status_t result = B_ERROR;
	nspace* vol = (nspace*)_vol->private_volume;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_write_fs_stat called\n"));

	/* if it's a r/o file system and not the special hack, then don't allow
	 * volume renaming */
	if ((vol->flags & B_FS_IS_READONLY) && memcmp(vol->vol_label, "__RO__     ", 11)) {
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (mask & FS_WRITE_FSINFO_NAME) {
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
			result = block_cache_get_writable_etc(vol->fBlockCache, 0,
				0, vol->bytes_per_sector, -1, (void**)&buffer);
			if (result != B_OK)
				goto bi;

			if ((vol->sectors_per_fat == 0 && (buffer[0x42] != 0x29
					|| strncmp((const char *)buffer + 0x47, vol->vol_label, 11)
						!= 0))
				|| (vol->sectors_per_fat != 0 && (buffer[0x26] != 0x29
					|| strncmp((const char *)buffer + 0x2b, vol->vol_label, 11)
						== 0))) {
				dprintf("dosfs_wfsstat: label mismatch\n");
				block_cache_set_dirty(vol->fBlockCache, 0, false, -1);
				result = B_ERROR;
			} else {
				memcpy(buffer + 0x2b, name, 11);
				result = B_OK;
			}
			block_cache_put(vol->fBlockCache, 0);
		} else if (vol->vol_entry >= 0) {
			struct diri diri;
			uint8 *buffer;
			buffer = diri_init(vol, vol->root_vnode.cluster, vol->vol_entry, &diri);

			// check if it is the same as the old volume label
			if (buffer == NULL || strncmp((const char *)buffer, vol->vol_label,
					11) == 0) {
				dprintf("dosfs_wfsstat: label mismatch\n");
				diri_free(&diri);
				result = B_ERROR;
				goto bi;
			}

			diri_make_writable(&diri);
			memcpy(buffer, name, 11);
			diri_free(&diri);
			result = B_OK;
		} else {
			uint32 index;
			result = create_volume_label(vol, name, &index);
			if (result == B_OK) vol->vol_entry = index;
		}

		if (result == B_OK) {
			memcpy(vol->vol_label, name, 11);
			dosfs_trim_spaces(vol->vol_label);
		}
	}

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

bi:	UNLOCK_VOL(vol);

	return result;
}


#if 0
static status_t
dosfs_ioctl(fs_volume *_vol, fs_vnode *_node, void *cookie, uint32 code,
	void *buf, size_t len)
{
	status_t result = B_OK;
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;

	TOUCH(cookie); TOUCH(len);

	LOCK_VOL(vol);

	switch (code) {
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
			dprintf("volume label [%s]\n", vol->vol_label);
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
			dprintf("validating vcache for %lx\n", vol->id);
			validate_vcache(vol);
			dprintf("validation complete for %lx\n", vol->id);
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
			DPRINTF(0, ("dosfs_ioctl: vol %" B_PRIdDEV ", vnode %" B_PRIdINO
				" code = %" B_PRIu32 "\n", vol->id, node->vnid, code));
			result = B_DEV_INVALID_IOCTL;
			break;
	}

	UNLOCK_VOL(vol);

	return result;
}
#endif


status_t
_dosfs_sync(nspace *vol)
{
	update_fsinfo(vol);
	block_cache_sync(vol->fBlockCache);

	return B_OK;
}


static status_t
dosfs_sync(fs_volume *_vol)
{
	nspace *vol = (nspace *)_vol->private_volume;
	status_t err;

	DPRINTF(0, ("dosfs_sync called on volume %" B_PRIdDEV "\n", vol->id));

	LOCK_VOL(vol);
	err = _dosfs_sync(vol);
	UNLOCK_VOL(vol);

	return err;
}


static status_t
dosfs_fsync(fs_volume *_vol, fs_vnode *_node)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	status_t err = B_OK;

	LOCK_VOL(vol);

	if (node->cache)
		err = file_cache_sync(node->cache);

	UNLOCK_VOL(vol);
	return err;
}


//	#pragma mark -


static uint32
dosfs_get_supported_operations(partition_data* partition, uint32 mask)
{
	dprintf("dosfs_get_supported_operations\n");
	// TODO: We should at least check the partition size.
	return B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_WRITING;
}


//	#pragma mark -


static status_t
dos_std_ops(int32 op, ...)
{
	dprintf("dos_std_ops()\n");
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


fs_volume_ops gFATVolumeOps = {
	&dosfs_unmount,
	&dosfs_read_fs_stat,
	&dosfs_write_fs_stat,
	&dosfs_sync,
	&dosfs_read_vnode,

	/* index directory & index operations */
	NULL,	//&fs_open_index_dir,
	NULL,	//&fs_close_index_dir,
	NULL,	//&fs_free_index_dir_cookie,
	NULL,	//&fs_read_index_dir,
	NULL,	//&fs_rewind_index_dir,

	NULL,	//&fs_create_index,
	NULL,	//&fs_remove_index,
	NULL,	//&fs_stat_index,

	/* query operations */
	NULL,	//&fs_open_query,
	NULL,	//&fs_close_query,
	NULL,	//&fs_free_query_cookie,
	NULL,	//&fs_read_query,
	NULL,	//&fs_rewind_query,
};


fs_vnode_ops gFATVnodeOps = {
	/* vnode operations */
	&dosfs_walk,
	&dosfs_get_vnode_name,
	&dosfs_release_vnode,
	&dosfs_remove_vnode,

	/* VM file access */
	&dosfs_can_page,
	&dosfs_read_pages,
	&dosfs_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	&dosfs_get_file_map,

	NULL,	// fs_ioctl()
	NULL,	// fs_set_flags,
	NULL,	// fs_select
	NULL,	// fs_deselect
	&dosfs_fsync,

	&dosfs_readlink,
	NULL,	// fs_create_symlink,

	NULL,	// fs_link,
	&dosfs_unlink,
	&dosfs_rename,

	&dosfs_access,
	&dosfs_rstat,
	&dosfs_wstat,
	NULL,	// fs_preallocate,

	/* file operations */
	&dosfs_create,
	&dosfs_open,
	&dosfs_close,
	&dosfs_free_cookie,
	&dosfs_read,
	&dosfs_write,

	/* directory operations */
	&dosfs_mkdir,
	&dosfs_rmdir,
	&dosfs_opendir,
	&dosfs_closedir,
	&dosfs_free_dircookie,
	&dosfs_readdir,
	&dosfs_rewinddir,

	/* attribute directory operations */
	&dosfs_open_attrdir,
	&dosfs_close_attrdir,
	&dosfs_free_attrdir_cookie,
	&dosfs_read_attrdir,
	&dosfs_rewind_attrdir,

	/* attribute operations */
	NULL,	// fs_create_attr,
	&dosfs_open_attr,
	&dosfs_close_attr,
	&dosfs_free_attr_cookie,
	&dosfs_read_attr,
	&dosfs_write_attr,

	&dosfs_read_attr_stat,
	NULL,	// fs_write_attr_stat,
	NULL,	// fs_rename_attr,
	NULL,	// fs_remove_attr,
};


static file_system_module_info sFATFileSystem = {
	{
		"file_systems/fat" B_CURRENT_FS_API_VERSION,
		0,
		dos_std_ops,
	},

	"fat",					// short_name
	"FAT32 File System",	// pretty_name

	// DDM flags
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
//	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	dosfs_identify_partition,
	dosfs_scan_partition,
	dosfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&dosfs_mount,

	/* capability querying operations */
	&dosfs_get_supported_operations,

	NULL,	// validate_resize
	NULL,	// validate_move
	NULL,	// validate_set_content_name
	NULL,	// validate_set_content_parameters
	NULL,	// validate_initialize,

	/* shadow partition modification */
	NULL,	// shadow_changed

	/* writing */
	NULL,	// defragment
	NULL,	// repair
	NULL,	// resize
	NULL,	// move
	NULL,	// set_content_name
	NULL,	// set_content_parameters
	dosfs_initialize,
	dosfs_uninitialize
};

module_info *modules[] = {
	(module_info *)&sFATFileSystem,
	NULL,
};
