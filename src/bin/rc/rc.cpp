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


extern const char *__progname;

static const char *kTitle = "Haiku Resource Compiler 1.1";
static const char *kProgramName = __progname;


static bool sQuiet = false;
static bool sDecompile = false;
static uint32 sFlags = 0;

static char sOutputFile[B_PATH_NAME_LENGTH] = { 0 };
static char *sFirstInputFile = NULL;


void
warn(const char *format, ...)
{
	va_list ap;

	if (!sQuiet) {
		fprintf(stderr, "%s: Warning! ", kProgramName);
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}
}


void
error(const char *format, ...)
{
	va_list ap;

	if (!sQuiet) {
		fprintf(stderr, "%s: Error! ", kProgramName);
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}

	exit(EXIT_FAILURE);
}


static void
usage()
{
	printf("%s\n\n"
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
		kTitle, kProgramName, kProgramName);

	exit(EXIT_SUCCESS);
}


static void
version()
{
	printf("%s\n\n"
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
		kTitle);

	exit(EXIT_SUCCESS);
}


static bool
has_extension(char *name, const char *ext)
{
	size_t nameLength = strlen(name);
	size_t extLength = strlen(ext);

	if (nameLength > extLength)
		return strcmp(name + nameLength - extLength, ext) == 0;

	return false;
}


static void
cut_extension(char *name, const char *ext)
{
	if (!has_extension(name, ext))
		return;

	name[strlen(name) - strlen(ext)] = '\0';
}


static void
add_extension(char *name, const char *ext)
{
	strlcat(name, ext, B_PATH_NAME_LENGTH);
}


static void
parse_options(int argc, char *argv[])
{
	int32 args_left = argc - 1;

	// ToDo: use getopt_long()

	for (int32 i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0
				|| strcmp(argv[i], "--output") == 0) {
			if (i + 1 >= argc)
				error("%s should be followed by a file name", argv[i]);

			strcpy(sOutputFile, argv[i + 1]);

			argv[i] = NULL;
			argv[i + 1] = NULL;
			++i; args_left -= 2;
		} else if (strcmp(argv[i], "-I") == 0
				|| strcmp(argv[i], "--include") == 0) {
			if (i + 1 >= argc)
				error("%s should be followed by a directory name", argv[i]);

			rdef_add_include_dir(argv[i + 1], true);

			argv[i] = NULL;
			argv[i + 1] = NULL;
			++i; args_left -= 2;
		} else if (strcmp(argv[i], "-d") == 0
				|| strcmp(argv[i], "--decompile") == 0) {
			sDecompile = true;
			argv[i] = NULL;
			--args_left;
		} else if (strcmp(argv[i], "-m") == 0
				|| strcmp(argv[i], "--merge") == 0) {
			sFlags |= RDEF_MERGE_RESOURCES;
			argv[i] = NULL;
			--args_left;
		} else if (strcmp(argv[i], "--auto-names") == 0) {
			sFlags |= RDEF_AUTO_NAMES;
			argv[i] = NULL;
			--args_left;
		} else if (strcmp(argv[i], "-q") == 0
				|| strcmp(argv[i], "--quiet") == 0) {
			sQuiet = true;
			argv[i] = NULL;
			--args_left;
		} else if (strcmp(argv[i], "-h") == 0
				|| strcmp(argv[i], "--help") == 0) {
			usage();
		} else if (strcmp(argv[i], "-V") == 0
				|| strcmp(argv[i], "--version") == 0) {
			version();
		} else if (!strcmp(argv[i], "-")) {
			// stdin input file
			break;
		} else if (argv[i][0] == '-') {
			error("unknown option %s", argv[i]);
			argv[i] = NULL;
			--args_left;
		}
	}

	if (args_left < 1) {
		error("no input files");
		usage();
	}

	for (int i = 1; i < argc; ++i) {
		if (argv[i] == NULL)
			continue;

		if (sFirstInputFile == NULL)
			sFirstInputFile = argv[i];

		rdef_add_input_file(argv[i]);
	}

	if (sOutputFile[0] == '\0') {
		// no output file name was given, use the name of the
		// first source file as base
		strlcpy(sOutputFile, sFirstInputFile, sizeof(sOutputFile));

		cut_extension(sOutputFile, sDecompile ? ".rsrc" : ".rdef");
		add_extension(sOutputFile, sDecompile ? ".rdef" : ".rsrc");
	}
}


static void
compile()
{
	if (!has_extension(sOutputFile, ".rsrc"))
		add_extension(sOutputFile, ".rsrc");

	rdef_compile(sOutputFile);
}


static void
decompile()
{
	if (!has_extension(sOutputFile, ".rdef"))
		add_extension(sOutputFile, ".rdef");

	rdef_decompile(sOutputFile);
}


static void
report_error()
{
	switch (rdef_err) {
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


int
main(int argc, char *argv[])
{
	parse_options(argc, argv);

	rdef_set_flags(sFlags);

	if (sDecompile)
		decompile();
	else
		compile();

	rdef_free_input_files();
	rdef_free_include_dirs();

	if (rdef_err != B_OK)
		report_error();

	return EXIT_SUCCESS;
}

