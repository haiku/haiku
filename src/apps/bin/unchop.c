// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        unchop.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: recreates a file previously split with chop
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


void  usage       (void);
void  do_unchop   (char *, char *);
void  concatenate (int, int);
bool  valid_file  (char *);
void  replace     (char *, char *);
char *temp_file   ();



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// globals

#define BLOCKSIZE 64 * 1024     // file data is read in BLOCKSIZE blocks
static char Block[BLOCKSIZE];   // and stored in the global Block array

static int Errors = 0;



void
usage ()
	{
	puts ("usage: unchop file");
	puts ("Concatenates files named file00, file01... into file");
	}


int
main (int argc, char *argv[])
	{	
	if (argc != 2)
		{
		usage ();
		return 0;
		}
	else
		{
		char *origfile = argv[1];
		char *tmpfile  = origfile;
		bool  needs_replace = false;
	
		if (valid_file (origfile))
			{
			// output file already exists -- write to temp file
			tmpfile = temp_file ();
			needs_replace = true;
			}
		
		do_unchop (tmpfile, origfile);
		
		if (needs_replace)
			{
			if (Errors == 0)
				replace (origfile, tmpfile);
			else
				remove (tmpfile);
			}
		}
	
	putchar ('\n');
	return Errors;
	}


void
do_unchop (char *outfile, char *basename)
	{
	int fdout = open (outfile, O_WRONLY|O_CREAT|O_APPEND);
	if (fdout < 0)
		fprintf (stderr, "can't open '%s': %s\n", outfile, strerror (errno));
	else
		{
		int  i;
		char fnameN[256];
		
		for (i = 0; i < 999999; ++i)
			{
			sprintf (fnameN,  "%s%02d", basename, i);
			
			if (valid_file (fnameN))
				{
				int fdin = open (fnameN, O_RDONLY);
				if (fdin < 0)
					{
					fprintf (stderr, "can't open '%s': %s\n", fnameN, strerror (errno));
					++Errors;
					}
				else
					{
					concatenate (fdin, fdout);
					close (fdin);
					}
				}
			else
				{
				if (i == 0)
					printf ("No chunk files present (%s)", fnameN);
				break;
				}
			}
		close (fdout);
		}
	}


void
concatenate (int fdin, int fdout)
	{
	ssize_t got;
	
	for (;;)
		{
		got = read (fdin, Block, BLOCKSIZE);
		if (got <= 0)
			break;
		
		write (fdout, Block, got);
		}
	}


bool
valid_file (char *fname)
	{
	// for this program, a valid file is one that:
	//   a) exists (that always helps)
	//   b) is a regular file (not a directory, link, etc.)
	
	struct stat e;
		
	if (stat (fname, &e) == -1)
		{
		// no such file
		return false;
		}

	return (S_ISREG (e.st_mode));
	}


void
replace (char *origfile, char *newfile)
	{
	// replace the contents of the original file
	// with the contents of the new file
	
	char buf[1000];
	
	// delete the original file
	remove (origfile);
	
	// rename the new file to the original file name
	sprintf (buf, "mv \"%s\" \"%s\"", newfile, origfile);
	system (buf);
	}


char *
temp_file ()
	{
	char *tmp = tmpnam (NULL);
	
	FILE *fp = fopen (tmp, "w");
	fclose (fp);
	
	return tmp;
	}
