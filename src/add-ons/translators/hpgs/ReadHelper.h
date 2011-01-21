/*
 * Copyright 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef READ_HELPER_H
#define READ_HELPER_H

#include <errno.h>
#include <hpgs.h>
#include <DataIO.h>

#include "StreamBuffer.h"

typedef struct my_hpgs_istream_st
{
  StreamBuffer *buffer;
  status_t iseof;
} my_hpgs_istream;


static int 
positionio_getc(my_hpgs_istream *stream)
{
	unsigned char value;
	status_t error;
	stream->iseof = 0;
	error = stream->buffer->Read((void *)&value, sizeof(unsigned char));
	if (error > B_OK)
		return value;
	stream->iseof = EOF;
	return EOF;
}


static int 
positionio_ungetc(int c, my_hpgs_istream *stream)
{
	return stream->buffer->Seek(-1, SEEK_CUR);
}


static int 
positionio_close(my_hpgs_istream *stream)
{
	return B_OK;
}


static int 
positionio_eof(my_hpgs_istream *stream)
{
	return stream->iseof;
}


static int 
positionio_error(my_hpgs_istream *stream)
{
	return errno;
}


static int 
positionio_tell(my_hpgs_istream *stream, size_t *pos)
{
	*pos = stream->buffer->Position();
	return 0;
}


static int 
positionio_seek(my_hpgs_istream *stream, size_t pos)
{
	stream->buffer->Seek(pos, SEEK_SET);
	return 0;
}


static int 
positionio_seekend(my_hpgs_istream *stream, size_t pos)
{
	stream->buffer->Seek(pos, SEEK_END);
	return 0;
}


static size_t 
positionio_read(void *ptr, size_t size, size_t nmemb, my_hpgs_istream *stream)
{
	unsigned char *iptr = (unsigned char *)ptr;
	size_t i = 0;
	for (; i < nmemb; i++) {
		if (size != (size_t)stream->buffer->Read(iptr, size))
			break;
		iptr += size;
	}
	return i;
}


static hpgs_istream_vtable positionio_vtable =
  {
    (hpgs_istream_getc_func_t)    positionio_getc,
    (hpgs_istream_ungetc_func_t)  positionio_ungetc,
    (hpgs_istream_close_func_t)   positionio_close,
    (hpgs_istream_iseof_func_t)   positionio_eof,
    (hpgs_istream_iserror_func_t) positionio_error,
    (hpgs_istream_seek_func_t)    positionio_seek,
    (hpgs_istream_tell_func_t)    positionio_tell,
    (hpgs_istream_read_func_t)    positionio_read,
    (hpgs_istream_seekend_func_t) positionio_seekend
  };

hpgs_istream *
hpgs_new_wrapper_istream(BPositionIO *stream)
{
	hpgs_istream *ret = (hpgs_istream *)malloc(sizeof(hpgs_istream));

	if (!ret)
		return NULL;

	ret->stream = (my_hpgs_istream *)malloc(sizeof(my_hpgs_istream));
	ret->vtable = &positionio_vtable;
	((my_hpgs_istream *)ret->stream)->iseof = 0;
	((my_hpgs_istream *)ret->stream)->buffer = new StreamBuffer(stream, 2048);

	return ret;
}


void
hpgs_free_wrapper_istream(hpgs_istream *istream)
{
	delete ((my_hpgs_istream *)istream->stream)->buffer;
	free(istream->stream);
	free(istream);
}

#endif // READ_HELPER_H
