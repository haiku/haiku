/*
 * Copyright (c) 2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rdef.h"
#include "private.h"

ptr_list_t include_dirs;
ptr_list_t input_files;

uint32 flags = 0;

status_t rdef_err;
int32 rdef_err_line;
char rdef_err_file[B_PATH_NAME_LENGTH];
char rdef_err_msg[1024];


void
free_ptr_list(ptr_list_t &list)
{
	for (ptr_iter_t i = list.begin(); i != list.end(); ++i) {
		free(*i);
	}

	list.clear();
}


int32
rdef_get_version()
{
	return 2;
}


status_t
rdef_add_include_dir(const char *dir, bool toEndOfList)
{
	clear_error();

	char *path = (char *)malloc(strlen(dir) + 2);
	if (path == NULL) {
		rdef_err = B_NO_MEMORY;
		return B_NO_MEMORY;
	}

	strcpy(path, dir);
	strcat(path, "/");

	if (toEndOfList)
		include_dirs.push_back(path);
	else
		include_dirs.push_front(path);

	return B_OK;
}


status_t
rdef_remove_include_dir(const char *dir)
{
	size_t length = strlen(dir);
	bool noSlash = false;
	if (dir[length - 1] != '/')
		noSlash = true;

	for (ptr_iter_t i = include_dirs.begin(); i != include_dirs.end(); ++i) {
		char *path = (char *)*i;

		if (!strncmp(dir, path, length)
			&& path[length + (noSlash ? 1 : 0)] == '\0') {
			// we found the entry in the list, let's remove it
			include_dirs.erase(i);
			free(path);
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


void
rdef_free_include_dirs()
{
	free_ptr_list(include_dirs);
}


status_t
rdef_add_input_file(const char *file)
{
	clear_error();

	char *temp = strdup(file);
	if (temp == NULL) {
		rdef_err = B_NO_MEMORY;
		return B_NO_MEMORY;
	}

	input_files.push_back(temp);
	return B_OK;
}


void
rdef_free_input_files()
{
	free_ptr_list(input_files);
}


void
rdef_set_flags(uint32 flags_)
{
	flags = flags_;
}


void
rdef_clear_flags()
{
	flags = 0;
}


void 
clear_error()
{
	rdef_err = B_OK;
	rdef_err_line = 0;
	rdef_err_file[0] = '\0';
	rdef_err_msg[0] = '\0';
}


bool
open_file_from_include_dir(const char *filename, char *outname)
{
	for (ptr_iter_t i = include_dirs.begin(); i != include_dirs.end(); ++i) {
		char tmpname[B_PATH_NAME_LENGTH];
		strcpy(tmpname, (char *)*i);
		strcat(tmpname, filename);

		if (access(tmpname, R_OK) == 0) {
			strcpy(outname, tmpname);
			return true;
		}
	}

	return false;
}

