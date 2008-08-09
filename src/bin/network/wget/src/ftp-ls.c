/* Parsing FTP `ls' output.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc. 

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <time.h>

#include "wget.h"
#include "utils.h"
#include "ftp.h"
#include "url.h"
#include "convert.h"            /* for html_quote_string prototype */
#include "retr.h"               /* for output_stream */

/* Converts symbolic permissions to number-style ones, e.g. string
   rwxr-xr-x to 755.  For now, it knows nothing of
   setuid/setgid/sticky.  ACLs are ignored.  */
static int
symperms (const char *s)
{
  int perms = 0, i;

  if (strlen (s) < 9)
    return 0;
  for (i = 0; i < 3; i++, s += 3)
    {
      perms <<= 3;
      perms += (((s[0] == 'r') << 2) + ((s[1] == 'w') << 1) +
                (s[2] == 'x' || s[2] == 's'));
    }
  return perms;
}


/* Cleans a line of text so that it can be consistently parsed. Destroys
   <CR> and <LF> in case that thay occur at the end of the line and
   replaces all <TAB> character with <SPACE>. Returns the length of the
   modified line. */
static int
clean_line(char *line)
{
  int len = strlen (line);
  if (!len) return 0; 
  if (line[len - 1] == '\n')
    line[--len] = '\0';
  if (line[len - 1] == '\r')
    line[--len] = '\0';
  for ( ; *line ; line++ ) if (*line == '\t') *line = ' '; 
  return len;
}

/* Convert the Un*x-ish style directory listing stored in FILE to a
   linked list of fileinfo (system-independent) entries.  The contents
   of FILE are considered to be produced by the standard Unix `ls -la'
   output (whatever that might be).  BSD (no group) and SYSV (with
   group) listings are handled.

   The time stamps are stored in a separate variable, time_t
   compatible (I hope).  The timezones are ignored.  */
static struct fileinfo *
ftp_parse_unix_ls (const char *file, int ignore_perms)
{
  FILE *fp;
  static const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  int next, len, i, error, ignore;
  int year, month, day;         /* for time analysis */
  int hour, min, sec;
  struct tm timestruct, *tnow;
  time_t timenow;

  char *line, *tok, *ptok;      /* tokenizer */
  struct fileinfo *dir, *l, cur; /* list creation */

  fp = fopen (file, "rb");
  if (!fp)
    {
      logprintf (LOG_NOTQUIET, "%s: %s\n", file, strerror (errno));
      return NULL;
    }
  dir = l = NULL;

  /* Line loop to end of file: */
  while ((line = read_whole_line (fp)) != NULL)
    {
      len = clean_line (line);
      /* Skip if total...  */
      if (!strncasecmp (line, "total", 5))
        {
          xfree (line);
          continue;
        }
      /* Get the first token (permissions).  */
      tok = strtok (line, " ");
      if (!tok)
        {
          xfree (line);
          continue;
        }

      cur.name = NULL;
      cur.linkto = NULL;

      /* Decide whether we deal with a file or a directory.  */
      switch (*tok)
        {
        case '-':
          cur.type = FT_PLAINFILE;
          DEBUGP (("PLAINFILE; "));
          break;
        case 'd':
          cur.type = FT_DIRECTORY;
          DEBUGP (("DIRECTORY; "));
          break;
        case 'l':
          cur.type = FT_SYMLINK;
          DEBUGP (("SYMLINK; "));
          break;
        default:
          cur.type = FT_UNKNOWN;
          DEBUGP (("UNKNOWN; "));
          break;
        }

      if (ignore_perms)
        {
          switch (cur.type)
            {
            case FT_PLAINFILE:
              cur.perms = 0644;
              break;
            case FT_DIRECTORY:
              cur.perms = 0755;
              break;
            default:
              /*cur.perms = 1023;*/     /* #### What is this?  --hniksic */
              cur.perms = 0644;
            }
          DEBUGP (("implicit perms %0o; ", cur.perms));
        }
       else
         {
           cur.perms = symperms (tok + 1);
           DEBUGP (("perms %0o; ", cur.perms));
         }

      error = ignore = 0;       /* Erroneous and ignoring entries are
                                   treated equally for now.  */
      year = hour = min = sec = 0; /* Silence the compiler.  */
      month = day = 0;
      next = -1;
      /* While there are tokens on the line, parse them.  Next is the
         number of tokens left until the filename.

         Use the month-name token as the "anchor" (the place where the
         position wrt the file name is "known").  When a month name is
         encountered, `next' is set to 5.  Also, the preceding
         characters are parsed to get the file size.

         This tactic is quite dubious when it comes to
         internationalization issues (non-English month names), but it
         works for now.  */
      tok = line;
      while (ptok = tok,
             (tok = strtok (NULL, " ")) != NULL)
        {
          --next;
          if (next < 0)         /* a month name was not encountered */
            {
              for (i = 0; i < 12; i++)
                if (!strcmp (tok, months[i]))
                  break;
              /* If we got a month, it means the token before it is the
                 size, and the filename is three tokens away.  */
              if (i != 12)
                {
                  wgint size;

                  /* Parse the previous token with str_to_wgint.  */
                  if (ptok == line)
                    {
                      /* Something has gone wrong during parsing. */
                      error = 1;
                      break;
                    }
                  errno = 0;
                  size = str_to_wgint (ptok, NULL, 10);
                  if (size == WGINT_MAX && errno == ERANGE)
                    /* Out of range -- ignore the size.  #### Should
                       we refuse to start the download.  */
                    cur.size = 0;
                  else
                    cur.size = size;
                  DEBUGP (("size: %s; ", number_to_static_string(cur.size)));

                  month = i;
                  next = 5;
                  DEBUGP (("month: %s; ", months[month]));
                }
            }
          else if (next == 4)   /* days */
            {
              if (tok[1])       /* two-digit... */
                day = 10 * (*tok - '0') + tok[1] - '0';
              else              /* ...or one-digit */
                day = *tok - '0';
              DEBUGP (("day: %d; ", day));
            }
          else if (next == 3)
            {
              /* This ought to be either the time, or the year.  Let's
                 be flexible!

                 If we have a number x, it's a year.  If we have x:y,
                 it's hours and minutes.  If we have x:y:z, z are
                 seconds.  */
              year = 0;
              min = hour = sec = 0;
              /* We must deal with digits.  */
              if (ISDIGIT (*tok))
                {
                  /* Suppose it's year.  */
                  for (; ISDIGIT (*tok); tok++)
                    year = (*tok - '0') + 10 * year;
                  if (*tok == ':')
                    {
                      /* This means these were hours!  */
                      hour = year;
                      year = 0;
                      ++tok;
                      /* Get the minutes...  */
                      for (; ISDIGIT (*tok); tok++)
                        min = (*tok - '0') + 10 * min;
                      if (*tok == ':')
                        {
                          /* ...and the seconds.  */
                          ++tok;
                          for (; ISDIGIT (*tok); tok++)
                            sec = (*tok - '0') + 10 * sec;
                        }
                    }
                }
              if (year)
                DEBUGP (("year: %d (no tm); ", year));
              else
                DEBUGP (("time: %02d:%02d:%02d (no yr); ", hour, min, sec));
            }
          else if (next == 2)    /* The file name */
            {
              int fnlen;
              char *p;

              /* Since the file name may contain a SPC, it is possible
                 for strtok to handle it wrong.  */
              fnlen = strlen (tok);
              if (fnlen < len - (tok - line))
                {
                  /* So we have a SPC in the file name.  Restore the
                     original.  */
                  tok[fnlen] = ' ';
                  /* If the file is a symbolic link, it should have a
                     ` -> ' somewhere.  */
                  if (cur.type == FT_SYMLINK)
                    {
                      p = strstr (tok, " -> ");
                      if (!p)
                        {
                          error = 1;
                          break;
                        }
                      cur.linkto = xstrdup (p + 4);
                      DEBUGP (("link to: %s\n", cur.linkto));
                      /* And separate it from the file name.  */
                      *p = '\0';
                    }
                }
              /* If we have the filename, add it to the list of files or
                 directories.  */
              /* "." and ".." are an exception!  */
              if (!strcmp (tok, ".") || !strcmp (tok, ".."))
                {
                  DEBUGP (("\nIgnoring `.' and `..'; "));
                  ignore = 1;
                  break;
                }
              /* Some FTP sites choose to have ls -F as their default
                 LIST output, which marks the symlinks with a trailing
                 `@', directory names with a trailing `/' and
                 executables with a trailing `*'.  This is no problem
                 unless encountering a symbolic link ending with `@',
                 or an executable ending with `*' on a server without
                 default -F output.  I believe these cases are very
                 rare.  */
              fnlen = strlen (tok); /* re-calculate `fnlen' */
              cur.name = xmalloc (fnlen + 1);
              memcpy (cur.name, tok, fnlen + 1);
              if (fnlen)
                {
                  if (cur.type == FT_DIRECTORY && cur.name[fnlen - 1] == '/')
                    {
                      cur.name[fnlen - 1] = '\0';
                      DEBUGP (("trailing `/' on dir.\n"));
                    }
                  else if (cur.type == FT_SYMLINK && cur.name[fnlen - 1] == '@')
                    {
                      cur.name[fnlen - 1] = '\0';
                      DEBUGP (("trailing `@' on link.\n"));
                    }
                  else if (cur.type == FT_PLAINFILE
                           && (cur.perms & 0111)
                           && cur.name[fnlen - 1] == '*')
                    {
                      cur.name[fnlen - 1] = '\0';
                      DEBUGP (("trailing `*' on exec.\n"));
                    }
                } /* if (fnlen) */
              else
                error = 1;
              break;
            }
          else
            abort ();
        } /* while */

      if (!cur.name || (cur.type == FT_SYMLINK && !cur.linkto))
        error = 1;

      DEBUGP (("%s\n", cur.name ? cur.name : ""));

      if (error || ignore)
        {
          DEBUGP (("Skipping.\n"));
          xfree_null (cur.name);
          xfree_null (cur.linkto);
          xfree (line);
          continue;
        }

      if (!dir)
        {
          l = dir = xnew (struct fileinfo);
          memcpy (l, &cur, sizeof (cur));
          l->prev = l->next = NULL;
        }
      else
        {
          cur.prev = l;
          l->next = xnew (struct fileinfo);
          l = l->next;
          memcpy (l, &cur, sizeof (cur));
          l->next = NULL;
        }
      /* Get the current time.  */
      timenow = time (NULL);
      tnow = localtime (&timenow);
      /* Build the time-stamp (the idea by zaga@fly.cc.fer.hr).  */
      timestruct.tm_sec   = sec;
      timestruct.tm_min   = min;
      timestruct.tm_hour  = hour;
      timestruct.tm_mday  = day;
      timestruct.tm_mon   = month;
      if (year == 0)
        {
          /* Some listings will not specify the year if it is "obvious"
             that the file was from the previous year.  E.g. if today
             is 97-01-12, and you see a file of Dec 15th, its year is
             1996, not 1997.  Thanks to Vladimir Volovich for
             mentioning this!  */
          if (month > tnow->tm_mon)
            timestruct.tm_year = tnow->tm_year - 1;
          else
            timestruct.tm_year = tnow->tm_year;
        }
      else
        timestruct.tm_year = year;
      if (timestruct.tm_year >= 1900)
        timestruct.tm_year -= 1900;
      timestruct.tm_wday  = 0;
      timestruct.tm_yday  = 0;
      timestruct.tm_isdst = -1;
      l->tstamp = mktime (&timestruct); /* store the time-stamp */

      xfree (line);
    }

  fclose (fp);
  return dir;
}

static struct fileinfo *
ftp_parse_winnt_ls (const char *file)
{
  FILE *fp;
  int len;
  int year, month, day;         /* for time analysis */
  int hour, min;
  struct tm timestruct;

  char *line, *tok;             /* tokenizer */
  struct fileinfo *dir, *l, cur; /* list creation */

  fp = fopen (file, "rb");
  if (!fp)
    {
      logprintf (LOG_NOTQUIET, "%s: %s\n", file, strerror (errno));
      return NULL;
    }
  dir = l = NULL;

  /* Line loop to end of file: */
  while ((line = read_whole_line (fp)) != NULL)
    {
      len = clean_line (line);

      /* Extracting name is a bit of black magic and we have to do it
         before `strtok' inserted extra \0 characters in the line
         string. For the moment let us just suppose that the name starts at
         column 39 of the listing. This way we could also recognize
         filenames that begin with a series of space characters (but who
         really wants to use such filenames anyway?). */
      if (len < 40) continue;
      tok = line + 39;
      cur.name = xstrdup(tok);
      DEBUGP(("Name: '%s'\n", cur.name));

      /* First column: mm-dd-yy. Should atoi() on the month fail, january
         will be assumed.  */
      tok = strtok(line, "-");
      if (tok == NULL) continue;
      month = atoi(tok) - 1;
      if (month < 0) month = 0;
      tok = strtok(NULL, "-");
      if (tok == NULL) continue;
      day = atoi(tok);
      tok = strtok(NULL, " ");
      if (tok == NULL) continue;
      year = atoi(tok);
      /* Assuming the epoch starting at 1.1.1970 */
      if (year <= 70) year += 100;

      /* Second column: hh:mm[AP]M, listing does not contain value for
         seconds */
      tok = strtok(NULL,  ":");
      if (tok == NULL) continue;
      hour = atoi(tok);
      tok = strtok(NULL,  "M");
      if (tok == NULL) continue;
      min = atoi(tok);
      /* Adjust hour from AM/PM. Just for the record, the sequence goes
         11:00AM, 12:00PM, 01:00PM ... 11:00PM, 12:00AM, 01:00AM . */
      tok+=2;
      if (hour == 12)  hour  = 0;
      if (*tok == 'P') hour += 12;

      DEBUGP(("YYYY/MM/DD HH:MM - %d/%02d/%02d %02d:%02d\n", 
              year+1900, month, day, hour, min));
      
      /* Build the time-stamp (copy & paste from above) */
      timestruct.tm_sec   = 0;
      timestruct.tm_min   = min;
      timestruct.tm_hour  = hour;
      timestruct.tm_mday  = day;
      timestruct.tm_mon   = month;
      timestruct.tm_year  = year;
      timestruct.tm_wday  = 0;
      timestruct.tm_yday  = 0;
      timestruct.tm_isdst = -1;
      cur.tstamp = mktime (&timestruct); /* store the time-stamp */

      DEBUGP(("Timestamp: %ld\n", cur.tstamp));

      /* Third column: Either file length, or <DIR>. We also set the
         permissions (guessed as 0644 for plain files and 0755 for
         directories as the listing does not give us a clue) and filetype
         here. */
      tok = strtok(NULL, " ");
      if (tok == NULL) continue;
      while ((tok != NULL) && (*tok == '\0'))  tok = strtok(NULL, " ");
      if (tok == NULL) continue;
      if (*tok == '<')
        {
          cur.type  = FT_DIRECTORY;
          cur.size  = 0;
          cur.perms = 0755;
          DEBUGP(("Directory\n"));
        }
      else
        {
          wgint size;
          cur.type  = FT_PLAINFILE;
          errno = 0;
          size = str_to_wgint (tok, NULL, 10);
          if (size == WGINT_MAX && errno == ERANGE)
            cur.size = 0;       /* overflow */
          else
            cur.size = size;
          cur.perms = 0644;
          DEBUGP(("File, size %s bytes\n", number_to_static_string (cur.size)));
        }

      cur.linkto = NULL;

      /* And put everything into the linked list */
      if (!dir)
        {
          l = dir = xnew (struct fileinfo);
          memcpy (l, &cur, sizeof (cur));
          l->prev = l->next = NULL;
        }
      else
        {
          cur.prev = l;
          l->next = xnew (struct fileinfo);
          l = l->next;
          memcpy (l, &cur, sizeof (cur));
          l->next = NULL;
        }

      xfree (line);
    }

  fclose(fp);
  return dir;
}

/* Converts VMS symbolic permissions to number-style ones, e.g. string
   RWED,RWE,RE to 755. "D" (delete) is taken to be equal to "W"
   (write). Inspired by a patch of Stoyan Lekov <lekov@eda.bg>. */
static int
vmsperms (const char *s)
{
  int perms = 0;

  do
    {
      switch (*s) {
        case ',': perms <<= 3; break;
        case 'R': perms  |= 4; break;
        case 'W': perms  |= 2; break;
        case 'D': perms  |= 2; break;
        case 'E': perms  |= 1; break;
        default:  DEBUGP(("wrong VMS permissons!\n")); 
      }
    }
  while (*++s);
  return perms;
}


static struct fileinfo *
ftp_parse_vms_ls (const char *file)
{
  FILE *fp;
  /* #### A third copy of more-or-less the same array ? */
  static const char *months[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
  };
  int i;
  int year, month, day;          /* for time analysis */
  int hour, min, sec;
  struct tm timestruct;

  char *line, *tok;              /* tokenizer */
  struct fileinfo *dir, *l, cur; /* list creation */

  fp = fopen (file, "rb");
  if (!fp)
    {
      logprintf (LOG_NOTQUIET, "%s: %s\n", file, strerror (errno));
      return NULL;
    }
  dir = l = NULL;

  /* Skip empty line. */
  line = read_whole_line (fp);
  xfree_null (line);

  /* Skip "Directory PUB$DEVICE[PUB]" */
  line = read_whole_line (fp);
  xfree_null (line);

  /* Skip empty line. */
  line = read_whole_line (fp);
  xfree_null (line);

  /* Line loop to end of file: */
  while ((line = read_whole_line (fp)) != NULL)
    {
      char *p;
      i = clean_line (line);
      if (!i)
        {
          xfree (line);
          break;
        }

      /* First column: Name. A bit of black magic again. The name my be
         either ABCD.EXT or ABCD.EXT;NUM and it might be on a separate
         line. Therefore we will first try to get the complete name
         until the first space character; if it fails, we assume that the name
         occupies the whole line. After that we search for the version
         separator ";", we remove it and check the extension of the file;
         extension .DIR denotes directory. */

      tok = strtok(line, " ");
      if (tok == NULL) tok = line;
      DEBUGP(("file name: '%s'\n", tok));
      for (p = tok ; *p && *p != ';' ; p++)
        ;
      if (*p == ';') *p = '\0';
      p   = tok + strlen(tok) - 4;
      if (!strcmp(p, ".DIR")) *p = '\0';
      cur.name = xstrdup(tok);
      DEBUGP(("Name: '%s'\n", cur.name));

      /* If the name ends on .DIR or .DIR;#, it's a directory. We also set
         the file size to zero as the listing does tell us only the size in
         filesystem blocks - for an integrity check (when mirroring, for
         example) we would need the size in bytes. */
      
      if (! *p)
        {
          cur.type  = FT_DIRECTORY;
          cur.size  = 0;
          DEBUGP(("Directory\n"));
        }
      else
        {
          cur.type  = FT_PLAINFILE;
          DEBUGP(("File\n"));
        }

      cur.size  = 0;

      /* Second column, if exists, or the first column of the next line
         contain file size in blocks. We will skip it. */

      tok = strtok(NULL, " ");
      if (tok == NULL) 
      {
        DEBUGP(("Getting additional line\n"));
        xfree (line);
        line = read_whole_line (fp);
        if (!line)
        {
          DEBUGP(("empty line read, leaving listing parser\n"));
          break;
        }
        i = clean_line (line);
        if (!i) 
        {
          DEBUGP(("confusing VMS listing item, leaving listing parser\n"));
          xfree (line);
          break;
        }
        tok = strtok(line, " ");
      }
      DEBUGP(("second token: '%s'\n", tok));

      /* Third/Second column: Date DD-MMM-YYYY. */

      tok = strtok(NULL, "-");
      if (tok == NULL) continue;
      DEBUGP(("day: '%s'\n",tok));
      day = atoi(tok);
      tok = strtok(NULL, "-");
      if (!tok)
      {
        /* If the server produces garbage like
           'EA95_0PS.GZ;1      No privilege for attempted operation'
           the first strtok(NULL, "-") will return everything until the end
           of the line and only the next strtok() call will return NULL. */
        DEBUGP(("nonsense in VMS listing, skipping this line\n"));
        xfree (line);
        break;
      }
      for (i=0; i<12; i++) if (!strcmp(tok,months[i])) break;
      /* Uknown months are mapped to January */
      month = i % 12 ; 
      tok = strtok (NULL, " ");
      if (tok == NULL) continue;
      year = atoi (tok) - 1900;
      DEBUGP(("date parsed\n"));

      /* Fourth/Third column: Time hh:mm[:ss] */
      tok = strtok (NULL, " ");
      if (tok == NULL) continue;
      min = sec = 0;
      p = tok;
      hour = atoi (p);
      for (; *p && *p != ':'; ++p)
        ;
      if (*p)
        min = atoi (++p);
      for (; *p && *p != ':'; ++p)
        ;
      if (*p)
        sec = atoi (++p);

      DEBUGP(("YYYY/MM/DD HH:MM:SS - %d/%02d/%02d %02d:%02d:%02d\n", 
              year+1900, month, day, hour, min, sec));
      
      /* Build the time-stamp (copy & paste from above) */
      timestruct.tm_sec   = sec;
      timestruct.tm_min   = min;
      timestruct.tm_hour  = hour;
      timestruct.tm_mday  = day;
      timestruct.tm_mon   = month;
      timestruct.tm_year  = year;
      timestruct.tm_wday  = 0;
      timestruct.tm_yday  = 0;
      timestruct.tm_isdst = -1;
      cur.tstamp = mktime (&timestruct); /* store the time-stamp */

      DEBUGP(("Timestamp: %ld\n", cur.tstamp));

      /* Skip the fifth column */

      tok = strtok(NULL, " ");
      if (tok == NULL) continue;

      /* Sixth column: Permissions */

      tok = strtok(NULL, ","); /* Skip the VMS-specific SYSTEM permissons */
      if (tok == NULL) continue;
      tok = strtok(NULL, ")");
      if (tok == NULL)
        {
          DEBUGP(("confusing VMS permissions, skipping line\n"));
          xfree (line);
          continue;
        }
      /* Permissons have the format "RWED,RWED,RE" */
      cur.perms = vmsperms(tok);
      DEBUGP(("permissions: %s -> 0%o\n", tok, cur.perms));

      cur.linkto = NULL;

      /* And put everything into the linked list */
      if (!dir)
        {
          l = dir = xnew (struct fileinfo);
          memcpy (l, &cur, sizeof (cur));
          l->prev = l->next = NULL;
        }
      else
        {
          cur.prev = l;
          l->next = xnew (struct fileinfo);
          l = l->next;
          memcpy (l, &cur, sizeof (cur));
          l->next = NULL;
        }

      xfree (line);
    }

  fclose (fp);
  return dir;
}


/* This function switches between the correct parsing routine depending on
   the SYSTEM_TYPE. The system type should be based on the result of the
   "SYST" response of the FTP server. According to this repsonse we will
   use on of the three different listing parsers that cover the most of FTP
   servers used nowadays.  */

struct fileinfo *
ftp_parse_ls (const char *file, const enum stype system_type)
{
  switch (system_type)
    {
    case ST_UNIX:
      return ftp_parse_unix_ls (file, 0);
    case ST_WINNT:
      {
        /* Detect whether the listing is simulating the UNIX format */
        FILE *fp;
        int   c;
        fp = fopen (file, "rb");
        if (!fp)
        {
          logprintf (LOG_NOTQUIET, "%s: %s\n", file, strerror (errno));
          return NULL;
        }
        c = fgetc(fp);
        fclose(fp);
        /* If the first character of the file is '0'-'9', it's WINNT
           format. */
        if (c >= '0' && c <='9')
          return ftp_parse_winnt_ls (file);
        else
          return ftp_parse_unix_ls (file, 1);
      }
    case ST_VMS:
      return ftp_parse_vms_ls (file);
    case ST_MACOS:
      return ftp_parse_unix_ls (file, 1);
    default:
      logprintf (LOG_NOTQUIET, _("\
Unsupported listing type, trying Unix listing parser.\n"));
      return ftp_parse_unix_ls (file, 0);
    }
}

/* Stuff for creating FTP index. */

/* The function creates an HTML index containing references to given
   directories and files on the appropriate host.  The references are
   FTP.  */
uerr_t
ftp_index (const char *file, struct url *u, struct fileinfo *f)
{
  FILE *fp;
  char *upwd;
  char *htclfile;               /* HTML-clean file name */

  if (!output_stream)
    {
      fp = fopen (file, "wb");
      if (!fp)
        {
          logprintf (LOG_NOTQUIET, "%s: %s\n", file, strerror (errno));
          return FOPENERR;
        }
    }
  else
    fp = output_stream;
  if (u->user)
    {
      char *tmpu, *tmpp;        /* temporary, clean user and passwd */

      tmpu = url_escape (u->user);
      tmpp = u->passwd ? url_escape (u->passwd) : NULL;
      if (tmpp)
        upwd = concat_strings (tmpu, ":", tmpp, "@", (char *) 0);
      else
        upwd = concat_strings (tmpu, "@", (char *) 0);
      xfree (tmpu);
      xfree_null (tmpp);
    }
  else
    upwd = xstrdup ("");
  fprintf (fp, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
  fprintf (fp, "<html>\n<head>\n<title>");
  fprintf (fp, _("Index of /%s on %s:%d"), u->dir, u->host, u->port);
  fprintf (fp, "</title>\n</head>\n<body>\n<h1>");
  fprintf (fp, _("Index of /%s on %s:%d"), u->dir, u->host, u->port);
  fprintf (fp, "</h1>\n<hr>\n<pre>\n");
  while (f)
    {
      fprintf (fp, "  ");
      if (f->tstamp != -1)
        {
          /* #### Should we translate the months?  Or, even better, use
             ISO 8601 dates?  */
          static const char *months[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
          };
          time_t tstamp = f->tstamp;
          struct tm *ptm = localtime (&tstamp);

          fprintf (fp, "%d %s %02d ", ptm->tm_year + 1900, months[ptm->tm_mon],
                  ptm->tm_mday);
          if (ptm->tm_hour)
            fprintf (fp, "%02d:%02d  ", ptm->tm_hour, ptm->tm_min);
          else
            fprintf (fp, "       ");
        }
      else
        fprintf (fp, _("time unknown       "));
      switch (f->type)
        {
        case FT_PLAINFILE:
          fprintf (fp, _("File        "));
          break;
        case FT_DIRECTORY:
          fprintf (fp, _("Directory   "));
          break;
        case FT_SYMLINK:
          fprintf (fp, _("Link        "));
          break;
        default:
          fprintf (fp, _("Not sure    "));
          break;
        }
      htclfile = html_quote_string (f->name);
      fprintf (fp, "<a href=\"ftp://%s%s:%d", upwd, u->host, u->port);
      if (*u->dir != '/')
        putc ('/', fp);
      fprintf (fp, "%s", u->dir);
      if (*u->dir)
        putc ('/', fp);
      fprintf (fp, "%s", htclfile);
      if (f->type == FT_DIRECTORY)
        putc ('/', fp);
      fprintf (fp, "\">%s", htclfile);
      if (f->type == FT_DIRECTORY)
        putc ('/', fp);
      fprintf (fp, "</a> ");
      if (f->type == FT_PLAINFILE)
        fprintf (fp, _(" (%s bytes)"), number_to_static_string (f->size));
      else if (f->type == FT_SYMLINK)
        fprintf (fp, "-> %s", f->linkto ? f->linkto : "(nil)");
      putc ('\n', fp);
      xfree (htclfile);
      f = f->next;
    }
  fprintf (fp, "</pre>\n</body>\n</html>\n");
  xfree (upwd);
  if (!output_stream)
    fclose (fp);
  else
    fflush (fp);
  return FTPOK;
}
