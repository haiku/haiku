/* GNU tar Archive Format description.

   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   2000, 2001, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* tar Header Block, from POSIX 1003.1-1990.  */

/* POSIX header.  */

struct posix_header
{				/* byte offset */
  char name[100];		/*   0 */
  char mode[8];			/* 100 */
  char uid[8];			/* 108 */
  char gid[8];			/* 116 */
  char size[12];		/* 124 */
  char mtime[12];		/* 136 */
  char chksum[8];		/* 148 */
  char typeflag;		/* 156 */
  char linkname[100];		/* 157 */
  char magic[6];		/* 257 */
  char version[2];		/* 263 */
  char uname[32];		/* 265 */
  char gname[32];		/* 297 */
  char devmajor[8];		/* 329 */
  char devminor[8];		/* 337 */
  char prefix[155];		/* 345 */
				/* 500 */
};

#define TMAGIC   "ustar"	/* ustar and a null */
#define TMAGLEN  6
#define TVERSION "00"		/* 00 and no null */
#define TVERSLEN 2

/* Values used in typeflag field.  */
#define REGTYPE	 '0'		/* regular file */
#define AREGTYPE '\0'		/* regular file */
#define LNKTYPE  '1'		/* link */
#define SYMTYPE  '2'		/* reserved */
#define CHRTYPE  '3'		/* character special */
#define BLKTYPE  '4'		/* block special */
#define DIRTYPE  '5'		/* directory */
#define FIFOTYPE '6'		/* FIFO special */
#define CONTTYPE '7'		/* reserved */

#define XHDTYPE  'x'            /* Extended header referring to the
				   next file in the archive */
#define XGLTYPE  'g'            /* Global extended header */

/* Bits used in the mode field, values in octal.  */
#define TSUID    04000		/* set UID on execution */
#define TSGID    02000		/* set GID on execution */
#define TSVTX    01000		/* reserved */
				/* file permissions */
#define TUREAD   00400		/* read by owner */
#define TUWRITE  00200		/* write by owner */
#define TUEXEC   00100		/* execute/search by owner */
#define TGREAD   00040		/* read by group */
#define TGWRITE  00020		/* write by group */
#define TGEXEC   00010		/* execute/search by group */
#define TOREAD   00004		/* read by other */
#define TOWRITE  00002		/* write by other */
#define TOEXEC   00001		/* execute/search by other */

/* tar Header Block, GNU extensions.  */

/* In GNU tar, SYMTYPE is for to symbolic links, and CONTTYPE is for
   contiguous files, so maybe disobeying the `reserved' comment in POSIX
   header description.  I suspect these were meant to be used this way, and
   should not have really been `reserved' in the published standards.  */

/* *BEWARE* *BEWARE* *BEWARE* that the following information is still
   boiling, and may change.  Even if the OLDGNU format description should be
   accurate, the so-called GNU format is not yet fully decided.  It is
   surely meant to use only extensions allowed by POSIX, but the sketch
   below repeats some ugliness from the OLDGNU format, which should rather
   go away.  Sparse files should be saved in such a way that they do *not*
   require two passes at archive creation time.  Huge files get some POSIX
   fields to overflow, alternate solutions have to be sought for this.  */

/* Descriptor for a single file hole.  */

struct sparse
{				/* byte offset */
  char offset[12];		/*   0 */
  char numbytes[12];		/*  12 */
				/*  24 */
};

/* Sparse files are not supported in POSIX ustar format.  For sparse files
   with a POSIX header, a GNU extra header is provided which holds overall
   sparse information and a few sparse descriptors.  When an old GNU header
   replaces both the POSIX header and the GNU extra header, it holds some
   sparse descriptors too.  Whether POSIX or not, if more sparse descriptors
   are still needed, they are put into as many successive sparse headers as
   necessary.  The following constants tell how many sparse descriptors fit
   in each kind of header able to hold them.  */

#define SPARSES_IN_EXTRA_HEADER  16
#define SPARSES_IN_OLDGNU_HEADER 4
#define SPARSES_IN_SPARSE_HEADER 21

/* Extension header for sparse files, used immediately after the GNU extra
   header, and used only if all sparse information cannot fit into that
   extra header.  There might even be many such extension headers, one after
   the other, until all sparse information has been recorded.  */

struct sparse_header
{				/* byte offset */
  struct sparse sp[SPARSES_IN_SPARSE_HEADER];
				/*   0 */
  char isextended;		/* 504 */
				/* 505 */
};

/* The old GNU format header conflicts with POSIX format in such a way that
   POSIX archives may fool old GNU tar's, and POSIX tar's might well be
   fooled by old GNU tar archives.  An old GNU format header uses the space
   used by the prefix field in a POSIX header, and cumulates information
   normally found in a GNU extra header.  With an old GNU tar header, we
   never see any POSIX header nor GNU extra header.  Supplementary sparse
   headers are allowed, however.  */

struct oldgnu_header
{				/* byte offset */
  char unused_pad1[345];	/*   0 */
  char atime[12];		/* 345 Incr. archive: atime of the file */
  char ctime[12];		/* 357 Incr. archive: ctime of the file */
  char offset[12];		/* 369 Multivolume archive: the offset of
				   the start of this volume */
  char longnames[4];		/* 381 Not used */
  char unused_pad2;		/* 385 */
  struct sparse sp[SPARSES_IN_OLDGNU_HEADER];
				/* 386 */
  char isextended;		/* 482 Sparse file: Extension sparse header
				   follows */
  char realsize[12];		/* 483 Sparse file: Real size*/
				/* 495 */
};

/* OLDGNU_MAGIC uses both magic and version fields, which are contiguous.
   Found in an archive, it indicates an old GNU header format, which will be
   hopefully become obsolescent.  With OLDGNU_MAGIC, uname and gname are
   valid, though the header is not truly POSIX conforming.  */
#define OLDGNU_MAGIC "ustar  "	/* 7 chars and a null */

/* The standards committee allows only capital A through capital Z for
   user-defined expansion.  Other letters in use include:

   'A' Solaris Access Control List
   'E' Solaris Extended Attribute File
   'I' Inode only, as in 'star'
   'N' Obsolete GNU tar, for file names that do not fit into the main header.
   'X' POSIX 1003.1-2001 eXtended (VU version)  */

/* This is a dir entry that contains the names of files that were in the
   dir at the time the dump was made.  */
#define GNUTYPE_DUMPDIR	'D'

/* Identifies the *next* file on the tape as having a long linkname.  */
#define GNUTYPE_LONGLINK 'K'

/* Identifies the *next* file on the tape as having a long name.  */
#define GNUTYPE_LONGNAME 'L'

/* This is the continuation of a file that began on another volume.  */
#define GNUTYPE_MULTIVOL 'M'

/* This is for sparse files.  */
#define GNUTYPE_SPARSE 'S'

/* This file is a tape/volume header.  Ignore it on extraction.  */
#define GNUTYPE_VOLHDR 'V'

/* Solaris extended header */
#define SOLARIS_XHDTYPE 'X'

/* J@"org Schilling star header */

struct star_header
{				/* byte offset */
  char name[100];		/*   0 */
  char mode[8];			/* 100 */
  char uid[8];			/* 108 */
  char gid[8];			/* 116 */
  char size[12];		/* 124 */
  char mtime[12];		/* 136 */
  char chksum[8];		/* 148 */
  char typeflag;		/* 156 */
  char linkname[100];		/* 157 */
  char magic[6];		/* 257 */
  char version[2];		/* 263 */
  char uname[32];		/* 265 */
  char gname[32];		/* 297 */
  char devmajor[8];		/* 329 */
  char devminor[8];		/* 337 */
  char prefix[131];		/* 345 */
  char atime[12];               /* 476 */
  char ctime[12];               /* 488 */
                                /* 500 */
};

#define SPARSES_IN_STAR_HEADER      4
#define SPARSES_IN_STAR_EXT_HEADER  21

struct star_in_header
{
  char fill[345];       /*   0  Everything that is before t_prefix */
  char prefix[1];       /* 345  t_name prefix */
  char fill2;           /* 346  */
  char fill3[8];        /* 347  */
  char isextended;      /* 355  */
  struct sparse sp[SPARSES_IN_STAR_HEADER]; /* 356  */
  char realsize[12];    /* 452  Actual size of the file */
  char offset[12];      /* 464  Offset of multivolume contents */
  char atime[12];       /* 476  */
  char ctime[12];       /* 488  */
  char mfill[8];        /* 500  */
  char xmagic[4];       /* 508  "tar" */
};

struct star_ext_header
{
  struct sparse sp[SPARSES_IN_STAR_EXT_HEADER];
  char isextended;
};

/* END */


/* tar Header Block, overall structure.  */

/* tar files are made in basic blocks of this size.  */
#define BLOCKSIZE 512

enum archive_format
{
  DEFAULT_FORMAT,		/* format to be decided later */
  V7_FORMAT,			/* old V7 tar format */
  OLDGNU_FORMAT,		/* GNU format as per before tar 1.12 */
  USTAR_FORMAT,                 /* POSIX.1-1988 (ustar) format */
  POSIX_FORMAT,			/* POSIX.1-2001 format */
  STAR_FORMAT,                  /* Star format defined in 1994 */
  GNU_FORMAT			/* Same as OLDGNU_FORMAT with one exception:
                                   see FIXME note for to_chars() function
                                   (create.c:189) */
};

/* Information about a sparse file.  */
struct sp_array
{
  off_t offset;
  size_t numbytes;
};

struct xheader
{
  struct obstack *stk;
  size_t size;
  char *buffer;
  uintmax_t string_length;
};

struct tar_stat_info
{
  char *orig_file_name;     /* name of file read from the archive header */
  char *file_name;          /* name of file for the current archive entry
			       after being normalized.  */
  bool had_trailing_slash;  /* true if the current archive entry had a
			       trailing slash before it was normalized. */
  char *link_name;          /* name of link for the current archive entry.  */

  char          *uname;     /* user name of owner */
  char          *gname;     /* group name of owner */
  struct stat   stat;       /* regular filesystem stat */

  /* STAT doesn't always have access, data modification, and status
     change times in a convenient form, so store them separately.  */
  struct timespec atime;
  struct timespec mtime;
  struct timespec ctime;

  off_t archive_file_size;  /* Size of file as stored in the archive.
			       Equals stat.st_size for non-sparse files */

  bool   is_sparse;         /* Is the file sparse */

  /* For sparse files: */
  unsigned sparse_major;
  unsigned sparse_minor;
  size_t sparse_map_avail;  /* Index to the first unused element in
			       sparse_map array. Zero if the file is
			       not sparse */
  size_t sparse_map_size;   /* Size of the sparse map */
  struct sp_array *sparse_map;

  /* Extended headers */
  struct xheader xhdr;
  
  /* For dumpdirs */
  bool is_dumpdir;          /* Is the member a dumpdir? */
  bool skipped;             /* The member contents is already read
			       (for GNUTYPE_DUMPDIR) */
  char *dumpdir;            /* Contents of the dump directory */
};

union block
{
  char buffer[BLOCKSIZE];
  struct posix_header header;
  struct star_header star_header;
  struct oldgnu_header oldgnu_header;
  struct sparse_header sparse_header;
  struct star_in_header star_in_header;
  struct star_ext_header star_ext_header;
};
