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

#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdef.h"
#include "compile.h"
#include "private.h"
#include "parser.hpp"

char lexfile[B_PATH_NAME_LENGTH];

static BEntry entry;
static BFile file;
BResources rsrc;
const char* rsrc_file;

static jmp_buf abort_jmp;  // for aborting compilation

// When it encounters an error, the parser immediately aborts (we don't try
// error recovery). But its parse stack may still contain malloc'ed objects.
// We keep track of these memory blocks in the mem_list, so we can properly
// free them in case of an error (which is the polite thing to do since we
// are a shared library). If the compilation was successful, then mem_list
// should be empty. In DEBUG mode we dump some statistics to verify that the
// parser properly frees up objects when it is done with them.

#ifdef DEBUG
struct mem_t {
	void* ptr;
	char* file;
	int32 line;
};

typedef std::list<mem_t> mem_list_t;
typedef mem_list_t::iterator mem_iter_t;

static mem_list_t mem_list;
#else
static ptr_list_t mem_list;
#endif


class AddIncludeDir {
	public:
		AddIncludeDir(const char *file);
		~AddIncludeDir();

	private:
		BPath fPath;
};


AddIncludeDir::AddIncludeDir(const char *file)
{
	// ignore the special stdin file
	if (!strcmp(file, "-"))
		return;

	if (fPath.SetTo(file) != B_OK
		|| fPath.GetParent(&fPath) != B_OK) {
		fPath.Unset();
		return;
	}

	rdef_add_include_dir(fPath.Path(), false);
}


AddIncludeDir::~AddIncludeDir()
{
	if (fPath.InitCheck() == B_OK)
		rdef_remove_include_dir(fPath.Path());
}


//	#pragma mark -


void *
alloc_mem(size_t size)
{
	void *ptr = malloc(size);  // can be 0
	if (ptr == NULL)
		abort_compile(B_NO_MEMORY, "out of memory");

#ifdef DEBUG
	mem_t mem;
	mem.ptr = ptr;
	mem.file = strdup(lexfile);
	mem.line = yylineno;
	mem_list.push_front(mem);
#else
	mem_list.push_front(ptr);
#endif

	return ptr;
}


void
free_mem(void *ptr)
{
	if (ptr != NULL) {
#ifdef DEBUG
		for (mem_iter_t i = mem_list.begin(); i != mem_list.end(); ++i) {
			if (i->ptr == ptr) {
				free(i->ptr);
				free(i->file);
				mem_list.erase(i);
				return;
			}
		}
#else
		mem_list.remove(ptr);
		free(ptr);
#endif
	}
}


static void
clean_up_mem()
{
#ifdef DEBUG
	if (mem_list.size() != 0)
		printf("mem_list leaks %ld objects\n", mem_list.size());

	for (mem_iter_t i = mem_list.begin(); i != mem_list.end(); ) {
		printf("%p allocated at %s:%ld\n", i->ptr, i->file, i->line);	
		free(i->ptr);
		free(i->file);
		i = mem_list.erase(i);
	}
#else
	free_ptr_list(mem_list);
#endif
}


void
abort_compile(status_t err, const char *format, ...)
{
	va_list ap;

	rdef_err = err;
	rdef_err_line = yylineno;
	strcpy(rdef_err_file, lexfile);

	va_start(ap, format);
	vsprintf(rdef_err_msg, format, ap);
	va_end(ap);

	abort_compile();
}


void
abort_compile()
{
	longjmp(abort_jmp, 1);
}


static void
compile_file(char *file)
{
	strcpy(lexfile, file);

	// "-" means reading from stdin
	if (strcmp(file, "-"))
		yyin = fopen(lexfile, "r");
	else
		yyin = stdin;

	if (yyin == NULL) {
		strcpy(rdef_err_file, lexfile);
		rdef_err = RDEF_FILE_NOT_FOUND;
		return;
	}

	init_lexer();
	init_parser();

	if (setjmp(abort_jmp) == 0) {
		yyparse();
	}

	// About error handling: If the bison-generated parser encounters 
	// a syntax error, it calls yyerror(), aborts parsing, and returns 
	// from yyparse(). For other kinds of errors (semantics, problem 
	// writing to BResources, etc), we bail out with a longjmp(). From 
	// then on, we can tell success or failure by looking at rdef_err.

	clean_up_lexer();
	clean_up_parser();
	clean_up_mem();
}


static status_t
open_output_file()
{
	status_t err = entry.SetTo(rsrc_file, true);
	if (err == B_OK) {
		uint32 openMode = B_READ_WRITE | B_CREATE_FILE;
		bool clobber = false;

		if (!(flags & RDEF_MERGE_RESOURCES)) {
			openMode |= B_ERASE_FILE;
			clobber   = true;
		}

		err = file.SetTo(&entry, openMode);
		if (err == B_OK)
			err = rsrc.SetTo(&file, clobber);
	}

	return err;
}


static void
close_output_file()
{
	if (rdef_err == B_OK || (flags & RDEF_MERGE_RESOURCES) != 0)
		rsrc.Sync();
	else
		entry.Remove();  // throw away output file

	file.Unset();
	entry.Unset();
}


status_t
rdef_compile(const char *outputFile)
{
	clear_error();

	if (outputFile == NULL || outputFile[0] == '\0') {
		rdef_err = B_BAD_VALUE;
		return rdef_err;
	}

	rsrc_file = outputFile;
	rdef_err = open_output_file();
	if (rdef_err != B_OK)
		return rdef_err;

	for (ptr_iter_t i = input_files.begin(); 
			(i != input_files.end()) && (rdef_err == B_OK); ++i) {
		char *path = (char *)*i;

		AddIncludeDir add(path);
		compile_file(path);
	}

	close_output_file();
	return rdef_err;
}

