/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/* A BMallocIO similar structure but with less overhead */

#ifndef _SIMPLE_MALLOC_IO_H_
#define _SIMPLE_MALLOC_IO_H_

#include <malloc.h>

namespace BPrivate {

class BSimpleMallocIO {
public:
						BSimpleMallocIO(size_t size)
							:	fSize(size)
						{
							fBuffer = (char *)malloc(size);
						}

						~BSimpleMallocIO()
						{
							free(fBuffer);
						}

		void			Read(void *buffer)
						{
							memcpy(buffer, fBuffer, fSize);
						}

		void			Read(void *buffer, size_t size)
						{
							memcpy(buffer, fBuffer, size);
						}

		void			ReadAt(off_t pos, void *buffer, size_t size)
						{
							memcpy(buffer, fBuffer + pos, size);
						}

		void			Write(const void *buffer)
						{
							memcpy(fBuffer, buffer, fSize);
						}

		void			Write(const void *buffer, size_t size)
						{
							memcpy(fBuffer, buffer, size);
						}

		void			WriteAt(off_t pos, const void *buffer, size_t size)
						{
							memcpy(fBuffer + pos, buffer, size);
						}

		status_t		SetSize(off_t size)
						{
							fBuffer = (char *)realloc(fBuffer, size);
							if (!fBuffer)
								return B_NO_MEMORY;

							fSize = size;
							return B_OK;
						}

		char			*Buffer()
						{
							return fBuffer;
						}

		size_t			BufferLength()
						{
							return fSize;
						}

private:
		char			*fBuffer;
		size_t			fSize;
};

} // namespace BPivate

#endif // _SIMPLE_MALLOC_IO_H_
