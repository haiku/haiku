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

#ifndef PRIVATE_H
#define PRIVATE_H

#include <list>
#include <stdio.h>

typedef std::list<void*> ptr_list_t;
typedef ptr_list_t::iterator ptr_iter_t;

// Configuration options for the (de)compiler.
extern uint32 flags;

// Where to look for #include files.
extern ptr_list_t include_dirs;

// The list of input files.
extern ptr_list_t input_files;

// free()'s all the elements from a list.
void free_ptr_list(ptr_list_t& list);

// Resets the rdef_err_* variables.
void clear_error();

// Scans all include dirs for the specified file.
bool open_file_from_include_dir(const char* filename, char* outname);

#endif // PRIVATE_H
