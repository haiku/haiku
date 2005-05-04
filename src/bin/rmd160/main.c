/* 
   rmd.c
   RIPEMD-160 generate and check utility
   by Po Shan Cheah, Copyright (c) 1997
   You may distribute this program freely provided this notice
   is retained. 

   January 5, 1997:
   Initial release.

*/

#include "rmd160.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFLEN 4096
#define RMDBITS 160

int binary_mode = 0;
int verbose_mode = 0;
char *progname = "";

int prog_rc = 0;

void setrc(int newrc) {
  
  /* raises but never lowers the program return code to newrc */

  if (newrc > prog_rc)
    prog_rc = newrc;
}

int cvthex(char c) {
  /* convert a hex digit to an integer */

  c = toupper(c);

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return c - '0';
}

int hexdigit(char c) {
  /* returns true if character is a hexadecimal digit */

  return 
    (c >= '0' && c <= '9') ||
      (toupper(c) >= 'A' && toupper(c) <= 'F');
}

#define ESCAPE '%'

char *encodestr(char *instr) {
  /* replace unprintable characters and % itself with %HH */

  static char buf[BUFLEN];
  char *outstr = buf;

  for ( ; *instr; ++instr) {
    if (isprint(*instr) && !isspace(*instr) && *instr != ESCAPE)
      *outstr++ = *instr;
    else {
      sprintf(outstr, "%%%02x", *instr);
      outstr += 3;
    }
  } 
  *outstr = '\0';
  return buf;
}

char *decodestr(char *instr) {
  /* undo what encodestr did */

  static char buf[BUFLEN];
  char *outstr = buf;

  while (*instr) {
    if (*instr == ESCAPE &&
	hexdigit(instr[1]) &&
	hexdigit(instr[2])) {
      *outstr++ = cvthex(instr[1]) << 4 | cvthex(instr[2]);
      instr += 3;
    }
    else 
      *outstr++ = *instr ++;
  }
  *outstr = '\0';
  return buf;
}

void pack_chunk(word *chunk, byte *buf) {
  /* pack 64 bytes into 16 little-endian 32-bit words */

  int j;

  for (j = 0; j < 16; ++j, buf += 4)
    *chunk++ = (word) buf[0] | (word) buf[1] << 8 | 
      (word) buf[2] << 16 | (word) buf[3] << 24 ;
}

byte *rmdfp(char *fname, FILE *fp) {
  /* calculate the message digest of a file
     and return it in string form */

  int bytesread;
  int i;
  word tmp;
  word chunk[16];
  char buf[BUFLEN];
  word length[2];
  word mdbuf[RMDBITS / 32];
  static byte code[RMDBITS / 8];

  length[0] = length[1] = 0;
  MDinit(mdbuf);

  /* do all full BUFLEN portions */

  while ((bytesread = fread(buf, 1, BUFLEN, fp)) == BUFLEN) {

    for (i = 0; i < BUFLEN; i += 64) {
      pack_chunk(chunk, buf + i);
      MDcompress(mdbuf, chunk);
    }

    if ((tmp = length[0] + bytesread) < length[0])
      ++ length[1];	/* overflow */
    length[0] = tmp;
  }

  if (ferror(fp)) {
    fprintf(stderr, "%s: %s\n", fname, strerror(errno));
    setrc(2);
    return NULL;
  }

  /* do all the remaining 64-byte blocks */

  for (i = 0; i < bytesread - 63; i += 64) {
    pack_chunk(chunk, buf + i);
    MDcompress(mdbuf, chunk);
  }

  if ((tmp = length[0] + bytesread) < length[0])
    ++ length[1];	/* overflow */
  length[0] = tmp;

  /* do the last partial or zero length block */

  MDfinish(mdbuf, buf + i, length[0] << 3,
	   length[0] >> 29 | length[1] << 3);

  /* convert to 64-byte string using little-endian conversion */

  for (i = 0; i < RMDBITS / 8; i += 4) {

    word wd = mdbuf[i >> 2];

    code[i] = (byte) (wd);
    code[i + 1] = (byte) (wd >> 8);
    code[i + 2] = (byte) (wd >> 16);
    code[i + 3] = (byte) (wd >> 24);
  }

  return code;
}

void printcode(byte *code) {
  /* print a RIPEMD-160 code */
  
  int i;

  for (i = 0; i < RMDBITS / 8; ++i)
    printf("%02x", code[i]);
}

byte *rmdfile(char *fname) {
  /* calculate the message digest of a single file
     and output the digest followed by the file name */

  FILE *fp;

  fp = fopen(fname, binary_mode ? "rb" : "r");
  if (fp == NULL) {
    fprintf(stderr, "%s: %s\n", fname, strerror(errno));
    setrc(2);
    return NULL;
  }

  else {
    byte *code = rmdfp(fname, fp);
    fclose(fp);
    return code;
  }
}

int checkdigest(char *digest) {
  /* returns true if string is a valid message digest string */

  if (strlen(digest) != RMDBITS / 8 * 2)
    return 0;

  for ( ; *digest; ++digest) {
    if ( ! hexdigit(*digest))
      return 0;
  }

  return 1;
}

void stringtocode(byte *code, char *str) {
  /* convert message digest string into 20 byte code */

  int i;

  for (i = 0; i < RMDBITS / 8; ++i, str += 2)
    *code++ = cvthex(str[0]) << 4 | cvthex(str[1]);
}

void checkfp(char *fname, FILE *fp) {
  /* check message digests. message digest data comes from 
     the given open file handle. */

  int lineno = 0;
  int nfail = 0;
  int nfile = 0;
  char *filename;
  byte *code;
  byte inputcode[RMDBITS / 8];
  char line[BUFLEN];
  char digest[BUFLEN];
  char infilename[BUFLEN];

  while (fgets(line, BUFLEN, fp)) {

    ++lineno;

    /* error checking */

    if (sscanf(line, "%s%s", digest, infilename) < 2) {
      fprintf(stderr, "%s: invalid input on line %d\n",
	      progname, lineno);
      setrc(2);
      continue;
    }

    if (!checkdigest(digest)) {
      fprintf(stderr, "%s: invalid message digest on line %d\n", 
	      progname, lineno);
      setrc(2);
      continue;
    }

    stringtocode(inputcode, digest);

    /* calculate message digest for file */

    filename = decodestr(infilename);

    code = rmdfile(filename);
    if (code) {

      ++nfile;

      /* compare digests */

      if (memcmp(inputcode, code, RMDBITS / 8) != 0) {
	++nfail;
	setrc(1);

	if (verbose_mode)
	  printf("FAILED   %s\n", filename);
	else
	  printf("%s: RIPEMD-160 check failed for `%s'\n",
		 progname, filename);
      }

      else {
	if (verbose_mode)
	  printf("GOOD     %s\n", filename);
      }
    }
  }

  if (verbose_mode && nfail)
    printf("%s: %d of %d file(s) failed RIPEMD-160 check\n", 
	   progname, nfail, nfile);

  if (ferror(fp)) {
    fprintf(stderr, "%s: %s\n", fname, strerror(errno));
    setrc(2);
  }
} 

void checkfile(char *fname) {
  /* check message digests. message digest data comes from 
     the named file. */

  FILE *fp;

  fp = fopen(fname, "r");
  if (fp == NULL) {
    fprintf(stderr, "%s: %s\n", fname, strerror(errno));
    setrc(2);
  }

  else {
    checkfp(fname, fp);
    fclose(fp);
  }
}

int main(int argc, char *argv[]) {

  int c;
  int check_mode = 0;

  progname = argv[0];

  /* parse command line arguments */

  while ((c = getopt(argc, argv, "bcvh")) >= 0) {

    switch (c) {

    case 'b':
      binary_mode = 1;
      break;

    case 'c':
      check_mode = 1;
      break;

    case 'v':
      verbose_mode = 1;
      break;

    case 'h':
    case ':':
    case '?':
      printf("Usage: %s [-b] [-c] [-v] [<file>...]\n"
	     "Generates or checks RIPEMD-160 message digests\n"
	     "    -b  read files in binary mode\n"
	     "    -c  check message digests\n"
	     "    -v  verbose, print file names while checking\n\n"
	     "If -c is not specified, then one message digest is "
	     "generated per file\n"
	     "and sent to standard output. If no files are specified "
	     "then input is\n"
	     "taken from standard input and only one message digest "
	     "will be\n"
	     "generated.\n\n"
	     "If -c is specified then message digests for a list of "
	     "files are\n"
	     "checked. The input should be a table in the same format "
	     "as the output\n"
	     "of this program when message digests are generated. If a "
	     "file is\n"
	     "specified then the message digest table is read from that "
	     "file. If no\n"
	     "file is specified, then the message digest table is read "
	     "from standard\n"
	     "input.\n", progname);
      exit(0);
    }
  }

  if (check_mode) {

    if (optind < argc) {

      /* check using data from file named on command line */

      checkfile(argv[optind]);

    }

    else {

      /* check using data from standard input */

      checkfp("(stdin)", stdin);

    }
  }

  else {

    if (optind < argc) {

      /* process file arguments */

      for ( ; optind < argc; ++optind) {

	byte *code = rmdfile(argv[optind]);

	if (code) {
	  printcode(code);      
	  printf("  %s\n", encodestr(argv[optind]));
	}
      }
    }

    else {

      /* no file arguments so take input from stdin */

      byte *code = rmdfp("(stdin)", stdin);

      if (code) {
	printcode(code); 
	printf("\n");
      }
    }
  }

  return prog_rc;
}
