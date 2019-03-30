/**
 * @file rdef.h
 *
 * Public header file for the librdef shared library. Programs that want to
 * use librdef should include this file, and link to librdef.so.
 *
 * @author Copyright (c) 2003 Matthijs Hollemans
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

#ifndef RDEF_H
#define RDEF_H

#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

// bonefish: What was this needed for?
//#if __i386__
//# ifdef _BUILDING_RDEF
//#  define _IMPEXP_RDEF __declspec(dllexport)
//# else
//#  define _IMPEXP_RDEF __declspec(dllimport)
//# endif
//#else
# define _IMPEXP_RDEF
//#endif

/**
 * Whether to overwrite or merge with the output file. With this flag, the
 * compiler will add the resources to the output file (if it exists). When
 * something goes wrong, the output file is not deleted (although it may
 * already contain some of the new resources). The default action is to clobber
 * the contents of the output file if it already existed, and the file is
 * deleted in case of an error. Not used for decompiling.
 */
#define RDEF_MERGE_RESOURCES  (1 << 0)

/**
 * Whether to generate resource names from symbolic identifiers, or vice versa.
 * With this flag, the compiler will use symbolic names in statements such as
 * "resource(R_Symbol)" to generate the resource name, in this case "R_Symbol".
 * Otherwise, you must explicitly specify a name, such as "resource(R_Symbol,
 * "Name") ..." If this option is set when decompiling, the decompiler will put
 * resource names that are valid C/C++ identifiers in an enum statement in a
 * new header file.
 */
#define RDEF_AUTO_NAMES  (1 << 1)

/** Error codes returned by the librdef functions. */
enum {
	/** Syntax error or other compiler error. */
	RDEF_COMPILE_ERR = B_ERRORS_END + 1,

	/** Could not find one of the input files. */
	RDEF_FILE_NOT_FOUND,

	/** One of the input files has nothing to decompile. */
	RDEF_NO_RESOURCES,

	/** Could not write the output file. */
	RDEF_WRITE_ERR,

	/* B_OK */
	/* B_ERROR */
	/* B_NO_MEMORY */
};

/** 
 * The result of the most recent (de)compilation. B_OK if the operation
 * was successful, a negative error code otherwise. This is the same as
 * the value returned by any of the rdef_xxx functions.
 */
_IMPEXP_RDEF extern status_t rdef_err;

/** 
 * The line number where compilation failed. Valid line numbers start
 * at 1. This is 0 if the error did not happen on a specific line.
 */
_IMPEXP_RDEF extern int32 rdef_err_line;

/** 
 * The file where the error occurred. This is an empty string if the
 * error did not happen in a specific file.
 */
_IMPEXP_RDEF extern char rdef_err_file[];

/** 
 * The error message from the compiler. This is an empty string if there
 * was no additional information to report.
 */
_IMPEXP_RDEF extern char rdef_err_msg[];

/** 
 * Returns the version number of the librdef API. You can use this to
 * check which functions and variables are available.
 */
_IMPEXP_RDEF int32 rdef_get_version();

/**
 * Adds a directory where the compiler will look for include files.
 * Typically, you want to add the current directory to this list.
 * Not used for decompilation.
 */
_IMPEXP_RDEF status_t rdef_add_include_dir(const char *dir, bool toEndOfList);

/**	Removes an include directory */
_IMPEXP_RDEF status_t rdef_remove_include_dir(const char *dir);

/** 
 * Frees the list of include directories. If you call rdef_add_include_dir(),
 * you should always call rdef_free_include_dirs() when the compiler is done.
 */
_IMPEXP_RDEF void rdef_free_include_dirs();

/**
 * Adds an input file for the compiler or decompiler. Input files are not
 * required to have a specific extension.
 */
_IMPEXP_RDEF status_t rdef_add_input_file(const char* file);

/**
 * Frees the list of input files. If you call rdef_add_input_file(), you
 * should always call rdef_free_input_files() when the compiler is done.
 */
_IMPEXP_RDEF void rdef_free_input_files();

/** Changes the configuration of the compiler or decompiler. */
_IMPEXP_RDEF void rdef_set_flags(uint32 flags);

/** Resets all the configuration options to their default values. */
_IMPEXP_RDEF void rder_clear_flags();

/** 
 * Invokes the rdef-to-rsrc compiler. Before you call rdef_compile(), you must
 * have specified at least one input file with rdef_add_input_file(), and one
 * include search path with rdef_add_include_dir().
 */
_IMPEXP_RDEF status_t rdef_compile(const char* output_file);

/** 
 * Invokes the rsrc-to-rdef decompiler. Just as with rdef_compile(), you must
 * first add at least one input file with rdef_add_input_file(). Include dirs
 * are not necessary, because the decompiler does not use them. The decompiler
 * writes at least an rdef script file. In addition, if the "auto names" option
 * is enabled it also writes a C/C++ header file. If these files already exist,
 * they will be overwritten.
 */
_IMPEXP_RDEF status_t rdef_decompile(const char* output_file);

#ifdef __cplusplus
}
#endif

#endif /* RDEF_H */
