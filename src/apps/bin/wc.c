// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        wc.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: standard Unix wc (word count) command
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>


int  do_wc   (char *);
void wc_file (FILE *, char *);
void display (int, int, int, char *);


// a few globals to brighten your day

int TotalLines = 0;
int TotalWords = 0;
int TotalBytes = 0;

int ShowLines = 1;
int ShowWords = 1;
int ShowBytes = 1;




int
main (int argc, char *argv[])
	{
	if (argc == 1)
		{
		wc_file (stdin, "");
		}
	else
		{
		int  i;
		int  file_count = 0;
		int  reset_opts = 0;
		char *arg;
		char c;
		
		// pass 1: collect and set the options
		for (i = 1; i < argc; ++i)
			{
			arg = argv[i];
			if (arg[0] == '-')
				{
				if (!reset_opts)
					{
					ShowLines = ShowWords = ShowBytes = 0;
					reset_opts = 1;
					}
				while (c = *++arg)
					{
					if      (c == 'l') ShowLines = 1;
					else if (c == 'w') ShowWords = 1;
					else if (c == 'c') ShowBytes = 1;
					}
				}
			}
		
		// pass 2: process filename args
		for (i = 1; i < argc; ++i)
			{
			arg = argv[i];
			if (arg[0] != '-')
				file_count += do_wc (arg);
			}
		
		if (file_count > 1)
			display (TotalLines, TotalWords, TotalBytes, "total");
		}
	
	putchar ('\n');
	return 0;
	}


int
do_wc (char *fname)
	{
	struct stat e;
	
	if (stat (fname, &e) == -1)
		{
		fprintf (stderr, "'%s': no such file or directory\n", fname);
		return 0;
		}

	if (S_ISDIR (e.st_mode))
		{
		fprintf (stderr, "'%s' is a directory\n", fname);
		display (0, 0, 0, fname);
		return 1;
		}
	else
		{
		FILE *fp = fopen (fname, "rb");
		if (fp)
			{
			wc_file (fp, fname);
			fclose (fp);
			return 1;
			}
		else
			{
			fprintf (stderr, "'%s': %s\n", fname, strerror (errno));
			return 0;
			}
		}
	}


void
wc_file (FILE *fp, char *fname)
	{
	int lc = 0;  // line count
	int wc = 0;  // word count
	int bc = 0;  // byte count
	
	int ns = 0;  // non-spaces (consecutive count)
	int c;
	
	while ((c = fgetc (fp)) != EOF)
		{
		++bc;
		if (c == '\n')
			++lc;
		
		if (isspace (c))
			{
			if (ns > 0)
				++wc, ns = 0;
			}
		else
			++ns;
		}
	
	if (ns > 0)
		++wc;
	
	TotalLines += lc;
	TotalWords += wc;
	TotalBytes += bc;
	
	display (lc, wc, bc, fname);
	}


void
display (int lc, int wc, int bc, char *fname)
	{
	if (ShowLines) printf ("%7d", lc);
	if (ShowWords) printf ("%8d", wc);
	if (ShowBytes) printf ("%8d", bc);
	if (*fname)    printf (" %s", fname);
	
	putchar ('\n');
	}

