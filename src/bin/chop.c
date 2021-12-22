// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//
//  File:        chop.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: splits one file into a collection of smaller files
//
//  Notes:
//  This program was written such that it would have identical output as from
//  the original chop program included with BeOS R5. However, there are a few
//  minor differences:
//
//  a) using "chop -n" (with no other args) crashes the original version,
//     but not this one.
//
//  b) filenames are enclosed in single quotes here, but are not in the original.
//     It is generally better to enquote filenames for error messages so that
//     problems with the name (e.g extra space chars) can be more easily detected.
//
//  c) this version checks for validity of the input file (including file size)
//     before there is any attempt to open it -- this changes the error output
//     slightly from the original in some situations. It can also prevent some
//     weirdness. For example, the original version will take a command such as:
//
//         chop /dev/ports/serial1
//
//     and attempt to open the device. If serial1 is unused, this will actually
//     block while waiting for data from the serial port. This version will never
//     encounter that because the device will be found to have size 0 which will
//     abort immediately. Since the semantics of chop don't make sense for such
//     devices as the source, this is really the better behavior (provided that
//     anyone ever attempts to use such strange arguments, which is unlikely).
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


void chop_file (int, char *, off_t);
void do_chop   (char *);
void usage     (void);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// globals

#define BLOCKSIZE 64 * 1024        // file data is read in BLOCKSIZE blocks
static char Block[BLOCKSIZE];      // and stored in the global Block array

static int KBytesPerChunk = 1400;  // determines size of output files


void
usage()
{
	printf("Usage: chop [-n kbyte_per_chunk] file\n");
	printf("Splits file into smaller files named file00, file01...\n");
	printf("Default split size is 1400k\n");
}



int
main(int argc, char *argv[])
{
	char *arg = NULL;
	char *first;
	
	if ((argc < 2) || (argc > 4)) {
		usage();
		return 0;
	}
	
	first = *++argv;
	
	if (strcmp(first, "--help") == 0) {
		usage();
		return 0;
	}
	
	if (strcmp(first, "-n") == 0) {
		if (--argc > 1) {
			char *num = *++argv;
			
			if (!isdigit(*num))
				printf("-n option needs a numeric argument\n");
			else {
				int b = atoi(num);
				KBytesPerChunk = (b < 1 ? 1 : b);
				
				if (--argc > 1)
					arg = *++argv;
				else
					printf("no file specified\n");
			}
		}
		else
			printf("-n option needs a numeric argument\n");
	}
	else
		arg = first;
	
	if (arg)
		do_chop(arg);
	
	putchar ('\n');
	return 0;
}


void
do_chop(char *fname)
{
	// do some checks for validity
	// then call chop_file() to do the actual read/writes
	
	struct stat e;
	off_t  fsize;
	int    fd;
	
	// input file must exist
	if (stat(fname, &e) == -1) {
		fprintf(stderr, "'%s': no such file or directory\n", fname);
		return;
	}

	// and it must be not be a directory
	if (S_ISDIR(e.st_mode)) {
		fprintf(stderr, "'%s' is a directory\n", fname);
		return;
	}
	
	// needs to be big enough such that splitting it actually does something
	fsize = e.st_size;
	if (fsize < (KBytesPerChunk * 1024)) {
		fprintf(stderr, "'%s': file is already small enough\n", fname);
		return;
	}

	// also, don't chop up if chunk files are already present
	{
		char buf[256];
		
		strcpy(buf, fname);
		strcat(buf, "00");
		
		if (stat(buf, &e) >= 0) {
			fprintf(stderr, "'%s' already exists - aborting\n", buf);
			return;
		}
	}

	// finally! chop up the file
	fd = open(fname, O_RDONLY);
	if (fd < 0)
		fprintf(stderr, "can't open '%s': %s\n", fname, strerror(errno));
	else {
		chop_file(fd, fname, fsize);
		close(fd);
	}
}


void
chop_file(int fdin, char *fname, off_t fsize)
{
	const off_t chunk_size = KBytesPerChunk * 1024;  // max bytes written to any output file

	bool open_next_file = true;  // when to open a new output file
	char fnameN[256];            // name of the current output file (file01, file02, etc.)
	int  index = 0;              // used to generate the next output file name
	int  fdout = -1;             // output file descriptor

	ssize_t got;                 // size of the current data block -- i.e. from the last read()
	ssize_t put;                 // number of bytes just written   -- i.e. from the last write()
	ssize_t needed;              // how many bytes we can safely write to the current output file
	ssize_t avail;               // how many bytes we can safely grab from the current data block
	off_t   curr_written = 0;    // number of bytes written to the current output file
	off_t   total_written = 0;   // total bytes written out to all output files

	char *beg = Block;  // pointer to the beginning of the block data to be written out
	char *end = Block;  // end of the current block (init to beginning to force first block read)

	printf("Chopping up %s into %d kbyte chunks\n", fname, KBytesPerChunk);

	while (total_written < fsize) {
		if (beg >= end) {
			// read in another block
			got = read(fdin, Block, BLOCKSIZE);
			if (got <= 0)
				break;

			beg = Block;
			end = Block + got - 1;
		}

		if (open_next_file) {
			// start a new output file
			sprintf(fnameN,  "%s%02d", fname, index++);

			fdout = open(fnameN, O_WRONLY|O_CREAT);
			if (fdout < 0) {
				fprintf(stderr, "unable to create chunk file '%s': %s\n", fnameN, strerror(errno));
				return;
			}
			curr_written = 0;
			open_next_file = false;
		}

		needed = chunk_size - curr_written;
		avail  = end - beg + 1;
		if (needed > avail)
			needed = avail;

		if (needed > 0) {
			put = write(fdout, beg, needed);
			beg += put;

			curr_written  += put;
			total_written += put;
		}

		if (curr_written >= chunk_size) {
			// the current output file is full
			close(fdout);
			open_next_file = true;
		}
	}

	// close up the last output file if it's still open
	if (!open_next_file)
		close(fdout);
}
