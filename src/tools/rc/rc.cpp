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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdef.h"

#define TITLE "Haiku Resource Compiler 1.1"

#define DEFAULT_OUT_RSRC  "out.rsrc"
#define DEFAULT_OUT_RDEF  "out.rdef"

static char* prog_name;

static bool quiet = false;
static bool should_decompile = false;
static uint32 flags = 0;

static char output_file[B_PATH_NAME_LENGTH] = { 0 };

//------------------------------------------------------------------------------

void warn(const char* format, ...)
{
	va_list ap;

	if (!quiet)
	{
		fprintf(stderr, "Warning! ");
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}
}

//------------------------------------------------------------------------------

void error(const char* format, ...)
{
	va_list ap;

	if (!quiet)
	{
		fprintf(stderr, "Error! ");
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}

	exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------

static void usage()
{
	printf(

"%s\n\n"
"To compile an rdef script into a resource file:\n"
"    %s [options] [-o <file>] <file>...\n\n"
"To convert a resource file back into an rdef script:\n"
"    %s [options] [-o <file>] -d <file>...\n\n"
"Options:\n"
"    -d --decompile       create an rdef script from a resource file\n"
"       --auto-names      construct resource names from ID symbols\n"
"    -h --help            show this message\n"
"    -I --include <dir>   add <dir> to the list of include paths\n"
"    -m --merge           do not erase existing contents of output file\n"
"    -o --output          specify output file name, default is out.xxx\n"
"    -q --quiet           do not display any error messages\n"
"    -V --version         show software version and license\n",

		TITLE, prog_name, prog_name);

	exit(EXIT_SUCCESS);
}

//------------------------------------------------------------------------------

static void version()
{
	printf(
		"%s\n\n"
		"Copyright (c) 2003 Matthijs Hollemans\n\n"

"Permission is hereby granted, free of charge, to any person obtaining a\n"
"copy of this software and associated documentation files (the \"Software\"),\n"
"to deal in the Software without restriction, including without limitation\n"
"the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
"and/or sell copies of the Software, and to permit persons to whom the\n"
"Software is furnished to do so, subject to the following conditions:\n\n"

"The above copyright notice and this permission notice shall be included in\n"
"all copies or substantial portions of the Software.\n\n"

"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER\n"
"DEALINGS IN THE SOFTWARE.\n",

		TITLE);

	exit(EXIT_SUCCESS);
}

//------------------------------------------------------------------------------

static void get_program_name(char* argv[])
{
	char* p;
	prog_name = ((p = strrchr(argv[0], '/')) ? p + 1 : *argv);
}

//------------------------------------------------------------------------------

static void parse_options(int argc, char* argv[])
{
	int32 args_left = argc - 1;

	for (int32 i = 1; i < argc; ++i)
	{
		if ((strcmp(argv[i], "-o") == 0)
		||  (strcmp(argv[i], "--output") == 0))
		{
			if (i + 1 >= argc)
			{
				error("%s should be followed by a file name", argv[i]);
			}

			strcpy(output_file, argv[i + 1]);

			argv[i] = NULL;
			argv[i + 1] = NULL;
			++i; args_left -= 2;
		}
		else if ((strcmp(argv[i], "-I") == 0)
			 ||  (strcmp(argv[i], "--include") == 0))
		{
			if (i + 1 >= argc)
			{
				error("%s should be followed by a directory name", argv[i]);
			}

			rdef_add_include_dir(argv[i + 1]);

			argv[i] = NULL;
			argv[i + 1] = NULL;
			++i; args_left -= 2;
		}
		else if ((strcmp(argv[i], "-d") == 0)
			 ||  (strcmp(argv[i], "--decompile") == 0))
		{
			should_decompile = true;
			argv[i] = NULL;
			--args_left;
		}
		else if ((strcmp(argv[i], "-m") == 0)
			 ||  (strcmp(argv[i], "--merge") == 0))
		{
			flags |= RDEF_MERGE_RESOURCES;
			argv[i] = NULL;
			--args_left;
		}
		else if (strcmp(argv[i], "--auto-names") == 0)
		{
			flags |= RDEF_AUTO_NAMES;
			argv[i] = NULL;
			--args_left;
		}
		else if ((strcmp(argv[i], "-q") == 0)
			 ||  (strcmp(argv[i], "--quiet") == 0))
		{
			quiet = true;
			argv[i] = NULL;
			--args_left;
		}
		else if ((strcmp(argv[i], "-h") == 0)
			 ||  (strcmp(argv[i], "--help") == 0))
		{
			usage();
		}
		else if ((strcmp(argv[i], "-V") == 0)
			 ||  (strcmp(argv[i], "--version") == 0))
		{
			version();
		}
		else if (argv[i][0] == '-')
		{
			error("unknown option %s", argv[i]);
			argv[i] = NULL;
			--args_left;
		}
	}

	if (args_left < 1)
	{
		error("no input files");
	}

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] != NULL)
		{
			rdef_add_input_file(argv[i]);
		}
	}
}

//------------------------------------------------------------------------------

static bool has_extension(char* str, char* ext)
{
	size_t str_len = strlen(str);
	size_t ext_len = strlen(ext);

	if (str_len > ext_len)
	{
		return (strcmp(str + str_len - ext_len, ext) == 0);
	}

	return false;
}

//------------------------------------------------------------------------------

static void compile()
{
	if (output_file[0] == '\0')
	{
		rdef_compile(DEFAULT_OUT_RSRC);
	}
	else
	{
		if (!has_extension(output_file, ".rsrc"))
		{
			strcat(output_file, ".rsrc");
		}

		rdef_compile(output_file);
	}
}

//------------------------------------------------------------------------------

static void decompile()
{
	if (output_file[0] == '\0')
	{
		rdef_decompile(DEFAULT_OUT_RDEF);
	}
	else
	{
		if (!has_extension(output_file, ".rdef"))
		{
			strcat(output_file, ".rdef");
		}

		rdef_decompile(output_file);
	}
}

//------------------------------------------------------------------------------

static void report_error()
{
	switch (rdef_err)
	{
		case RDEF_COMPILE_ERR:
			error("%s:%ld %s", rdef_err_file, rdef_err_line, rdef_err_msg);
			break;

		case RDEF_FILE_NOT_FOUND:
			error("%s not found", rdef_err_file);
			break;

		case RDEF_NO_RESOURCES:
			error("%s is not a resource file", rdef_err_file);
			break;

		case RDEF_WRITE_ERR:
			error("error writing to %s (%s)", rdef_err_file, rdef_err_msg);
			break;

		case B_NO_MEMORY:
			error("out of memory");
			break;

		case B_ERROR:
		default:
			error("unknown error: %lx (%s)", rdef_err, strerror(rdef_err));
			break;
	}
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	rdef_add_include_dir(".");  // look in current dir first

	get_program_name(argv);
	parse_options(argc, argv);

	rdef_set_flags(flags);

	if (should_decompile)
	{
		decompile();
	}
	else
	{
		compile();
	}

	rdef_free_input_files();
	rdef_free_include_dirs();

	if (rdef_err != B_OK)
	{
		report_error();
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
