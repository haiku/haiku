/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: read/write and client interface for comment header packet
  last mod: $Id: comment.c,v 1.1 2004/02/24 13:50:13 shatty Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "encoder_internal.h"

void theora_comment_init(theora_comment *tc){
  memset(tc,0,sizeof(*tc));
}

void theora_comment_add(theora_comment *tc,char *comment){
  tc->user_comments=_ogg_realloc(tc->user_comments,
                            (tc->comments+2)*sizeof(*tc->user_comments));
  tc->comment_lengths=_ogg_realloc(tc->comment_lengths,
                            (tc->comments+2)*sizeof(*tc->comment_lengths));
  tc->comment_lengths[tc->comments]=strlen(comment);
  tc->user_comments[tc->comments]=_ogg_malloc(tc->comment_lengths[tc->comments]+1);
  strcpy(tc->user_comments[tc->comments], comment);
  tc->comments++;
  tc->user_comments[tc->comments]=NULL;
}

void theora_comment_add_tag(theora_comment *tc, char *tag, char *value){
  char *comment=_ogg_malloc(strlen(tag)+strlen(value)+2); /* +2 for = and \0 */
  strcpy(comment, tag);
  strcat(comment, "=");
  strcat(comment, value);
  theora_comment_add(tc, comment);
  _ogg_free(comment);
}

/* This is more or less the same as strncasecmp - but that doesn't exist
 * everywhere, and this is a fairly trivial function, so we include it */
static int tagcompare(const char *s1, const char *s2, int n){
  int c=0;
  while(c < n){
    if(toupper(s1[c]) != toupper(s2[c]))
      return !0;
    c++;
  }
  return 0;
}

char *theora_comment_query(theora_comment *tc, char *tag, int count){
  long i;
  int found = 0;
  int taglen = strlen(tag)+1; /* +1 for the = we append */
  char *fulltag = _ogg_malloc(taglen+ 1);

  strcpy(fulltag, tag);
  strcat(fulltag, "=");

  for(i=0;i<tc->comments;i++){
    if(!tagcompare(tc->user_comments[i], fulltag, taglen)){
      if(count == found){
        _ogg_free(fulltag);
        /* We return a pointer to the data, not a copy */
        return tc->user_comments[i] + taglen;
      }
      else
        found++;
    }
  }
  _ogg_free(fulltag);
  return NULL; /* didn't find anything */
}

int theora_comment_query_count(theora_comment *tc, char *tag){
  int i,count=0;
  int taglen = strlen(tag)+1; /* +1 for the = we append */
  char *fulltag = _ogg_malloc(taglen+1);
  strcpy(fulltag,tag);
  strcat(fulltag, "=");

  for(i=0;i<tc->comments;i++){
    if(!tagcompare(tc->user_comments[i], fulltag, taglen))
      count++;
  }
  _ogg_free(fulltag);
  return count;
}

void theora_comment_clear(theora_comment *tc){
  if(tc){
    long i;
    for(i=0;i<tc->comments;i++)
      if(tc->user_comments[i])_ogg_free(tc->user_comments[i]);
    if(tc->user_comments)_ogg_free(tc->user_comments);
        if(tc->comment_lengths)_ogg_free(tc->comment_lengths);
    if(tc->vendor)_ogg_free(tc->vendor);
  }
  memset(tc,0,sizeof(*tc));
}

