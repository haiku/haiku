/**
 *
 * TODO: description
 *
 * This file is a part of USB SCSI CAM for Haiku.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 *
 *
 */
/** handling SCSI data-buffer (both usual "plain" and scatter/gather ones) */

#ifndef _SG_BUFFER_H_
	#define _SG_BUFFER_H_

#ifndef _IOVEC_H
	#include <iovec.h>
#endif/*_IOVEC_H*/

/**
	\struct:_sg_buffer
*/
typedef struct _sg_buffer{
	iovec	iov;	 /**< to avoid extra memory allocations */
	iovec *piov;	/**< ptr to scatter/gather list, default is equal to &iov */
	int		count; /**< count of scatter/gather vector entries */
	bool	 b_own; /**< was allocated - must be freed */
} sg_buffer;

#define member_offset(__type, __member) ((size_t)&(((__type *)0)->__member))
#define member_size(__type, __member) sizeof(((__type *)0)->__member)

status_t init_sg_buffer(sg_buffer *sgb, CCB_SCSIIO *ccbio);
status_t realloc_sg_buffer(sg_buffer *sgb, size_t size);
status_t sg_buffer_len(sg_buffer *sgb, size_t *size);
status_t sg_access_byte(sg_buffer *sgb, off_t offset, uchar *byte, bool b_set);
status_t sg_memcpy(sg_buffer *dest_sgb, off_t dest_offset,
									 sg_buffer *src_sgb,	off_t src_offset, size_t size);
void free_sg_buffer(sg_buffer *sgb);

#endif /*_SG_BUFFER_H_*/

