/**
 *
 * TODO: description
 * 
 * This file is a part of USB SCSI CAM for Haiku OS.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 * 	
 * 	
 */
/** handling SCSI data-buffer (both usual "plain" and scatter/gather ones) */

#include <string.h>

#include "usb_scsi.h" 

#include <malloc.h>
#include "tracing.h" 
#include "sg_buffer.h"

/**
	\fn:init_sg_buffer
	TODO!!!
*/
status_t
init_sg_buffer(sg_buffer *sgb, CCB_SCSIIO *ccbio)
{
	status_t status = B_NO_INIT;
	if(0 != sgb){
		memset(sgb, 0, sizeof(sg_buffer));
		/* setup scatter/gather if exists in CCBIO*/
		if(ccbio->cam_ch.cam_flags & CAM_SCATTER_VALID) {
			sgb->piov	= (iovec *) ccbio->cam_data_ptr;
			sgb->count = ccbio->cam_sglist_cnt;
		} else {
			sgb->iov.iov_base = (iovec *) ccbio->cam_data_ptr;
			sgb->iov.iov_len	= ccbio->cam_dxfer_len;
			sgb->piov	= &sgb->iov;
			sgb->count = 1;
		}
		status = B_OK;
	} else {
		TRACE_ALWAYS("init_sg_buffer fatal: not initialized!\n");
	}
	return status;
}

/**
	\fn:alloc_sg_buffer
	TODO!!!
*/
status_t
realloc_sg_buffer(sg_buffer *sgb, size_t size)
{
	status_t status = B_NO_INIT;
	if(0 != sgb){
		/* NOTE: no check for previously allocations! May be dangerous!*/
		void *ptr = malloc(size);
		status = (0 != ptr) ? B_OK : B_NO_MEMORY;
		if(B_OK == status) {
			memset(ptr, 0, size);
			sgb->iov.iov_base = (iovec *)ptr;
			sgb->iov.iov_len	= size;
			sgb->piov	= &sgb->iov;
			sgb->count = 1;
			sgb->b_own = true;
			status = B_OK;
		}
	} else {
		TRACE_ALWAYS("realloc_sg_buffer fatal: not initialized!\n");
	}
	return status;
}

status_t
sg_access_byte(sg_buffer *sgb, off_t offset, uchar *byte, bool b_set)
{
	status_t status = B_ERROR; 
	int i = 0;
	for(; i < sgb->count; i++){
		if(offset >= sgb->piov[i].iov_len){
			offset -= sgb->piov[i].iov_len;
		} else {
			uchar *ptr = (uchar *)sgb->piov[i].iov_base;
			if(b_set){
				ptr[offset] = *byte;
			}else{
				*byte = ptr[offset];
			}
			status = B_OK;
			break;
		}
	}
	return status;
}

status_t
sg_memcpy(sg_buffer *d_sgb, off_t d_offset,
					sg_buffer *s_sgb, off_t s_offset, size_t size)
{
	status_t status = B_NO_INIT;
	while(0 != d_sgb && 0 != s_sgb){ 
		uchar *d_ptr = 0;
		uchar *s_ptr = 0;
		status = B_ERROR;
		if(1 == d_sgb->count){
			d_ptr = d_sgb->piov->iov_base + d_offset;
			if((d_offset + size) > d_sgb->piov->iov_len){
				TRACE_ALWAYS("sg_memcpy fatal: dest buffer overflow: has:%d req:%d\n",
							 d_sgb->piov->iov_len, d_offset + size);
				break;
			}
		}
		if(1 == s_sgb->count){
			s_ptr = s_sgb->piov->iov_base + s_offset;
			if((s_offset + size) > s_sgb->piov->iov_len){
				TRACE_ALWAYS("sg_memcpy fatal: src buffer overflow: has:%d req:%d\n",
							 s_sgb->piov->iov_len, s_offset + size);
				break;
			}
		}
		if(0 != d_ptr && 0 != s_ptr){
			memcpy(d_ptr, s_ptr, size);
		} else {
			uchar byte = 0;
			int i = 0;
			for(; i < size; i++){
				status = sg_access_byte(s_sgb, s_offset + i, &byte, false);
				if(B_OK == status)
					status = sg_access_byte(d_sgb, d_offset + i, &byte, true);
				if(B_OK != status){
					TRACE_ALWAYS("sg_memcpy fatal: dest:%d src:%d size:%d/%d\n",
										 d_offset, s_offset, i, size);
					break;
				}
			}
		}
		status = B_OK;
		break;
	}
	if(B_NO_INIT == status)
		TRACE_ALWAYS("sg_memcpy fatal: not initialized");
	return status;
}

/**
	\fn:free_sg_buffer
	TODO!!!
*/
void
free_sg_buffer(sg_buffer *sgb)
{
	if(0 != sgb && sgb->b_own){
		int i = 0;
		for(; i < sgb->count; i++){
			free(sgb->piov[i].iov_base);
		}
		memset(sgb, 0, sizeof(sg_buffer));
	}
}

status_t
sg_buffer_len(sg_buffer *sgb, size_t *size)
{
	status_t status = B_NO_INIT;
	if(0!=sgb && 0!=size){
		int i = 0; 
		*size = 0;
		for(; i < sgb->count; i++){
			*size += sgb->piov[i].iov_len;
		}
		status = B_OK;
	} else {
		TRACE_ALWAYS("sg_buffer_len fatal: not initialized (sgb:%x/size:%x)", sgb, size);
	}
	return status;
}

