/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_pcre.c
 *    functions to remmap file name requested by tftp clients according
 *    to regular expression rules
 *
 * $Id: tftpd_pcre.c,v 1.2 2003/04/25 00:16:19 jp Exp $
 *
 * Copyright (c) 2003 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * The PCRE code is provided by Jeff Miller <jeff.miller@transact.com.au>
 *
 * Copyright (c) 2003 Jeff Miller <jeff.miller@transact.com.au>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#include "config.h"

#if HAVE_PCRE

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "tftp_def.h"
#include "config.h"
#include "logger.h"

#include "tftpd_pcre.h"


/* 
 * number of elements in vector to hold substring info
 */
#define OVECCOUNT 30

/*
 * define the pattern for substitutions
 *    $0 whole string
 *    $1 to $9 substring 1 - 9
 */

/* create a pattern list from a file */
/* return 0 on success, -1 otherwise */
tftpd_pcre_self_t *tftpd_pcre_open(char *filename)
{
     int linecount;
     int erroffset;
     int matches;
     int ovector[OVECCOUNT];
     char line[MAXLEN];
     const char *error;
     FILE *fh;
     int subnum;
     char **substrlist;
     pcre *file_re;
     pcre_extra *file_pe;
     tftpd_pcre_self_t *self;
     tftpd_pcre_pattern_t *pat, **curpatp;

     /* open file */
     if ((fh = fopen(filename, "r")) == NULL)
     {
          logger(LOG_ERR, "Cannot open %s for reading: %s",
                 filename, strerror(errno));
          return NULL;
     }

     /* compile and study pattern for lines */
     logger(LOG_DEBUG, "Using file pattern %s", TFTPD_PCRE_FILE_PATTERN);
     if ((file_re = pcre_compile(TFTPD_PCRE_FILE_PATTERN, 0,
                                 &error, &erroffset, NULL)) == NULL)
     {
          logger(LOG_ERR, "PCRE file pattern failed to compile");
          return NULL;
     }

     file_pe = pcre_study(file_re, 0, &error);
     if (error != NULL)
     {
          logger(LOG_ERR, "PCRE file pattern failed to study");
          return NULL;
     }
    
     /* allocate header  and copy info */
     if ((self = calloc(1, sizeof(tftpd_pcre_self_t))) == NULL)
     {
          logger(LOG_ERR, "calloc filed");
          return NULL;
     }
     self->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
     Strncpy(self->filename, filename, MAXLEN);

     /* read patterns */
     for (linecount = 1, curpatp = &self->list;
          fgets(line, MAXLEN, fh) != NULL;
          linecount++, curpatp = &pat->next)
     {
          logger(LOG_DEBUG,"file: %s line: %d value: %s",
                 filename, linecount, line);
          
          /* allocate space for pattern info */
          if ((pat = (tftpd_pcre_pattern_t *)calloc(1,sizeof(tftpd_pcre_pattern_t))) == NULL) 
          {
               tftpd_pcre_close(self);
               return NULL;
          }
          *curpatp = pat;

          /* for each pattern read, compile and store the pattern */
          matches = pcre_exec(file_re, file_pe, line, (int)(strlen(line)),
                              0, 0, ovector, OVECCOUNT);
          /* log substring to help with debugging */
          pcre_get_substring_list(line, ovector, matches, (const char ***)&substrlist);
          for(subnum = 0; subnum <= matches; subnum++)
          {
               logger(LOG_DEBUG,"file: %s line: %d substring: %d value: %s",
                      filename, linecount, subnum, substrlist[subnum]);
          }
          pcre_free_substring_list((const char **)substrlist);
    
          if (matches < 2)
          {
               logger(LOG_ERR, "error with pattern in file \"%s\" line %d",
                      filename, linecount);
               tftpd_pcre_close(self);
               pcre_free(file_re);
               pcre_free(file_pe);
               return NULL;
          }
          /* remember line number */
          pat->linenum = linecount;
          
          /* extract left side */
          pcre_get_substring(line, ovector, matches, 1, (const char **)&pat->pattern);
    
          /* extract right side */
          pcre_get_substring(line, ovector, matches, 2, (const char **)&pat->right_str);
          logger(LOG_DEBUG,"pattern: %s right_str: %s", pat->pattern, pat->right_str);
          
          if ((pat->left_re = pcre_compile(pat->pattern, 0,
                                           &error, &erroffset, NULL)) == NULL)
          {
               /* compilation failed*/
               logger(LOG_ERR,
                      "PCRE compilation failed in file \"%s\" line %d at %d: %s",
                      filename, linecount,
                      erroffset, error);
               /* close file */
               fclose(fh);
               /* clean up */
               tftpd_pcre_close(self);
               return NULL;
          }
          /* we're going to be using this pattern a fair bit so lets study it */
          pat->left_pe = pcre_study(pat->left_re, 0, &error);
          if (error != NULL)
          {
               logger(LOG_ERR,
                      "PCRE study failed in file \"%s\" line %d: %s",
                      filename, linecount,
                      error);
               /* close file */
               fclose(fh);
               /* cleanup */
               tftpd_pcre_close(self);
               return NULL;
          }
     }
     /* clean up */
     pcre_free(file_re);
     pcre_free(file_pe);
     /* close file */
     fclose(fh);
     return self;
}

/* return filename being used */
/* returning a char point directly is a little risking when
 * using thread, but as we're using this before threads
 * are created we should be able to get away with it
 */
char *tftpd_pcre_getfilename(tftpd_pcre_self_t *self)
{
     return self->filename;
}

/* this is a utility function used to make the actual substitution*/
int tftpd_pcre_makesub(struct tftpd_pcre_pattern *pat,
		       char *outstr, int outsize,
		       char *str,
		       int *ovector, int matches)
{
     char *chp, *outchp;
     const char *tmpstr;
     int rc;
     
     /* $0  - whole string, $1-$9 substring 1-9   */                       
     for (chp = pat->right_str, outchp = outstr;
          (*chp != '\0') && (outchp - outstr < outsize);
          chp++)
     {
          if ((*chp == '$') && (*(chp+1) >= '0') && (*(chp+1) <= '9'))
          {
               chp++; /* point to value indicating substring */
               rc = pcre_get_substring(str, ovector, matches, *chp - 0x30, &tmpstr);
               /* found string */
               if (rc > 0)
               {
                    Strncpy(outchp, tmpstr, rc);
                    outchp += rc;
                    pcre_free_substring(tmpstr);
                    continue;
               }
               /* erro condition */
               switch (rc)
               {
               case PCRE_ERROR_NOMEMORY:
                    logger(LOG_ERR, "PCRE out of memory");
                    break;
               case PCRE_ERROR_NOSUBSTRING:
                    logger(LOG_ERR,
                           "PCRE attempted substitution failed for \"%s\" on pattern %d",
                           str, pat->linenum);
                    break;
               }
          }
          else
          {
               *outchp = *chp;
               outchp++;
          }
     }
     *outchp = '\0';
     return 0;
}

/* search for a replacement and return a string after substituation */
/* if no match is found return -1 */
int tftpd_pcre_sub(tftpd_pcre_self_t *self, char *outstr, int outlen, char *str)
{
     int rc;
     int ovector[OVECCOUNT];
     int matches;
     tftpd_pcre_pattern_t *pat;
     
     /* lock for duration */
     pthread_mutex_lock(&self->lock);
     
     logger(LOG_DEBUG, "Looking to match \"%s\"", str);
     /* interate over pattern list */
     for(pat = self->list; pat != NULL; pat = pat->next)
     {
          logger(LOG_DEBUG,"Attempting to match \"%s\"", pat->pattern);
          /* attempt match */
          matches = pcre_exec(pat->left_re, pat->left_pe,
                              str, (int)(strlen(str)),
                              0, 0,
                              ovector, OVECCOUNT);
          
          /* no match so we try again */
          if (matches == PCRE_ERROR_NOMATCH)
               continue;
          /* error in making a match - log and attempt to continue */
          if (matches < 0)
          {
               logger(LOG_WARNING,
                      "PCRE Matching error %d", matches);
               continue;
          }
          /* we have a match  - carry out substitution */
          logger(LOG_DEBUG,"Pattern \"%s\" matches", pat->pattern);
          rc = tftpd_pcre_makesub(pat,
                                  outstr, outlen,
                                  str,
                                  ovector, matches);
          logger(LOG_DEBUG,"outstr: \"%s\"", outstr);
          pthread_mutex_unlock(&self->lock);
          return 0;
     }
     logger(LOG_DEBUG, "Failed to match \"%s\"", str);
     pthread_mutex_unlock(&self->lock);
     return -1;
}

/* clean up and displose of anything we set up*/
void tftpd_pcre_close(tftpd_pcre_self_t *self)
{
     tftpd_pcre_pattern_t *next, *cur;

     /* free up list */
     pthread_mutex_lock(&self->lock);
     
     cur = self->list;
     while (cur != NULL)
     {
          next = cur->next;
          pcre_free_substring(cur->pattern);
          pcre_free(cur->left_re);
          pcre_free(cur->left_pe);
          pcre_free_substring(cur->right_str);
          free(cur);
          cur = next;
     }
     pthread_mutex_unlock(&self->lock);
     free(self);
}

#endif
