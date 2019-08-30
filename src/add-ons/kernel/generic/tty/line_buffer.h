/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef LINE_BUFFER_H
#define LINE_BUFFER_H


#include <OS.h>


struct line_buffer {
	int32	first;
	size_t	in;
	size_t	size;
	char	*buffer;
};

status_t init_line_buffer(struct line_buffer &buffer, size_t size);
status_t uninit_line_buffer(struct line_buffer &buffer);
status_t clear_line_buffer(struct line_buffer &buffer);
int32 line_buffer_readable(struct line_buffer &buffer);
int32 line_buffer_readable_line(struct line_buffer &buffer, char eol, char eof);
int32 line_buffer_writable(struct line_buffer &buffer);
ssize_t line_buffer_user_read(struct line_buffer &buffer, char *data,
			size_t length, char eof = 0, bool* hitEOF = NULL);
status_t line_buffer_getc(struct line_buffer &buffer, char *_c);
status_t line_buffer_putc(struct line_buffer &buffer, char c);
status_t line_buffer_ungetc(struct line_buffer &buffer, char *c);
bool line_buffer_tail_getc(struct line_buffer &buffer, char *c);

#endif	/* LINE_BUFFER_H */
