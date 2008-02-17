/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_PARTITION_SUPPORT_H
#define _FSSH_PARTITION_SUPPORT_H

#include "fssh_defs.h"
#include "fssh_stat.h"


namespace FSShell {


void	add_file_restriction(const char* fileName, fssh_off_t startOffset,
			fssh_off_t endOffset);

void	restricted_file_opened(int fd);
void	restricted_file_duped(int oldFD, int newFD);
void	restricted_file_closed(int fd);

int		restricted_file_restrict_io(int fd, fssh_off_t& pos, fssh_off_t size);
void	restricted_file_restrict_stat(struct fssh_stat* st);


}	// namespace FSShell


#endif	// _FSSH_PARTITION_SUPPORT_H
