/* volume_util.c - volume functions
 *
 * Copyright (c) 2006-2007 Troeglazov Gerasim (3dEyes**)
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "support.h"
#include "endians.h"
#include "bootsect.h"
#include "device.h"
#include "attrib.h"
#include "volume.h"
#include "mft.h"
#include "bitmap.h"
#include "inode.h"
#include "runlist.h"
#include "utils.h"
#include "ntfs.h"
#include "volume_util.h"

static unsigned char ntfs_bits_table[] = { 4, 3, 3, 2, 3, 2, 2, 1,
	3, 2, 2, 1, 2, 1, 1, 0 };

static uint8 ntfs_count_bits(unsigned char byte, unsigned char shift)
{
	int i;
	unsigned char counter = 0;
	
	if (shift < 8) {
	 	for (i=0; i<shift; i++) {
			if (!(byte & 0x80))
				counter++;
			byte = byte << 1;
		}
	} else {
		counter += ntfs_bits_table[byte & 0x0F];
		counter += ntfs_bits_table[(byte & 0xF0) >> 4];			
	}
	
	return counter;
}


int ntfs_calc_free_space(nspace *_ns)
{
	nspace		*ns = (nspace*)_ns;
	ntfs_volume *vol;
	ntfs_attr 	*data;
	s64 		free_clusters = 0;
	off_t 	 	pos = 0;
	size_t 	 	readed;
	unsigned 	char *buf;

	if (ns == NULL)
		return -1;

	vol = ns->ntvol;
	if (vol == NULL)
		return -1;

	data = vol->lcnbmp_na;
	if (data == NULL)
		return -1;

	if (!(ns->state & NF_FreeClustersOutdate))
		return -1;	

	buf = (unsigned char*)ntfs_malloc(vol->cluster_size);
	if (buf == NULL)
		goto exit;
	
	while (pos < data->data_size) {
		if (pos % vol->cluster_size == 0) {
			readed = ntfs_attr_pread(vol->lcnbmp_na, pos,
				min(data->data_size - pos, vol->cluster_size), buf);
			if (readed < B_NO_ERROR)
				goto error;
			if (readed != min(data->data_size - pos, vol->cluster_size))
				goto error;
		}

		free_clusters += ntfs_count_bits( buf[pos%vol->cluster_size],
			min((vol->nr_clusters) - (pos * 8), 8));
		pos++;
	}
error:
	free(buf);
exit:	
	ns->free_clusters = free_clusters;
	ns->state &= ~(NF_FreeClustersOutdate);
	
	return B_NO_ERROR;
}


static int resize_resident_attribute_value(MFT_RECORD *m, ATTR_RECORD *a,
		const u32 new_vsize)
{
	int new_alen, new_muse;

	/* New attribute length and mft record bytes used. */
	new_alen = (le16_to_cpu(a->value_offset) + new_vsize + 7) & ~7;
	new_muse = le32_to_cpu(m->bytes_in_use) - le32_to_cpu(a->length) +
			new_alen;
	/* Check for sufficient space. */
	if ((u32)new_muse > le32_to_cpu(m->bytes_allocated)) {
		errno = ENOSPC;
		return -1;
	}
	/* Move attributes behind @a to their new location. */
	memmove((char*)a + new_alen, (char*)a + le32_to_cpu(a->length),
			le32_to_cpu(m->bytes_in_use) - ((char*)a - (char*)m) -
			le32_to_cpu(a->length));
	/* Adjust @m to reflect change in used space. */
	m->bytes_in_use = cpu_to_le32(new_muse);
	/* Adjust @a to reflect new value size. */
	a->length = cpu_to_le32(new_alen);
	a->value_length = cpu_to_le32(new_vsize);
	return 0;
}


int ntfs_change_label(ntfs_volume *vol, char *label)
{
	ntfs_attr_search_ctx *ctx;
	ntfschar *new_label = NULL;
	ATTR_RECORD *a;
	int label_len;
	int result = 0;

	ctx = ntfs_attr_get_search_ctx(vol->vol_ni, NULL);
	if (!ctx) {
		ntfs_log_perror("Failed to get attribute search context");
		goto err_out;
	}
	if (ntfs_attr_lookup(AT_VOLUME_NAME, AT_UNNAMED, 0, 0, 0, NULL, 0,
			ctx)) {
		if (errno != ENOENT) {
			ntfs_log_perror("Lookup of $VOLUME_NAME attribute failed");
			goto err_out;
		}
		/* The volume name attribute does not exist.  Need to add it. */
		a = NULL;
	} else {
		a = ctx->attr;
		if (a->non_resident) {
			ntfs_log_error("Error: Attribute $VOLUME_NAME must be "
					"resident.\n");
			goto err_out;
		}
	}
	label_len = ntfs_mbstoucs(label, &new_label);
	if (label_len == -1) {
		ntfs_log_perror("Unable to convert label string to Unicode");
		goto err_out;
	}
	label_len *= sizeof(ntfschar);
	if (label_len > 0x100) {
		ntfs_log_error("New label is too long. Maximum %u characters "
				"allowed. Truncating excess characters.\n",
				(unsigned)(0x100 / sizeof(ntfschar)));
		label_len = 0x100;
		new_label[label_len / sizeof(ntfschar)] = cpu_to_le16(L'\0');
	}
	if (a) {
		if (resize_resident_attribute_value(ctx->mrec, a, label_len)) {
			ntfs_log_perror("Error resizing resident attribute");
			goto err_out;
		}
	} else {
		/* sizeof(resident attribute record header) == 24 */
		int asize = (24 + label_len + 7) & ~7;
		u32 biu = le32_to_cpu(ctx->mrec->bytes_in_use);
		if (biu + asize > le32_to_cpu(ctx->mrec->bytes_allocated)) {
			errno = ENOSPC;
			ntfs_log_perror("Error adding resident attribute");
			goto err_out;
		}
		a = ctx->attr;
		memmove((u8*)a + asize, a, biu - ((u8*)a - (u8*)ctx->mrec));
		ctx->mrec->bytes_in_use = cpu_to_le32(biu + asize);
		a->type = AT_VOLUME_NAME;
		a->length = cpu_to_le32(asize);
		a->non_resident = 0;
		a->name_length = 0;
		a->name_offset = cpu_to_le16(24);
		a->flags = cpu_to_le16(0);
		a->instance = ctx->mrec->next_attr_instance;
		ctx->mrec->next_attr_instance = cpu_to_le16((le16_to_cpu(
				ctx->mrec->next_attr_instance) + 1) & 0xffff);
		a->value_length = cpu_to_le32(label_len);
		a->value_offset = a->name_offset;
		a->resident_flags = 0;
		a->reservedR = 0;
	}
	memcpy((u8*)a + le16_to_cpu(a->value_offset), new_label, label_len);
	if (ntfs_inode_sync(vol->vol_ni)) {
		ntfs_log_perror("Error writing MFT Record to disk");
		goto err_out;
	}
	result = 0;
err_out:
	ntfs_attr_put_search_ctx(ctx);
	free(new_label);
	return result;
}
