/*
 * Copyright 2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef FD_PATH_H
#define FD_PATH_H


#include <OS.h>

#include <fd.h>
#include <khash.h>
#include <util/SinglyLinkedList.h>


class PathInfo : public SinglyLinkedListLinkImpl<PathInfo> {
public:
			void				SetTo(file_descriptor* fd, ino_t directory,
									const char* name);

			file_descriptor*	descriptor;
			ino_t				directory;
			char				filename[B_FILE_NAME_LENGTH];
};


typedef SinglyLinkedList<PathInfo>	PathInfoList;


struct fd_paths {
	fd_paths*			next;
	vnode*				node;
	PathInfoList		paths;
};


bool				init_fd_paths_hash_table();
void				dump_fd_paths_hash_table();

void				read_lock_fd_paths();
void				read_unlock_fd_paths();

//! lookup_fd_paths needs to be locked
fd_paths*			lookup_fd_paths(dev_t device, ino_t node);

status_t			insert_fd_path(vnode* node, int fd, bool kernel,
						ino_t directory, const char* filename);
status_t			remove_fd_path(file_descriptor*	descriptor);
status_t			move_fd_path(dev_t device, ino_t node, const char *fromName,
						ino_t newDirectory, const char* newName);

#endif // FD_PATH_H
