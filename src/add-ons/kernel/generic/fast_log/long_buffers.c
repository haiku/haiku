/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Fast logging facilities.

	Long buffer.

	The internal log buffer uses encoded events and 
	cannot be decoded on demand as the event codes are
	passed by the client and can be changed each time
	the client connects. So, we store the decoded events
	in long buffers which can be read from directly.

	The long buffer is split up into several smaller
	buffers that are allocated and freed on demand
	to save space.
*/


#include "fast_log_int.h"
#include "dl_list.h"

#include <lock.h>

#include <string.h>


// one long buffer
typedef struct tagfast_log_long_buffer {
	struct tagfast_log_long_buffer *next, *prev;
	// content
	char *data;
	// offset for writing
	size_t write_offset;
	// offset for reading
	size_t read_offset;
	// size of buffer
	size_t size;
} fast_log_long_buffer;

static fast_log_long_buffer 
	// list of long buffers
	*fast_log_long_buffers, 
	// current buffer to write to
	// (NULL if all long buffers are full)
	*fast_log_cur_long_buffer;
	
static uint
	// current number of long buffers 
	fast_log_num_long_buffers, 
	// maximum number of long buffers
	fast_log_max_long_buffers;
	
// size of one long buffer
static size_t fast_log_long_buffer_size;
// true, if all long buffers are full
// (used to insert a proper message in logging)
static bool fast_log_long_overflow = false;
// grab this when you want to access a long buffer.
// you must make sure that no swapping occurs when holding this lock
static benaphore fast_log_long_buffer_ben;
// grab this when you read out long buffers.
// you are allowed to swap when you hold this lock;
// must be acquired before fast_log_long_buffer_ben
static benaphore fast_log_long_read_ben;


/** flush current long log buffer and allocate new one */

static status_t
fast_log_inc_long_buffer(void)
{
	fast_log_long_buffer *buffer;

	fast_log_cur_long_buffer = NULL;

	// allocate new buffer if allowed and possible
	if (fast_log_num_long_buffers >= fast_log_max_long_buffers)
		return B_ERROR;

	buffer = malloc(sizeof(*buffer) + fast_log_long_buffer_size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	memset(buffer, 0, sizeof(*buffer));
	buffer->write_offset = 0;
	buffer->read_offset = 0;
	buffer->size = fast_log_long_buffer_size;
	buffer->data = (char *)(buffer + 1);

	ADD_CDL_LIST_TAIL(buffer, fast_log_long_buffer, fast_log_long_buffers, );

	fast_log_cur_long_buffer = buffer;

	return B_OK;
}


/** store string in long log, internal */

static status_t
fast_log_write_long_log_intern(const char *str)
{
	size_t len;

	len = strlen(str);

	while (len > 0) {
		fast_log_long_buffer *buffer;
		size_t cur_len;

		// get buffer, alloc a new one if there isn't one
		if (fast_log_cur_long_buffer == NULL) {
			if (fast_log_inc_long_buffer() != B_OK)
				return B_ERROR;
		}

		buffer = fast_log_cur_long_buffer;

		cur_len = min(len, buffer->size - buffer->write_offset);
		memcpy(buffer->data + buffer->write_offset, str, cur_len);

		buffer->write_offset += cur_len;
		len -= cur_len;
		str += cur_len;

		if (buffer->write_offset == buffer->size) {
			// next iteration will get a new one
			fast_log_cur_long_buffer = NULL;
		}
	}

	return B_OK;
}


/** store string in long log */

void
fast_log_write_long_log(const char *str)
{
	benaphore_lock(&fast_log_long_buffer_ben);

	// write pending "overflow" message	
	if (fast_log_long_overflow) {
		if (fast_log_write_long_log_intern("<BUFFER OVERFLOW, increase max_long_buffers>\n") != B_OK)
			goto err;

		fast_log_long_overflow = false;
	}

	if (fast_log_write_long_log_intern(str) != B_OK)
		fast_log_long_overflow = true;

err:	
	benaphore_unlock(&fast_log_long_buffer_ben);
}


/** free (first) long buffer */

static void
fast_log_free_long_buffer(void)
{
	fast_log_long_buffer *buffer = fast_log_long_buffers;

	REMOVE_CDL_LIST(buffer, fast_log_long_buffers, );

	free(buffer);
}


/** read data from long buffers */

size_t
fast_log_read_long(char *data, size_t len)
{
	size_t left_bytes = len;

	// fill long buffer
	fast_log_flush_buffer();

	// make sure noone reads log concurrently
	benaphore_lock(&fast_log_long_read_ben);
	benaphore_lock(&fast_log_long_buffer_ben);

	while (left_bytes > 0 && fast_log_long_buffers != NULL) {
		size_t cur_len;
		fast_log_long_buffer *buffer;

		buffer = fast_log_long_buffers;

		cur_len = min(left_bytes, buffer->write_offset - buffer->read_offset);
		if (cur_len == 0)
			break;

		// "data" may be pagable, so release lock during memcpy
		benaphore_unlock(&fast_log_long_buffer_ben);

		memcpy(data, buffer->data + buffer->read_offset, cur_len);

		benaphore_lock(&fast_log_long_buffer_ben);

		buffer->read_offset += cur_len;
		data += cur_len;
		left_bytes -= cur_len;

		// once a buffer is completely read, free it
		if (buffer->read_offset == buffer->size) {
			fast_log_free_long_buffer();
			buffer = fast_log_long_buffers;
		}
	}

	benaphore_unlock(&fast_log_long_buffer_ben);
	benaphore_unlock(&fast_log_long_read_ben);
	
	return len - left_bytes;
}


/** cleanup long buffers */

void
fast_log_uninit_long_buffer(void)
{
	while (fast_log_long_buffers != NULL)
		fast_log_free_long_buffer();

	benaphore_destroy(&fast_log_long_buffer_ben);
	benaphore_destroy(&fast_log_long_read_ben);
}


/** initialize long buffers.
 *	you must call "fast_log_uninit_long_buffer" on fail
 */

status_t
fast_log_init_long_buffer(void)
{
	status_t res;

	fast_log_long_buffer_ben.sem = -1;
	fast_log_long_read_ben.sem = -1;

	fast_log_long_buffers = fast_log_cur_long_buffer = NULL;
	fast_log_num_long_buffers = 0;

	fast_log_max_long_buffers = 256;
	fast_log_long_buffer_size = 16*1024;

	fast_log_long_overflow = false;

	if ((res = benaphore_init(&fast_log_long_buffer_ben, "fast_log_long_buffer_ben")) < 0)
		return res;

	if ((res = benaphore_init( &fast_log_long_read_ben, "fast_log_long_read_ben")) < 0)
		return res;

	return res;
}

