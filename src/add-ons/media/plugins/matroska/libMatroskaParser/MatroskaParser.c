/*
 * Copyright (c) 2004 Mike Matsnev.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    Mike Matsnev.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * $Id: MatroskaParser.c,v 1.8 2004/09/17 12:23:51 mike Exp $
 * 
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifdef _WIN32
// MS names some functions differently
#define	vsnprintf _vsnprintf
#define	alloca	  _alloca
#define	inline	  __inline

#include <tchar.h>
#endif

#include <zlib.h>

#ifndef EVCBUG
#define	EVCBUG
#endif

#include "MatroskaParser.h"

#define	EBML_VERSION	      1
#define	EBML_MAX_ID_LENGTH    4
#define	EBML_MAX_SIZE_LENGTH  8
#define	MATROSKA_VERSION      1
#define	MATROSKA_DOCTYPE      "matroska"

#define	MAX_STRING_LEN	      1023
#define	QSEGSIZE	      512
#define	MAX_TRACKS	      32
#define	MAX_READAHEAD	      (384*1024)

#define	DEFAULT_PAGE_SIZE     65536
#define	MIN_PAGE_SIZE	      4096
#define	MIN_PAGES	      2
#define	MAXCLUSTER	      (64*1048576)
#define	MAXFRAME	      (4*1048576)

#define	MAXU64		      0xffffffffffffffff

// compatibility
#ifdef _WIN32_WCE
static char  *strdup(const char *src) {
  size_t  len;
  char	  *dst;

  if (src==NULL)
    return NULL;

  len = strlen(src);
  dst = malloc(len+1);
  if (dst==NULL)
    return NULL;

  memcpy(dst,src,len+1);

  return dst;
}
#endif

#ifdef _WIN32
static void  strlcpy(char *dst,const char *src,unsigned size) {
  unsigned  i;

  for (i=0;i+1<size && src[i];++i)
    dst[i] = src[i];
  if (i<size)
    dst[i] = 0;
}
#endif

struct Cue {
  ulonglong	Time;
  ulonglong	Position;
  ulonglong	Block;
  unsigned char	Track;
};

struct QueueEntry {
  struct QueueEntry *next;
  unsigned int	    Length;

  ulonglong	    Start;
  ulonglong	    End;
  ulonglong	    Position;

  unsigned int	    flags;
};

struct Queue {
  struct QueueEntry *head;
  struct QueueEntry *tail;
};

typedef struct FileCache FileCache;

#define	MPF_ERROR 0x10000
#define	IBSZ	  1024

#define	RBRESYNC  1

struct MatroskaFile {
  // parser config
  unsigned    flags;

  // input
  FileCache   *cache;

  // internal buffering
  char	      inbuf[IBSZ];
  ulonglong   bufbase; // file offset of the first byte in buffer
  int	      bufpos; // current read position in buffer
  int	      buflen; // valid bytes in buffer

  // error reporting
  char	      errmsg[128];
  jmp_buf     jb;

  // pointers to key elements
  ulonglong   pSegment;
  ulonglong   pSeekHead;
  ulonglong   pSegmentInfo;
  ulonglong   pCluster;
  ulonglong   pTracks;
  ulonglong   pCues;
  ulonglong   pAttachments;
  ulonglong   pChapters;
  ulonglong   pTags;

  // flags for key elements
  struct {
    unsigned int  SegmentInfo:1;
    unsigned int  Cluster:1;
    unsigned int  Tracks:1;
    unsigned int  Cues:1;
    unsigned int  Attachments:1;
    unsigned int  Chapters:1;
    unsigned int  Tags:1;
  } seen;

  // file info
  ulonglong   firstTimecode;

  // SegmentInfo
  struct SegmentInfo  Seg;

  // Tracks
  unsigned int	    nTracks,nTracksSize;
  struct TrackInfo  **Tracks;

  // Queues
  struct QueueEntry *QFreeList;
  unsigned int	    nQBlocks,nQBlocksSize;
  struct QueueEntry **QBlocks;
  struct Queue	    *Queues;
  ulonglong	    readPosition;
  unsigned int	    trackMask;
  ulonglong	    pSegmentTop;  // offset of next byte after the segment
  ulonglong	    tcCluster;    // current cluster timecode

  // Cues
  unsigned int	    nCues,nCuesSize;
  struct Cue	    *Cues;

  // Attachments
  unsigned int	    nAttachments,nAttachmentsSize;
  struct Attachment *Attachments;

  // Chapters
  unsigned int	    nChapters,nChaptersSize;
  struct Chapter    *Chapters;

  // Tags
  unsigned int	    nTags,nTagsSize;
  struct Tag	    *Tags;
};

///////////////////////////////////////////////////////////////////////////
// error reporting
static void   errorjmp(MatroskaFile *mf,const char *fmt, ...) {
  va_list   ap;

  va_start(ap, fmt);
  vsnprintf(mf->errmsg,sizeof(mf->errmsg),fmt,ap);
  va_end(ap);

  mf->flags |= MPF_ERROR;

  longjmp(mf->jb,1);
}

///////////////////////////////////////////////////////////////////////////
// arrays
static void *ArrayAlloc(MatroskaFile *mf,void **base,
			unsigned *cur,unsigned *max,unsigned elem_size)
{
  if (*cur>=*max) {
    void      *np;
    unsigned  newsize = *max * 2;
    if (newsize==0)
      newsize = 1;

    np = realloc(*base,newsize*elem_size);
    if (np==NULL)
      errorjmp(mf,"Out of memory in ArrayAlloc");

    *base = np;
    *max = newsize;
  }

  return (char*)*base + elem_size * (*cur)++;
}

static void ArrayReleaseMemory(void **base,
			       unsigned cur,unsigned *max,unsigned elem_size)
{
  if (cur<*max) {
    void  *np = realloc(*base,cur*elem_size);
    *base = np;
    *max = cur;
  }
}


#define	ASGET(f,s,name)	  ArrayAlloc((f),(void**)&(s)->name,&(s)->n##name,&(s)->n##name##Size,sizeof(*((s)->name)))
#define	AGET(f,name)	  ArrayAlloc((f),(void**)&(f)->name,&(f)->n##name,&(f)->n##name##Size,sizeof(*((f)->name)))
#define	ARELEASE(f,name)  ArrayReleaseMemory((void**)&(f)->name,(f)->n##name,&(f)->n##name##Size,sizeof(*((f)->name)))

///////////////////////////////////////////////////////////////////////////
// queues
static struct QueueEntry *QPut(struct Queue *q,struct QueueEntry *qe) {
  if (q->tail)
    q->tail->next = qe;
  qe->next = NULL;
  q->tail = qe;
  if (q->head==NULL)
    q->head = qe;

  return qe;
}

static struct QueueEntry  *QGet(struct Queue *q) {
  struct QueueEntry   *qe = q->head;
  if (qe == NULL)
    return NULL;
  q->head = qe->next;
  if (q->tail == qe)
    q->tail = NULL;
  return qe;
}

static struct QueueEntry  *QAlloc(MatroskaFile *mf) {
  struct QueueEntry   *qe,**qep;
  if (mf->QFreeList == NULL) {
    unsigned	      i;

    qep = AGET(mf,QBlocks);

    *qep = malloc(QSEGSIZE * sizeof(*qe));
    if (*qep == NULL)
      errorjmp(mf,"Ouf of memory");

    qe = *qep;

    for (i=0;i<QSEGSIZE-1;++i)
      qe[i].next = qe+i+1;
    qe[QSEGSIZE-1].next = NULL;

    mf->QFreeList = qe;
  }

  qe = mf->QFreeList;
  mf->QFreeList = qe->next;

  return qe;
}

static inline void QFree(MatroskaFile *mf,struct QueueEntry *qe) {
  qe->next = mf->QFreeList;
  mf->QFreeList = qe;
}

// fill the buffer at current position
static void fillbuf(MatroskaFile *mf) {
  int	    rd;

  // advance buffer pointers
  mf->bufbase += mf->buflen;
  mf->buflen = mf->bufpos = 0;

  // get the relevant page
  rd = mf->cache->read(mf->cache, mf->bufbase, mf->inbuf, IBSZ);
  if (rd<0)
    errorjmp(mf,"I/O Error: %s",mf->cache->geterror(mf->cache));

  mf->buflen = rd;
}

// fill the buffer and return next char
static int  nextbuf(MatroskaFile *mf) {
  fillbuf(mf);

  if (mf->bufpos < mf->buflen)
    return (unsigned char)(mf->inbuf[mf->bufpos++]);

  return EOF;
}

static inline int readch(MatroskaFile *mf) {
  return mf->bufpos < mf->buflen ? (unsigned char)(mf->inbuf[mf->bufpos++]) : nextbuf(mf);
}

static inline ulonglong	filepos(MatroskaFile *mf) {
  return mf->bufbase + mf->bufpos;
}

static void   readbytes(MatroskaFile *mf,void *buffer,int len) {
  char	*cp = buffer;
  int	nb = mf->buflen - mf->bufpos;

  if (nb > len)
    nb = len;

  memcpy(cp, mf->inbuf + mf->bufpos, nb);
  mf->bufpos += nb;
  len -= nb;
  cp += nb;

  if (len>0) {
    mf->bufbase += mf->buflen;
    mf->bufpos = mf->buflen = 0;

    nb = mf->cache->read(mf->cache, mf->bufbase, cp, len);
    if (nb<0)
      errorjmp(mf,"I/O Error: %s",mf->cache->geterror(mf->cache));
    if (nb != len)
      errorjmp(mf,"Short read: got %d bytes of %d",nb,len);
    mf->bufbase += len;
  }
}

static void   skipbytes(MatroskaFile *mf,ulonglong len) {
  int	    nb = mf->buflen - mf->bufpos;

  if (nb > len)
    nb = (int)len;

  mf->bufpos += nb;
  len -= nb;

  if (len>0) {
    mf->bufbase += mf->buflen;
    mf->bufpos = mf->buflen = 0;

    mf->bufbase += len;
  }
}

static void seek(MatroskaFile *mf,ulonglong pos) {
  // see if pos is inside buffer
  if (pos>=mf->bufbase && pos<mf->bufbase+mf->buflen)
    mf->bufpos = (unsigned)(pos - mf->bufbase);
  else {
    // invalidate buffer and set pointer
    mf->bufbase = pos;
    mf->buflen = mf->bufpos = 0;
  }
}

///////////////////////////////////////////////////////////////////////////
// floating point
static inline MKFLOAT	mkfi(int i) {
#ifdef MATROSKA_INTEGER_ONLY
  MKFLOAT  f;
  f.v = (longlong)i << 32;
  return f;
#else
  return i;
#endif
}

static inline longlong mul3(MKFLOAT scale,longlong tc) {
#ifdef MATROSKA_INTEGER_ONLY
  //             x1 x0
  //             y1 y0
  //    --------------
  //             x0*y0
  //          x1*y0
  //          x0*y1
  //       x1*y1
  //    --------------
  //       .. r1 r0 ..
  //
  //    r = ((x0*y0) >> 32) + (x1*y0) + (x0*y1) + ((x1*y1) << 32)
  // TODO calc tc*scale
  unsigned    x0,x1,y0,y1;
  ulonglong   p;
  char	      sign = 0;

  if (scale.v < 0)
    sign = !sign, scale.v = -scale.v;
  if (tc < 0)
    sign = !sign, tc = -tc;

  x0 = (unsigned)scale.v;
  x1 = (unsigned)((ulonglong)scale.v >> 32);
  y0 = (unsigned)tc;
  y1 = (unsigned)((ulonglong)tc >> 32);

  p = (ulonglong)x0*y0 >> 32;
  p += (ulonglong)x0*y1;
  p += (ulonglong)x1*y0;
  p += (ulonglong)(x1*y1) << 32;

  return p;
#else
  return (ulonglong)(scale * tc);
#endif
}

///////////////////////////////////////////////////////////////////////////
// EBML support
static int   readID(MatroskaFile *mf) {
  int	c1,c2,c3,c4;

  c1 = readch(mf);
  if (c1 == EOF)
    return EOF;

  if (c1 & 0x80)
    return c1;

  if ((c1 & 0xf0) == 0)
    errorjmp(mf,"Invalid first byte of EBML ID: %02X",c1);

  c2 = readch(mf);
  if (c2 == EOF)
fail:
    errorjmp(mf,"Got EOF while reading EBML ID");

  if ((c1 & 0xc0) == 0x40)
    return (c1<<8) | c2;

  c3 = readch(mf);
  if (c3 == EOF)
    goto fail;

  if ((c1 & 0xe0) == 0x20)
    return (c1<<16) | (c2<<8) | c3;

  c4 = readch(mf);
  if (c4 == EOF)
    goto fail;

  if ((c1 & 0xf0) == 0x10)
    return (c1<<24) | (c2<<16) | (c3<<8) | c4;

  return 0; // NOT REACHED
}

static ulonglong readVLUIntImp(MatroskaFile *mf,int *mask) {
  int		c,d,m;
  ulonglong	v = 0;

  c = readch(mf);
  if (c == EOF)
    return EOF;

  if (c == 0)
    errorjmp(mf,"Invalid first byte of EBML integer: 0");

  for (m=0;;++m) {
    if (c & (0x80 >> m)) {
      c &= 0x7f >> m;
      if (mask)
	*mask = m;
      return v | ((ulonglong)c << m*8);
    }
    d = readch(mf);
    if (d == EOF)
      errorjmp(mf,"Got EOF while reading EBML unsigned integer");
    v = (v<<8) | d;
  }
  // NOT REACHED
}

static inline ulonglong	readVLUInt(MatroskaFile *mf) {
  return readVLUIntImp(mf,NULL);
}

static ulonglong	readSize(MatroskaFile *mf) {
  int	    m;
  ulonglong v = readVLUIntImp(mf,&m);

  // see if it's unspecified
  if (v == (0xffffffffffffffff >> (57-m*7)))
    errorjmp(mf,"Unspecified element size is not supported here.");

  return v;
}

static inline longlong	readVLSInt(MatroskaFile *mf) {
  static longlong bias[8] = { (1<<6)-1, (1<<13)-1, (1<<20)-1, (1<<27)-1,
			      (1<<34)-1, (1<<41)-1, (1<<48)-1, (1<<55)-1 };

  int	    m;
  longlong  v = readVLUIntImp(mf,&m);

  return v - bias[m];
}

static ulonglong  readUInt(MatroskaFile *mf,unsigned int len) {
  int		c;
  unsigned int	m = len;
  ulonglong	v = 0;

  if (len==0)
    return v;
  if (len>8)
    errorjmp(mf,"Unsupported integer size in readUInt: %u",len);

  do {
    c = readch(mf);
    if (c == EOF)
      errorjmp(mf,"Got EOF while read EBML unsigned integer");
    v = (v<<8) | c;
  } while (--m);

  return v;
}

static inline longlong	readSInt(MatroskaFile *mf,unsigned int len) {
  longlong	v = readUInt(mf,(unsigned)len);
  int		s = 64 - (len<<3);
  return (v << s) >> s;
}

static MKFLOAT readFloat(MatroskaFile *mf,unsigned int len) {
#ifdef MATROSKA_INTEGER_ONLY
  MKFLOAT	  f;
  int		  shift;
#else
  union {
    unsigned int  ui;
    ulonglong	  ull;
    float	  f;
    double	  d;
  } u;
#endif

  if (len!=4 && len!=8)
    errorjmp(mf,"Invalid float size in readFloat: %u",len);

#ifdef MATROSKA_INTEGER_ONLY
  if (len == 4) {
    unsigned  ui = (unsigned)readUInt(mf,(unsigned)len);
    f.v = (ui & 0x7fffff) | 0x800000;
    if (ui & 0x80000000)
      f.v = -f.v;
    shift = (ui >> 23) & 0xff;
    if (shift == 0) // assume 0
zero:
      shift = 0, f.v = 0;
    else if (shift == 255)
inf:
      if (ui & 0x80000000)
	f.v = 0x8000000000000000;
      else
	f.v = 0x7fffffffffffffff;
    else {
      shift += -127 + 9;
      if (shift > 39)
	goto inf;
shift:
      if (shift < 0)
	f.v = f.v >> -shift;
      else if (shift > 0)
	f.v = f.v << shift;
    }
  }

  if (len == 8) {
    ulonglong  ui = (unsigned)readUInt(mf,(unsigned)len);
    f.v = (ui & 0xfffffffffffff) | 0x10000000000000;
    if (ui & 0x80000000)
      f.v = -f.v;
    shift = (int)((ui >> 52) & 0x7ff);
    if (shift == 0) // assume 0
      goto zero;
    else if (shift == 2047)
      goto inf;
    else {
      shift += -1023 - 20;
      if (shift > 10)
	goto inf;
      goto shift;
    }
  }

  return f;
#else
  if (len==4) {
    u.ui = (unsigned int)readUInt(mf,(unsigned)len);
    return u.f;
  }

  if (len==8) {
    u.ull = readUInt(mf,(unsigned)len);
    return u.d;
  }

  return 0;
#endif
}

static void readString(MatroskaFile *mf,ulonglong len,char *buffer,int buflen) {
  int	  nread;

  if (buflen<1)
    errorjmp(mf,"Invalid buffer size in readString: %d",buflen);

  nread = buflen - 1;

  if (nread > len)
    nread = (int)len;

  readbytes(mf,buffer,nread);
  len -= nread;

  if (len>0)
    skipbytes(mf,len);

  buffer[nread] = '\0';
}

///////////////////////////////////////////////////////////////////////////
// file parser
#define	FOREACH(f,tl) \
  { \
    ulonglong	tmplen = (tl); \
    { \
      ulonglong   start = filepos(f); \
      ulonglong   cur,len; \
      int	      id; \
      for (;;) { \
	cur = filepos(mf); \
	if (cur == start + tmplen) \
	  break; \
	id = readID(f); \
	if (id==EOF) \
	  errorjmp(mf,"Unexpected EOF while reading EBML container"); \
	len = readSize(mf); \
	switch (id) {

#define	ENDFOR1(f) \
	default: \
	  skipbytes(f,len); \
	  break; \
	}
#define	ENDFOR2() \
      } \
    } \
  }

#define	ENDFOR(f) ENDFOR1(f) ENDFOR2()

#define	STRGETF(f,v,len,func) \
  { \
    char *TmpVal; \
    unsigned TmpLen = (len)>MAX_STRING_LEN ? MAX_STRING_LEN : (unsigned)(len); \
    TmpVal = func(TmpLen+1); \
    if (TmpVal == NULL) \
      errorjmp(mf,"Out of memory"); \
    readString(f,len,TmpVal,TmpLen+1); \
    (v) = TmpVal; \
  }

#define	STRGETA(f,v,len)  STRGETF(f,v,len,alloca)
#define	STRGETM(f,v,len)  STRGETF(f,v,len,malloc)

static int  IsWritingApp(MatroskaFile *mf,const char *str) {
  const char  *cp = mf->Seg.WritingApp;
  if (!cp)
    return 0;

  while (*str && *str++==*cp++) ;

  return !*str;
}

static void parseEBML(MatroskaFile *mf,ulonglong toplen) {
  ulonglong v;
  char	    buf[32];

  FOREACH(mf,toplen)
    case 0x4286: // Version
      v = readUInt(mf,(unsigned)len);
      break;
    case 0x42f7: // ReadVersion
      v = readUInt(mf,(unsigned)len);
      if (v > EBML_VERSION)
	errorjmp(mf,"File requires version %d EBML parser",(int)v);
      break;
    case 0x42f2: // MaxIDLength
      v = readUInt(mf,(unsigned)len);
      if (v > EBML_MAX_ID_LENGTH)
	errorjmp(mf,"File has identifiers longer than %d",(int)v);
      break;
    case 0x42f3: // MaxSizeLength
      v = readUInt(mf,(unsigned)len);
      if (v > EBML_MAX_SIZE_LENGTH)
	errorjmp(mf,"File has integers longer than %d",(int)v);
      break;
    case 0x4282: // DocType
      readString(mf,len,buf,sizeof(buf));
      if (strcmp(buf,MATROSKA_DOCTYPE))
	errorjmp(mf,"Unsupported DocType: %s",buf);
      break;
    case 0x4287: // DocTypeVersion
      v = readUInt(mf,(unsigned)len);
      break;
    case 0x4285: // DocTypeReadVersion
      v = readUInt(mf,(unsigned)len);
      if (v > MATROSKA_VERSION)
	errorjmp(mf,"File requires version %d Matroska parser",(int)v);
      break;
  ENDFOR(mf);
}

static void parseSeekEntry(MatroskaFile *mf,ulonglong toplen) {
  int	    seekid = 0;
  ulonglong pos = (ulonglong)-1;

  FOREACH(mf,toplen)
    case 0x53ab: // SeekID
      if (len>EBML_MAX_ID_LENGTH)
	errorjmp(mf,"Invalid ID size in parseSeekEntry: %d\n",(int)len);
      seekid = (int)readUInt(mf,(unsigned)len);
      break;
    case 0x53ac: // SeekPos
      pos = readUInt(mf,(unsigned)len);
      break;
  ENDFOR(mf);

  if (pos == (ulonglong)-1)
    errorjmp(mf,"Invalid element position in parseSeekEntry");

  pos += mf->pSegment;
  switch (seekid) {
    case 0x114d9b74: // next SeekHead
      if (mf->pSeekHead)
	errorjmp(mf,"SeekHead contains more than one SeekHead pointer");
      mf->pSeekHead = pos;
      break;
    case 0x1549a966: // SegmentInfo
      mf->pSegmentInfo = pos;
      break;
    case 0x1f43b675: // Cluster
      if (!mf->pCluster)
	mf->pCluster = pos;
      break;
    case 0x1654ae6b: // Tracks
      mf->pTracks = pos;
      break;
    case 0x1c53bb6b: // Cues
      mf->pCues = pos;
      break;
    case 0x1941a469: // Attachments
      mf->pAttachments = pos;
      break;
    case 0x1043a770: // Chapters
      mf->pChapters = pos;
      break;
    case 0x1254c367: // tags
      mf->pTags = pos;
      break;
  }
}

static void parseSeekHead(MatroskaFile *mf,ulonglong toplen) {
  FOREACH(mf,toplen)
    case 0x4dbb:
      parseSeekEntry(mf,len);
      break;
  ENDFOR(mf);
}

static void parseSegmentInfo(MatroskaFile *mf,ulonglong toplen) {
  MKFLOAT     duration;

  if (mf->seen.SegmentInfo)
    return;

  mf->seen.SegmentInfo = 1;
  mf->Seg.TimecodeScale = 1000000; // Default value

  FOREACH(mf,toplen)
    case 0x73a4: // SegmentUID
      if (len!=sizeof(mf->Seg.UID))
	errorjmp(mf,"SegmentUID size is not %d bytes",mf->Seg.UID);
      readbytes(mf,mf->Seg.UID,sizeof(mf->Seg.UID));
      break;
    case 0x7384: // SegmentFilename
      STRGETM(mf,mf->Seg.Filename,len);
      break;
    case 0x3cb923: // PrevUID
      if (len!=sizeof(mf->Seg.PrevUID))
	errorjmp(mf,"PrevUID size is not %d bytes",mf->Seg.PrevUID);
      readbytes(mf,mf->Seg.PrevUID,sizeof(mf->Seg.PrevUID));
      break;
    case 0x3c83ab: // PrevFilename
      STRGETM(mf,mf->Seg.PrevFilename,len);
      break;
    case 0x3eb923: // NextUID
      if (len!=sizeof(mf->Seg.NextUID))
	errorjmp(mf,"NextUID size is not %d bytes",mf->Seg.NextUID);
      readbytes(mf,mf->Seg.NextUID,sizeof(mf->Seg.NextUID));
      break;
    case 0x3e83bb: // NextFilename
      STRGETM(mf,mf->Seg.NextFilename,len);
      break;
    case 0x2ad7b1: // TimecodeScale
      mf->Seg.TimecodeScale = readUInt(mf,(unsigned)len);
      break;
    case 0x4489: // Duration
      duration = readFloat(mf,(unsigned)len);
      break;
    case 0x4461: // DateUTC
      mf->Seg.DateUTC = readUInt(mf,(unsigned)len);
      break;
    case 0x7ba9: // Title
      STRGETM(mf,mf->Seg.Title,len);
      break;
    case 0x4d80: // MuxingApp
      STRGETM(mf,mf->Seg.MuxingApp,len);
      break;
    case 0x5741: // WritingApp
      STRGETM(mf,mf->Seg.WritingApp,len);
      break;
  ENDFOR(mf);

  mf->Seg.Duration = mul3(duration,mf->Seg.TimecodeScale);
}

static void parseFirstCluster(MatroskaFile *mf,ulonglong toplen) {
  mf->seen.Cluster = 1;
  mf->firstTimecode = 0;

  FOREACH(mf,toplen)
    case 0xe7: // Timecode
      mf->firstTimecode += readUInt(mf,(unsigned)len);
      break;
    case 0xa0: // BlockGroup
      FOREACH(mf,len)
	case 0xa1: // Block
	  readVLUInt(mf); // track number
	  mf->firstTimecode += readSInt(mf,2); 

	  skipbytes(mf,start + toplen - filepos(mf));
	  return;
      ENDFOR(mf);
      break;
  ENDFOR(mf);
}

static void parseVideoInfo(MatroskaFile *mf,ulonglong toplen,struct TrackInfo *ti) {
  ulonglong v;
  char	    dW = 0, dH = 0;

  FOREACH(mf,toplen)
    case 0x9a: // FlagInterlaced
      ti->Video.Interlaced = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x53b8: // StereoMode
      v = readUInt(mf,(unsigned)len);
      if (v>3)
	errorjmp(mf,"Invalid stereo mode");
      ti->Video.StereoMode = (unsigned char)v;
      break;
    case 0xb0: // PixelWidth
      v = readUInt(mf,(unsigned)len);
      if (v>0xffffffff)
	errorjmp(mf,"PixelWidth is too large");
      ti->Video.PixelWidth = (unsigned)v;
      if (!dW)
	ti->Video.DisplayWidth = ti->Video.PixelWidth;
      break;
    case 0xba: // PixelHeight
      v = readUInt(mf,(unsigned)len);
      if (v>0xffffffff)
	errorjmp(mf,"PixelHeight is too large");
      ti->Video.PixelHeight = (unsigned)v;
      if (!dH)
	ti->Video.DisplayHeight = ti->Video.PixelHeight;
      break;
    case 0x54b0: // DisplayWidth
      v = readUInt(mf,(unsigned)len);
      if (v>0xffffffff)
	errorjmp(mf,"DisplayWidth is too large");
      ti->Video.DisplayWidth = (unsigned)v;
      dW = 1;
      break;
    case 0x54ba: // DisplayHeight
      v = readUInt(mf,(unsigned)len);
      if (v>0xffffffff)
	errorjmp(mf,"DisplayHeight is too large");
      ti->Video.DisplayHeight = (unsigned)v;
      dH = 1;
      break;
    case 0x54b2: // DisplayUnit
      v = readUInt(mf,(unsigned)len);
      if (v>2)
	errorjmp(mf,"Invalid DisplayUnit: %d",(int)v);
      ti->Video.DisplayUnit = (unsigned char)v;
      break;
    case 0x54b3: // AspectRatioType
      v = readUInt(mf,(unsigned)len);
      if (v>2)
	errorjmp(mf,"Invalid AspectRatioType: %d",(int)v);
      ti->Video.AspectRatioType = (unsigned char)v;
      break;
    case 0x2eb524: // ColourSpace
      ti->Video.ColourSpace = (unsigned)readUInt(mf,4);
      break;
    case 0x2fb523: // GammaValue
      ti->Video.GammaValue = readFloat(mf,(unsigned)len);
      break;
  ENDFOR(mf);
}

static void parseAudioInfo(MatroskaFile *mf,ulonglong toplen,struct TrackInfo *ti) {
  ulonglong   v;

  FOREACH(mf,toplen)
    case 0xb5: // SamplingFrequency
      ti->Audio.SamplingFreq = readFloat(mf,(unsigned)len);
      break;
    case 0x78b5: // OutputSamplingFrequency
      ti->Audio.OutputSamplingFreq = readFloat(mf,(unsigned)len);
      break;
    case 0x9f: // Channels
      v = readUInt(mf,(unsigned)len);
      if (v<1 || v>255)
	errorjmp(mf,"Invalid Channels value");
      ti->Audio.Channels = (unsigned char)v;
      break;
    case 0x7d7b: // ChannelPositions
      skipbytes(mf,len);
      break;
    case 0x6264: // BitDepth
      v = readUInt(mf,(unsigned)len);
      if ((v<1 || v>255) && !IsWritingApp(mf,"AVI-Mux GUI"))
	errorjmp(mf,"Invalid BitDepth: %d",(int)v);
      ti->Audio.BitDepth = (unsigned char)v;
      break;
  ENDFOR(mf);

  if (mkv_TruncFloat(ti->Audio.OutputSamplingFreq)==0)
    ti->Audio.OutputSamplingFreq = ti->Audio.SamplingFreq;
}

static void CopyStr(char **src,char **dst) {
  size_t l;

  if (!*src)
    return;

  l = strlen(*src)+1;
  memcpy(*dst,*src,l);
  *src = *dst;
  *dst += l;
}

static void parseTrackEntry(MatroskaFile *mf,ulonglong toplen) {
  struct TrackInfo  t,*tp,**tpp;
  ulonglong	    v;
  char		    *cp = NULL;
  size_t	    cplen = 0, cpadd = 0;

  if (mf->nTracks >= MAX_TRACKS)
    errorjmp(mf,"Too many tracks.");

  // clear track info
  memset(&t,0,sizeof(t));

  // fill default values
  t.Enabled = 1;
  t.Default = 1;
  t.Lacing = 1;
  t.TimecodeScale = mkfi(1);
  t.DecodeAll = 1;

  FOREACH(mf,toplen)
    case 0xd7: // TrackNumber
      v = readUInt(mf,(unsigned)len);
      if (v>255)
	errorjmp(mf,"Track number is >255 (%d)",(int)v);
      t.Number = (unsigned char)v;
      break;
    case 0x73c5: // TrackUID
      t.UID = readUInt(mf,(unsigned)len);
      break;
    case 0x83: // TrackType
      v = readUInt(mf,(unsigned)len);
      if (v<1 || v>254)
	errorjmp(mf,"Invalid track type: %d",(int)v);
      t.Type = (unsigned char)v;
      if (t.Type == 2) { // fill defaults for audio tracks
	t.Audio.Channels = 1;
	t.Audio.SamplingFreq = mkfi(8000);
      }
      break;
    case 0xb9: // Enabled
      t.Enabled = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x88: // Default
      t.Default = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x9c: // Lacing
      t.Lacing = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x6de7: // MinCache
      v = readUInt(mf,(unsigned)len);
      if (v > 0xffffffff)
	errorjmp(mf,"MinCache is too large");
      t.MinCache = (unsigned)v;
      break;
    case 0x6df8: // MaxCache
      v = readUInt(mf,(unsigned)len);
      if (v > 0xffffffff)
	errorjmp(mf,"MaxCache is too large");
      t.MaxCache = (unsigned)v;
      break;
    case 0x23e383: // DefaultDuration
      t.DefaultDuration = readUInt(mf,(unsigned)len);
      break;
    case 0x23314f: // TrackTimecodeScale
      t.TimecodeScale = readFloat(mf,(unsigned)len);
      break;
    case 0x536e: // Name
      if (t.Name)
	errorjmp(mf,"Duplicate Track Name");
      STRGETA(mf,t.Name,len);
      break;
    case 0x22b59c: // Language
      if (t.Language)
	errorjmp(mf,"Duplicate Track Language");
      STRGETA(mf,t.Language,len);
      break;
    case 0x86: // CodecID
      if (t.CodecID)
	errorjmp(mf,"Duplicate CodecID");
      STRGETA(mf,t.CodecID,len);
      break;
    case 0x63a2: // CodecPrivate
      if (cp)
	errorjmp(mf,"Duplicate CodecPrivate");
      if (len>262144) // 256KB
	errorjmp(mf,"CodecPrivate is too large: %d",(int)len);
      cplen = (unsigned)len;
      cp = alloca(cplen);
      readbytes(mf,cp,(int)cplen);
      break;
    case 0x258688: // CodecName
      skipbytes(mf,len);
      break;
    case 0x3a9697: // CodecSettings
      skipbytes(mf,len);
      break;
    case 0x3b4040: // CodecInfoURL
      skipbytes(mf,len);
      break;
    case 0x26b240: // CodecDownloadURL
      skipbytes(mf,len);
      break;
    case 0xaa: // CodecDecodeAll
      t.DecodeAll = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x6fab: // TrackOverlay
      v = readUInt(mf,(unsigned)len);
      if (v>255)
	errorjmp(mf,"Track number in TrackOverlay is too large: %d",(int)v);
      t.TrackOverlay = (unsigned char)v;
      break;
    case 0xe0: // VideoInfo
      parseVideoInfo(mf,len,&t);
      break;
    case 0xe1: // AudioInfo
      parseAudioInfo(mf,len,&t);
      break;
    case 0x6d80: // ContentEncodings
      FOREACH(mf,len)
	case 0x6240: // ContentEncoding
	  FOREACH(mf,len)
	    case 0x5031: // ContentEncodingOrder
	      readUInt(mf,(unsigned)len);
	      break;
	    case 0x5032: // ContentEncodingScope
	      readUInt(mf,(unsigned)len);
	      break;
	    case 0x5033: // ContentEncodingType
	      readUInt(mf,(unsigned)len);
	      break;
	    case 0x5034: // ContentCompression
	      // fill in defaults
	      t.CompEnabled = 1;
	      t.CompMethod = COMP_ZLIB;
	      FOREACH(mf,len)
		case 0x4254: // ContentCompAlgo
		  v = readUInt(mf,(unsigned)len);
		  if (v != 0)
		    errorjmp(mf,"Unsupported compression algorithm: %d",(int)v);
		  t.CompEnabled = 1;
		  t.CompMethod = COMP_ZLIB;
		  break;
		case 0x4255: // ContentCompSettings
		  skipbytes(mf,len);
		  break;
	      ENDFOR(mf);
	      break;
	      // TODO Implement Encryption/Signatures
	  ENDFOR(mf);
	  break;
      ENDFOR(mf);
      break;
  ENDFOR(mf);

  // validate track info
  if (!t.CodecID)
    errorjmp(mf,"Track has no Codec ID");

  // allocate new track
  tpp = AGET(mf,Tracks);

  // copy strings
  if (t.Name)
    cpadd += strlen(t.Name)+1;
  if (t.Language)
    cpadd += strlen(t.Language)+1;
  if (t.CodecID)
    cpadd += strlen(t.CodecID)+1;

  tp = malloc(sizeof(*tp) + cplen + cpadd);
  if (tp == NULL)
    errorjmp(mf,"Out of memory");

  memcpy(tp,&t,sizeof(*tp));
  memcpy(tp+1,cp,cplen);
  if (cplen) {
    tp->CodecPrivate = tp+1;
    tp->CodecPrivateSize = (unsigned)cplen;
  }

  cp = (char*)(tp+1) + cplen;
  CopyStr(&tp->Name,&cp);
  CopyStr(&tp->Language,&cp);
  CopyStr(&tp->CodecID,&cp);

  // set default language
  if (!tp->Language)
    tp->Language="eng";

  *tpp = tp;
}

static void parseTracks(MatroskaFile *mf,ulonglong toplen) {
  mf->seen.Tracks = 1;
  FOREACH(mf,toplen)
    case 0xae: // TrackEntry
      parseTrackEntry(mf,len);
      break;
  ENDFOR(mf);
}

static void addCue(MatroskaFile *mf,ulonglong pos,ulonglong timecode) {
  struct Cue  *cc = AGET(mf,Cues);
  cc->Time = timecode;
  cc->Position = pos;
  cc->Track = 0;
  cc->Block = 0;
}

static void parseCues(MatroskaFile *mf,ulonglong toplen) {
  ulonglong   v;
  struct Cue  cc;
  unsigned    i,j,k;

  mf->seen.Cues = 1;
  cc.Block = 0;

  FOREACH(mf,toplen)
    case 0xbb: // CuePoint
      FOREACH(mf,len)
	case 0xb3: // CueTime
	  cc.Time = readUInt(mf,(unsigned)len);
	  break;
	case 0xb7: // CueTrackPositions
	  FOREACH(mf,len)
	    case 0xf7: // CueTrack
	      v = readUInt(mf,(unsigned)len);
	      if (v>255)
		errorjmp(mf,"CueTrack points to an invalid track: %d",(int)v);
	      cc.Track = (unsigned char)v;
	      break;
	    case 0xf1: // CueClusterPosition
	      cc.Position = readUInt(mf,(unsigned)len);
	      break;
	    case 0x5378: // CueBlockNumber
	      cc.Block = readUInt(mf,(unsigned)len);
	      break;
	    case 0xea: // CodecState
	      readUInt(mf,(unsigned)len);
	      break;
	    case 0xdb: // CueReference
	      FOREACH(mf,len)
		case 0x96: // CueRefTime
		  readUInt(mf,(unsigned)len);
		  break;
		case 0x97: // CueRefCluster
		  readUInt(mf,(unsigned)len);
		  break;
		case 0x535f: // CueRefNumber
		  readUInt(mf,(unsigned)len);
		  break;
		case 0xeb: // CueRefCodecState
		  readUInt(mf,(unsigned)len);
		  break;
	      ENDFOR(mf);
	      break;
	  ENDFOR(mf);
	  break;
      ENDFOR(mf);

      if (mf->nCues == 0)
	addCue(mf,mf->pCluster,mf->firstTimecode);

      memcpy(AGET(mf,Cues),&cc,sizeof(cc));
      break;
  ENDFOR(mf);

  ARELEASE(mf,Cues);

  // bubble sort the cues and fuck the losers that write unordered cues
  for (i = mf->nCues - 1, k = 1; i > 0 && k > 0; --i)
    for (j = k = 0; j < i; ++j)
      if (mf->Cues[j].Time > mf->Cues[j+1].Time) {
	struct Cue tmp = mf->Cues[j+1];
	mf->Cues[j+1] = mf->Cues[j];
	mf->Cues[j] = tmp;
	++k;
      }
}

static void parseAttachment(MatroskaFile *mf,ulonglong toplen) {
  struct Attachment a,*pa;

  memset(&a,0,sizeof(a));
  FOREACH(mf,toplen)
    case 0x467e: // Description
      STRGETA(mf,a.Description,len);
      break;
    case 0x466e: // Name
      STRGETA(mf,a.Name,len);
      break;
    case 0x4660: // MimeType
      STRGETA(mf,a.MimeType,len);
      break;
    case 0x46ae: // UID
      a.UID = readUInt(mf,(unsigned)len);
      break;
    case 0x465c: // Data
      a.Position = filepos(mf);
      a.Length = len;
      skipbytes(mf,len);
      break;
  ENDFOR(mf);

  if (!a.Position)
    return;

  pa = AGET(mf,Attachments);
  memcpy(pa,&a,sizeof(a));

  if (a.Description)
    pa->Description = strdup(a.Description);
  if (a.Name)
    pa->Name = strdup(a.Name);
  if (a.MimeType)
    pa->MimeType = strdup(a.MimeType);
}

static void parseAttachments(MatroskaFile *mf,ulonglong toplen) {
  mf->seen.Attachments = 1;

  FOREACH(mf,toplen)
    case 0x61a7: // AttachedFile
      parseAttachment(mf,len);
      break;
  ENDFOR(mf);
}

static void parseChapter(MatroskaFile *mf,ulonglong toplen,struct Chapter *parent) {
  struct ChapterDisplay	*disp;
  struct ChapterCommand	*cmd;
  struct Chapter	*ch = ASGET(mf,parent,Children);

  memset(ch,0,sizeof(*ch));

  ch->Enabled = 1;

  FOREACH(mf,toplen)
    case 0x73c4: // ChapterUID
      ch->UID = readUInt(mf,(unsigned)len);
      break;
    case 0x91: // ChapterTimeStart
      ch->Start = readUInt(mf,(unsigned)len);
      break;
    case 0x92: // ChapterTimeEnd
      ch->End = readUInt(mf,(unsigned)len);
      break;
    case 0x98: // ChapterFlagHidden
      ch->Hidden = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x4598: // ChapterFlagEnabled
      ch->Enabled = readUInt(mf,(unsigned)len)!=0;
      break;
    case 0x8f: // ChapterTrack
      FOREACH(mf,len)
	case 0x89: // ChapterTrackNumber
	  *(ulonglong*)(ASGET(mf,ch,Tracks)) = readUInt(mf,(unsigned)len);
	  break;
      ENDFOR(mf);
      break;
    case 0x80: // ChapterDisplay
      disp = NULL;

      FOREACH(mf,len)
	case 0x85: // ChapterString
	  if (disp==NULL) {
	    disp = ASGET(mf,ch,Display);
	    memset(disp, 0, sizeof(*disp));
	  }
	  if (disp->String)
	    skipbytes(mf,len); // Ignore duplicate string
	  else
	    STRGETM(mf,disp->String,len);
	  break;
	case 0x437c: // ChapterLanguage
	  if (disp==NULL) {
	    disp = ASGET(mf,ch,Display);
	    memset(disp, 0, sizeof(*disp));
	  }
	  if (disp->Language)
	    skipbytes(mf,len);
	  else
	    STRGETM(mf,disp->Language,len);
	  break;
	case 0x437e: // ChapterCountry
	  if (disp==NULL) {
	    disp = ASGET(mf,ch,Display);
	    memset(disp, 0, sizeof(*disp));
	  }
	  if (disp->Country)
	    skipbytes(mf,len);
	  else
	    STRGETM(mf,disp->Country,len);
	  break;
      ENDFOR(mf);

      if (disp && !disp->String) {
	free(disp->Language);
	free(disp->Country);
	--ch->nDisplay;
      }
      break;
    case 0x6944: // ChapterCommand
      cmd = NULL;

      FOREACH(mf,len)
	case 0x6922: // ChapterCommandTime
	  if (cmd == NULL) {
	    cmd = ASGET(mf,ch,Commands);
	    memset(cmd, 0, sizeof(*cmd));
	  }
	  cmd->Time = readUInt(mf,(unsigned)len);
	  break;
	case 0x6937: // ChapterCommandString
	  if (cmd == NULL) {
	    cmd = ASGET(mf,ch,Commands);
	    memset(cmd, 0, sizeof(*cmd));
	  }
	  if (cmd->String)
	    skipbytes(mf,len);
	  else
	    STRGETM(mf,cmd->String,len);
	  break;
      ENDFOR(mf);

      if (cmd && !cmd->String)
	--ch->nCommands;
      break;
    case 0xb6: // Nested ChapterAtom
      parseChapter(mf,len,ch);
      break;
  ENDFOR(mf);

  ARELEASE(ch,Tracks);
  ARELEASE(ch,Display);
  ARELEASE(ch,Children);
}

static void parseChapters(MatroskaFile *mf,ulonglong toplen) {
  struct Chapter  *ch;

  mf->seen.Chapters = 1;

  FOREACH(mf,toplen)
    case 0x45b9: // EditionEntry
	ch = AGET(mf,Chapters);
	memset(ch, 0, sizeof(*ch));
 	FOREACH(mf,len)
	  case 0x45bc: // EditionUID
	    ch->UID = readUInt(mf,(unsigned)len);
	    break;
	  case 0x45bd: // EditionFlagHidden
	    ch->Hidden = readUInt(mf,(unsigned)len)!=0;
	    break;
	  case 0x45db: // EditionFlagDefault
	    ch->Default = readUInt(mf,(unsigned)len)!=0;
	    break;
	  case 0x45dd: // EditionManaged
	    ch->Managed = readUInt(mf,(unsigned)len)!=0;
	    break;
	  case 0xb6: // ChapterAtom
	    parseChapter(mf,len,ch);
	    break;
	ENDFOR(mf);
      break;
  ENDFOR(mf);
}

static void parseTags(MatroskaFile *mf,ulonglong toplen) {
  struct Tag  *tag;
  struct Target *target;
  struct SimpleTag *st;

  mf->seen.Tags = 1;

  FOREACH(mf,toplen)
    case 0x7373: // Tag
      tag = AGET(mf,Tags);
      memset(tag,0,sizeof(*tag));

      FOREACH(mf,len)
	case 0x63c0: // Targets
	  FOREACH(mf,len)
	    case 0x63c5: // TrackUID
	      target = ASGET(mf,tag,Targets);
	      target->UID = readUInt(mf,(unsigned)len);
	      target->Type = TARGET_TRACK;
	      break;
	    case 0x63c4: // ChapterUID
	      target = ASGET(mf,tag,Targets);
	      target->UID = readUInt(mf,(unsigned)len);
	      target->Type = TARGET_CHAPTER;
	      break;
	    case 0x63c6: // AttachmentUID
	      target = ASGET(mf,tag,Targets);
	      target->UID = readUInt(mf,(unsigned)len);
	      target->Type = TARGET_ATTACHMENT;
	      break;
	    case 0x63c9: // EditionUID
	      target = ASGET(mf,tag,Targets);
	      target->UID = readUInt(mf,(unsigned)len);
	      target->Type = TARGET_EDITION;
	      break;
	  ENDFOR(mf);
	  break;
	case 0x67c8: // SimpleTag
	  st = ASGET(mf,tag,SimpleTags);
	  memset(st,0,sizeof(*st));

	  FOREACH(mf,len)
	    case 0x45a3: // TagName
	      if (st->Name)
		skipbytes(mf,len);
	      else
		STRGETM(mf,st->Name,len);
	      break;
	    case 0x4487: // TagString
	      if (st->Value)
		skipbytes(mf,len);
	      else
		STRGETM(mf,st->Value,len);
	      break;
	    case 0x447a: // TagLanguage
	      if (st->Language)
		skipbytes(mf,len);
	      else
		STRGETM(mf,st->Language,len);
	      break;
	    case 0x4484: // TagDefault
	      st->Default = readUInt(mf,(unsigned)len)!=0;
	      break;
	  ENDFOR(mf);

	  if (!st->Name || !st->Value) {
	    free(st->Name);
	    free(st->Value);
	    --tag->nSimpleTags;
	  }
	  break;
      ENDFOR(mf);
      break;
  ENDFOR(mf);
}

static void parseContainer(MatroskaFile *mf) {
  ulonglong len;
  int	    id = readID(mf);
  if (id==EOF)
    errorjmp(mf,"Unexpected EOF in parseContainer");

  len = readSize(mf);

  switch (id) {
    case 0x1549a966: // SegmentInfo
      parseSegmentInfo(mf,len);
      break;
    case 0x1f43b675: // Cluster
      parseFirstCluster(mf,len);
      break;
    case 0x1654ae6b: // Tracks
      parseTracks(mf,len);
      break;
    case 0x1c53bb6b: // Cues
      parseCues(mf,len);
      break;
    case 0x1941a469: // Attachments
      parseAttachments(mf,len);
      break;
    case 0x1043a770: // Chapters
      parseChapters(mf,len);
      break;
    case 0x1254c367: // Tags
      parseTags(mf,len);
      break;
  }
}

static void parseContainerPos(MatroskaFile *mf,ulonglong pos) {
  seek(mf,pos);
  parseContainer(mf);
}

static void parsePointers(MatroskaFile *mf) {
  if (mf->pSegmentInfo && !mf->seen.SegmentInfo)
    parseContainerPos(mf,mf->pSegmentInfo);
  if (mf->pCluster && !mf->seen.Cluster)
    parseContainerPos(mf,mf->pCluster);
  if (mf->pTracks && !mf->seen.Tracks)
    parseContainerPos(mf,mf->pTracks);
  if (mf->pCues && !mf->seen.Cues)
    parseContainerPos(mf,mf->pCues);
  if (mf->pAttachments && !mf->seen.Attachments)
    parseContainerPos(mf,mf->pAttachments);
  if (mf->pChapters && !mf->seen.Chapters)
    parseContainerPos(mf,mf->pChapters);
  if (mf->pTags && !mf->seen.Tags)
    parseContainerPos(mf,mf->pTags);
}

static void parseSegment(MatroskaFile *mf,ulonglong toplen) {
  ulonglong   nextpos;
  unsigned    nSeekHeads = 0, dontstop = 0;

  // we want to read data until we find a seekhead or a trackinfo
  FOREACH(mf,toplen)
    case 0x114d9b74: // SeekHead
      if (mf->flags & MKVF_AVOID_SEEKS) {
	skipbytes(mf,len);
	break;
      }

      nextpos = filepos(mf) + len;
      do {
	mf->pSeekHead = 0;
	parseSeekHead(mf,len);
	++nSeekHeads;
	if (mf->pSeekHead) { // this is possibly a chained SeekHead
	  seek(mf,mf->pSeekHead);
	  id = readID(mf);
	  if (id==EOF) // chained SeekHead points to EOF?
	    goto resume;
	  if (id != 0x114d9b74) // chained SeekHead doesnt point to a SeekHead?
	    goto resume;
	  len = readSize(mf);
	} else if (mf->pSegmentInfo && mf->pTracks && mf->pCues && mf->pCluster) { // we have pointers to all key elements
	  // XXX EVIL HACK
	  // Some software doesnt index tags via SeekHead, so we continue
	  // reading the segment after the second SeekHead
	  if (mf->pTags || nSeekHeads<2 || filepos(mf)>=start+toplen) {
	    parsePointers(mf);
	    return;
	  }
	  // reset nextpos pointer to current position
	  nextpos = filepos(mf);
	  dontstop = 1;
	}
      } while (mf->pSeekHead);
resume:
      // XXX workaround for AVI-Mux GUI
      if (mf->pSegmentInfo && !mf->seen.SegmentInfo)
	parseContainerPos(mf,mf->pSegmentInfo);
      seek(mf,nextpos); // resume reading segment
      break;
    case 0x1549a966: // SegmentInfo
      mf->pSegmentInfo = cur;
      parseSegmentInfo(mf,len);
      break;
    case 0x1f43b675: // Cluster
      if (!mf->pCluster)
	mf->pCluster = cur;
      if (mf->seen.Cluster)
	skipbytes(mf,len);
      else
	parseFirstCluster(mf,len);
      break;
    case 0x1654ae6b: // Tracks
      mf->pTracks = cur;
      parseTracks(mf,len);
      break;
    case 0x1c53bb6b: // Cues
      mf->pCues = cur;
      parseCues(mf,len);
      break;
    case 0x1941a469: // Attachments
      mf->pAttachments = cur;
      parseAttachments(mf,len);
      break;
    case 0x1043a770: // Chapters
      mf->pChapters = cur;
      parseChapters(mf,len);
      break;
    case 0x1254c367: // Tags
      mf->pTags = cur;
      parseTags(mf,len);
      break;
  ENDFOR1(mf);
    // if we have pointers to all key elements
    if (!dontstop && mf->pSegmentInfo && mf->pTracks && mf->pCluster)
      break;
  ENDFOR2();
  parsePointers(mf);
}

static void parseBlockGroup(MatroskaFile *mf,ulonglong toplen,ulonglong timecode) {
  ulonglong	v;
  ulonglong	duration = 0;
  ulonglong	dpos;
  struct QueueEntry *qe,*qf = NULL;
  unsigned char	have_duration = 0, have_block = 0;
  unsigned char	gap = 0;
  unsigned char	lacing = 0;
  unsigned char	ref = 0;
  unsigned char	trackid;
  unsigned	tracknum = 0;
  int		c;
  unsigned	nframes = 0,i;
  unsigned	*sizes;
  signed short	block_timecode;

  FOREACH(mf,toplen)
    case 0xfb: // ReferenceBlock
      readSInt(mf,(unsigned)len);
      ref = 1;
      break;
    case 0xa1: // Block
      have_block = 1;

      dpos = filepos(mf);

      v = readVLUInt(mf);
      if (v>255)
	errorjmp(mf,"Invalid track number in Block: %d",(int)v);
      trackid = (unsigned char)v;

      for (tracknum=0;tracknum<mf->nTracks;++tracknum)
	if (mf->Tracks[tracknum]->Number == trackid)
	  goto found;
      errorjmp(mf,"Invalid track number in Block: %u",trackid);
found:

      if (mf->trackMask & (1<<tracknum)) { // ignore this block
	skipbytes(mf,start + tmplen - filepos(mf)); // shortcut
	return;
      }

      block_timecode = (signed short)readSInt(mf,2);

      // recalculate this block's timecode to final timecode in ns
      timecode = mul3(mf->Tracks[tracknum]->TimecodeScale,
	(timecode - mf->firstTimecode + block_timecode) * mf->Seg.TimecodeScale);

      c = readch(mf);
      if (c==EOF)
	errorjmp(mf,"Unexpected EOF while reading Block flags");

      gap = c & 0x1;
      lacing = (c >> 1) & 3;

      if (lacing) {
	c = readch(mf);
	if (c == EOF)
	  errorjmp(mf,"Unexpected EOF while reading lacing data");
	nframes = c+1;
      } else
	nframes = 1;
      sizes = alloca(nframes*sizeof(*sizes));
 
      switch (lacing) {
	case 0: // No lacing
	  sizes[0] = (unsigned)(len - filepos(mf) + dpos);
	  break;
	case 1: // Xiph lacing
	  sizes[nframes-1] = 0;
	  for (i=0;i<nframes-1;++i) {
	    sizes[i] = 0;
	    do {
	      c = readch(mf);
	      if (c==EOF)
		errorjmp(mf,"Unexpected EOF while reading lacing data");
	      sizes[i] += c;
	    } while (c==255);
	    sizes[nframes-1] += sizes[i];
	  }
	  sizes[nframes-1] = (unsigned)(len - filepos(mf) + dpos) - sizes[nframes-1];
	  break;
	case 3: // EBML lacing
	  sizes[nframes-1] = 0;
	  sizes[0] = (unsigned)readVLUInt(mf);
	  for (i=1;i<nframes-1;++i) {
	    sizes[i] = sizes[i-1] + (int)readVLSInt(mf);
	    sizes[nframes-1] += sizes[i];
	  }
	  if (nframes>1)
	    sizes[nframes-1] = (unsigned)(len - filepos(mf) + dpos) - sizes[0] - sizes[nframes-1];
	  break;
	case 2: // Fixed lacing
	  sizes[0] = (unsigned)(len - filepos(mf) + dpos)/nframes;
	  for (i=1;i<nframes;++i)
	    sizes[i] = sizes[0];
	  break;
      }

      v = filepos(mf);
      qf = NULL;
      for (i=0;i<nframes;++i) {
	qe = QAlloc(mf);
	if (!qf)
	  qf = qe;

	qe->Start = timecode;
	qe->End = timecode;
	qe->Position = v;
	qe->Length = sizes[i];
	qe->flags = FRAME_UNKNOWN_END | FRAME_KF;
	if (i == nframes-1 && gap)
	  qe->flags |= FRAME_GAP;
	if (i > 0)
	  qe->flags |= FRAME_UNKNOWN_START;

	QPut(&mf->Queues[tracknum],qe);

	v += sizes[i];
      }

      // we want to still load these bytes into cache
      for (v = filepos(mf) & ~0x3fff; v < len + dpos; v += 0x4000)
	mf->cache->read(mf->cache,v,NULL,0); // touch page

      skipbytes(mf,len - filepos(mf) + dpos);
      break;
    case 0x9b: // BlockDuration
      duration = readUInt(mf,(unsigned)len);
      have_duration = 1;
      break;
  ENDFOR(mf);

  if (!have_block)
    errorjmp(mf,"Found a BlockGroup without Block");

  if (nframes > 1) {
    if (have_duration) {
      duration = mul3(mf->Tracks[tracknum]->TimecodeScale,
	duration * mf->Seg.TimecodeScale);
      dpos = duration / nframes;

      v = qf->Start;
      for (qe = qf; nframes > 1; --nframes, qe = qe->next) {
	qe->Start = v;
	v += dpos;
	duration -= dpos;
	qe->End = v;
	qe->flags &= ~(FRAME_UNKNOWN_START|FRAME_UNKNOWN_END);
      }
      qe->Start = v;
      qe->End = v + duration;
      qe->flags &= ~(FRAME_UNKNOWN_START|FRAME_UNKNOWN_END);
    } else if (mf->Tracks[tracknum]->DefaultDuration) {
      dpos = mf->Tracks[tracknum]->DefaultDuration;
      v = qf->Start;
      for (qe = qf; nframes > 0; --nframes, qe = qe->next) {
	qe->Start = v;
	v += dpos;
	qe->End = v;
	qe->flags &= ~(FRAME_UNKNOWN_START|FRAME_UNKNOWN_END);
      }
    }
  } else if (nframes == 1) {
    if (have_duration) {
      qf->End = qf->Start + mul3(mf->Tracks[tracknum]->TimecodeScale,
	duration * mf->Seg.TimecodeScale);
      qf->flags &= ~FRAME_UNKNOWN_END;
    } else if (mf->Tracks[tracknum]->DefaultDuration) {
      qf->End = qf->Start + mf->Tracks[tracknum]->DefaultDuration;
      qf->flags &= ~FRAME_UNKNOWN_END;
    }
  }

  if (ref)
    while (qf) {
      qf->flags &= ~FRAME_KF;
      qf = qf->next;
    }
}

static void ClearQueue(MatroskaFile *mf,struct Queue *q) {
  struct QueueEntry *qe,*qn;

  for (qe=q->head;qe;qe=qn) {
    qn = qe->next;
    qe->next = mf->QFreeList;
    mf->QFreeList = qe;
  }

  q->head = NULL;
  q->tail = NULL;
}

static void EmptyQueues(MatroskaFile *mf) {
  unsigned	    i;

  for (i=0;i<mf->nTracks;++i)
    ClearQueue(mf,&mf->Queues[i]);
}

static int  readMoreBlocks(MatroskaFile *mf) {
  ulonglong		toplen, cstop;
  longlong		cp;
  int			cid, ret = 0;
  jmp_buf		jb;
  volatile unsigned	retries = 0;

  if (mf->readPosition >= mf->pSegmentTop)
    return EOF;

  memcpy(&jb,&mf->jb,sizeof(jb));

  if (setjmp(mf->jb)) { // something evil happened here, try to resync
    ret = EOF;

    if (++retries > 3) // don't try too hard
      goto ex;

    for (;;cp) {
      if (filepos(mf) >= mf->pSegmentTop) {
	mf->readPosition = filepos(mf);
	goto ex;
      }

      cp = mf->cache->scan(mf->cache,filepos(mf),0x1f43b675); // cluster

      if (cp < 0 || (ulonglong)cp >= mf->pSegmentTop)
	goto ex;

      seek(mf,cp);

      cid = readID(mf);
      if (cid == EOF)
	goto ex;
      if (cid == 0x1f43b675) {
	toplen = readSize(mf);
	if (toplen < MAXCLUSTER) {
	  // reset error flags
	  mf->flags &= ~MPF_ERROR;
	  ret = RBRESYNC;
	  break;
	}
      }
    }

    mf->readPosition = cp;
  }

  cstop = mf->cache->getsize(mf->cache)>>1;
  if (cstop > MAX_READAHEAD)
    cstop = MAX_READAHEAD;
  cstop += mf->readPosition;

  seek(mf,mf->readPosition);

  for (;;) {
    cid = readID(mf);
    if (cid == EOF) {
      ret = EOF;
      break;
    }
    toplen = readSize(mf);

    if (cid == 0x1f43b675) { // Cluster
      unsigned char	have_timecode = 0;

      FOREACH(mf,toplen)
	case 0xe7: // Timecode
	  mf->tcCluster = readUInt(mf,(unsigned)len);
	  have_timecode = 1;
	  break;
	case 0xa7: // Position
	  readUInt(mf,(unsigned)len);
	  break;
	case 0xab: // PrevSize
	  readUInt(mf,(unsigned)len);
	  break;
	case 0xa0: // BlockGroup
	  if (!have_timecode)
	    errorjmp(mf,"Found BlockGroup before cluster TimeCode");
	  parseBlockGroup(mf,len,mf->tcCluster);
	  goto out;
      ENDFOR(mf);
out:;
    } else {
      if (toplen > MAXFRAME)
	errorjmp(mf,"Element in a cluster is too large %X [%u]",cid,(unsigned)toplen);
      if (cid == 0xa0) // BlockGroup
	parseBlockGroup(mf,toplen,mf->tcCluster);
      else
	skipbytes(mf,toplen);
    }

    if ((mf->readPosition = filepos(mf)) > cstop)
      break;
  }

  mf->readPosition = filepos(mf);

ex:
  memcpy(&mf->jb,&jb,sizeof(jb));

  return ret;
}

// this is almost the same as readMoreBlocks, except it ensures
// there are no partial frames queued, however empty queues are ok
static int  fillQueues(MatroskaFile *mf,unsigned int mask) {
  unsigned    i,j;
  int	      ret = 0;

  for (;;) {
    j = 0;

    for (i=0;i<mf->nTracks;++i)
      if (mf->Queues[i].head && !(mask & (1<<i)))
	++j;

    if (j>0) // have at least some frames
      return ret;

    if ((ret = readMoreBlocks(mf)) < 0) {
      j = 0;
      for (i=0;i<mf->nTracks;++i)
	if (mf->Queues[i].head && !(mask & (1<<i)))
	  ++j;
      if (j) // we adjusted some blocks
	return 0;
      return EOF;
    }
  }
}

static void parseFile(MatroskaFile *mf) {
  ulonglong len;
  unsigned  i;
  int	    id = readID(mf);
  int	    m;

  if (id==EOF)
    errorjmp(mf,"Unexpected EOF at start of file");

  if (id!=0x1a45dfa3)
    errorjmp(mf,"First element in file is not EBML");

  parseEBML(mf,readSize(mf));

  // next we need to find the first segment
  for (;;) {
    id = readID(mf);
    if (id==EOF)
      errorjmp(mf,"No segments found in the file");
    len = readVLUIntImp(mf,&m);
    // see if it's unspecified
    if (len == (MAXU64 >> (57-m*7)))
      len = MAXU64;
    if (id == 0x18538067) // Segment
      break;
    skipbytes(mf,len);
  }

  // found it
  mf->pSegment = filepos(mf);
  mf->pSegmentTop = len == MAXU64 ? MAXU64 : mf->pSegment + len;
  parseSegment(mf,len);

  // check if we got all data
  if (!mf->seen.SegmentInfo)
    errorjmp(mf,"Couldn't find SegmentInfo");
  if (!mf->seen.Tracks)
    errorjmp(mf,"Couldn't find Tracks");
  if (!mf->seen.Cluster)
    errorjmp(mf,"Couldn't find any Clusters");

  // ensure we have cues
  if (mf->nCues == 0)
    addCue(mf,mf->pCluster,mf->firstTimecode);

  // release extra memory
  ARELEASE(mf,Tracks);

  // adjust cues
  for (i=0;i<mf->nCues;++i)
    mf->Cues[i].Time *= mf->Seg.TimecodeScale;

  // initialize reader
  mf->readPosition = mf->pCluster;
  mf->Queues = malloc(mf->nTracks * sizeof(*mf->Queues));
  if (mf->Queues == NULL)
    errorjmp(mf, "Ouf of memory");
  memset(mf->Queues, 0, mf->nTracks * sizeof(*mf->Queues));
}

static void DeleteChapter(struct Chapter *ch) {
  unsigned i;

  for (i=0;i<ch->nDisplay;++i) {
    free(ch->Display[i].String);
    free(ch->Display[i].Language);
    free(ch->Display[i].Country);
  }
  free(ch->Display);
  free(ch->Tracks);

  for (i=0;i<ch->nChildren;++i)
    DeleteChapter(&ch->Children[i]);
  free(ch->Children);
}

///////////////////////////////////////////////////////////////////////////
// public interface
MatroskaFile  *mkv_OpenEx(FileCache *io,
			  unsigned flags,
			  char *err_msg,unsigned msgsize)
{
  MatroskaFile	*mf = malloc(sizeof(*mf));
  if (mf == NULL) {
    io->close(io);
    strlcpy(err_msg,"Out of memory",msgsize);
    return NULL;
  }

  memset(mf,0,sizeof(*mf));

  mf->cache = io;
  mf->flags = flags;

  if (setjmp(mf->jb)==0)
    parseFile(mf);
  else { // parser error
    strlcpy(err_msg,mf->errmsg,msgsize);
    mkv_Close(mf);
    return NULL;
  }

  return mf;
}

MatroskaFile  *mkv_Open(FileCache *io,
			char *err_msg,unsigned msgsize)
{
  return mkv_OpenEx(io,0,err_msg,msgsize);
}

MatroskaFile  *mkv_OpenFile(const char *filename,
			    unsigned cache_size,
			    unsigned flags,
			    char *err_msg,
			    unsigned msgsize)
{
  InputFile *ii;
  FileCache *cc;
  unsigned  pages,psize;

  if (cache_size > 0) {
    psize = DEFAULT_PAGE_SIZE;
    pages = cache_size / DEFAULT_PAGE_SIZE;
    if (pages < MIN_PAGES)
      pages = MIN_PAGES;
  } else {
    psize = MIN_PAGE_SIZE;
    pages = MIN_PAGES;
  }

  if ((ii = fileio_open(filename,err_msg,msgsize)) == NULL)
    return NULL;

  if ((cc = CacheAlloc(ii,pages,psize)) == NULL) {
    ii->close(ii);
    strlcpy(err_msg,"Can't allocate file cache",msgsize);
    return NULL;
  }

  return mkv_OpenEx(cc,flags,err_msg,msgsize);
}

void	      mkv_Close(MatroskaFile *mf) {
  unsigned  i,j;

  if (mf==NULL)
    return;

  mf->cache->close(mf->cache);

  for (i=0;i<mf->nTracks;++i)
    free(mf->Tracks[i]);
  free(mf->Tracks);

  for (i=0;i<mf->nQBlocks;++i)
    free(mf->QBlocks[i]);
  free(mf->QBlocks);

  free(mf->Queues);

  free(mf->Seg.Title);
  free(mf->Seg.MuxingApp);
  free(mf->Seg.WritingApp);
  free(mf->Seg.Filename);
  free(mf->Seg.NextFilename);
  free(mf->Seg.PrevFilename);

  free(mf->Cues);

  for (i=0;i<mf->nAttachments;++i) {
    free(mf->Attachments[i].Description);
    free(mf->Attachments[i].Name);
    free(mf->Attachments[i].MimeType);
  }
  free(mf->Attachments);

  for (i=0;i<mf->nChapters;++i)
    DeleteChapter(&mf->Chapters[i]);
  free(mf->Chapters);

  for (i=0;i<mf->nTags;++i) {
    for (j=0;j<mf->Tags[i].nSimpleTags;++j) {
      free(mf->Tags[i].SimpleTags[j].Name);
      free(mf->Tags[i].SimpleTags[j].Value);
    }
    free(mf->Tags[i].Targets);
    free(mf->Tags[i].SimpleTags);
  }
  free(mf->Tags);

  free(mf);
}

const char    *mkv_GetLastError(MatroskaFile *mf) {
  return mf->errmsg[0] ? mf->errmsg : NULL;
}

SegmentInfo   *mkv_GetFileInfo(MatroskaFile *mf) {
  return &mf->Seg;
}

unsigned int  mkv_GetNumTracks(MatroskaFile *mf) {
  return mf->nTracks;
}

TrackInfo     *mkv_GetTrackInfo(MatroskaFile *mf,unsigned track) {
  if (track>mf->nTracks)
    return NULL;

  return mf->Tracks[track];
}

void	      mkv_GetAttachments(MatroskaFile *mf,Attachment **at,unsigned *count) {
  *at = mf->Attachments;
  *count = mf->nAttachments;
}

void	      mkv_GetChapters(MatroskaFile *mf,Chapter **ch,unsigned *count) {
  *ch = mf->Chapters;
  *count = mf->nChapters;
}

void	      mkv_GetTags(MatroskaFile *mf,Tag **tag,unsigned *count) {
  *tag = mf->Tags;
  *count = mf->nTags;
}

#define	IS_DELTA(f) (!((f)->flags & FRAME_KF) || ((f)->flags & FRAME_UNKNOWN_START))

void  mkv_Seek(MatroskaFile *mf,ulonglong timecode,unsigned flags) {
  int		i,j,m,ret;
  unsigned	n,z,mask;
  ulonglong	m_kftime[MAX_TRACKS];
  unsigned char	m_seendf[MAX_TRACKS];

  if ((mf->flags & MKVF_AVOID_SEEKS) || mf->nCues==0)
    return;

  mf->flags &= ~MPF_ERROR;

  i = 0;
  j = mf->nCues - 1;

  for (;;) {
    if (i>j) {
      j = j>=0 ? j : 0;

      if (setjmp(mf->jb)!=0)
	return;

      mkv_SetTrackMask(mf,mf->trackMask);

      if (flags & MKVF_SEEK_TO_PREV_KEYFRAME) {
	// we do this in two stages
	// a. find the last keyframes before the require position
	// b. seek to them

	// pass 1
	for (;;) {
	  for (n=0;n<mf->nTracks;++n) {
	    m_kftime[n] = MAXU64;
	    m_seendf[n] = 0;
	  }

	  EmptyQueues(mf);

	  mf->readPosition = mf->Cues[j].Position + mf->pSegment;

	  for (;;) {
	    if ((ret = fillQueues(mf,0)) < 0 || ret == RBRESYNC)
	      return;

	    // drain queues until we get to the required timecode
	    for (n=0;n<mf->nTracks;++n) {
	      if (mf->Queues[n].head && mf->Queues[n].head->Start<timecode) {
		if (IS_DELTA(mf->Queues[n].head))
		  m_seendf[n] = 1;
		else
		  m_kftime[n] = mf->Queues[n].head->Start;
	      }

	      while (mf->Queues[n].head && mf->Queues[n].head->Start<timecode)
	      {
		if (IS_DELTA(mf->Queues[n].head))
		  m_seendf[n] = 1;
		else
		  m_kftime[n] = mf->Queues[n].head->Start;
		QFree(mf,QGet(&mf->Queues[n]));
	      }

	      if (mf->Queues[n].head)
		if (!IS_DELTA(mf->Queues[n].head))
		  m_kftime[n] = mf->Queues[n].head->Start;
	    }

	    for (n=0;n<mf->nTracks;++n)
	      if (mf->Queues[n].head && mf->Queues[n].head->Start>=timecode)
		goto found;
	  }
found:
  
	  for (n=0;n<mf->nTracks;++n)
	    if (!(mf->trackMask & (1<<n)) && m_kftime[n]==MAXU64 &&
		m_seendf[n] && j>0)
	    {
	      // we need to restart the search from prev cue
	      --j;
	      goto again;
	    }

	  break;
again:;
	}
      } else
	for (n=0;n<mf->nTracks;++n)
	  m_kftime[n] = timecode;

      // now seek to this timecode
      EmptyQueues(mf);

      mf->readPosition = mf->Cues[j].Position + mf->pSegment;

      for (mask=0;;) {
	if ((ret = fillQueues(mf,mask)) < 0 || ret == RBRESYNC)
	  return;

	// drain queues until we get to the required timecode
	for (n=0;n<mf->nTracks;++n) {
	  struct QueueEntry *qe;
	  for (qe = mf->Queues[n].head;qe && qe->Start<m_kftime[n];qe = mf->Queues[n].head)
	    QFree(mf,QGet(&mf->Queues[n]));
	}

	for (n=z=0;n<mf->nTracks;++n)
	  if (m_kftime[n]==MAXU64 || (mf->Queues[n].head && mf->Queues[n].head->Start>=m_kftime[n])) {
	    ++z;
	    mask |= 1<<n;
	  }

	if (z==mf->nTracks)
	  return;
      }
    }

    m = (i+j)>>1;

    if (timecode < mf->Cues[m].Time)
      j = m-1;
    else
      i = m+1;
  }
}

void  mkv_SkipToKeyframe(MatroskaFile *mf) {
  unsigned  n,wait;
  ulonglong ht;

  if (setjmp(mf->jb)!=0)
    return;

  // remove delta frames from queues
  do {
    wait = 0;

    if (fillQueues(mf,0)<0)
      return;

    for (n=0;n<mf->nTracks;++n)
      if (mf->Queues[n].head && !(mf->Queues[n].head->flags & FRAME_KF)) {
	++wait;
	QFree(mf,QGet(&mf->Queues[n]));
      }
  } while (wait);

  // find highest queued time
  for (n=0,ht=0;n<mf->nTracks;++n)
    if (mf->Queues[n].head && ht<mf->Queues[n].head->Start)
      ht = mf->Queues[n].head->Start;

  // ensure the time difference is less than 100ms
  do {
    wait = 0;

    if (fillQueues(mf,0)<0)
      return;

    for (n=0;n<mf->nTracks;++n)
      while (mf->Queues[n].head && mf->Queues[n].head->next &&
	  (mf->Queues[n].head->next->flags & FRAME_KF) &&
	  ht - mf->Queues[n].head->Start > 100000000)
      {
	++wait;
	QFree(mf,QGet(&mf->Queues[n]));
      }

  } while (wait);
}

ulonglong mkv_GetLowestQTimecode(MatroskaFile *mf) {
  unsigned  n,seen;
  ulonglong t;

  // find the lowest queued timecode
  for (n=seen=0,t=0;n<mf->nTracks;++n)
    if (mf->Queues[n].head && (!seen || t > mf->Queues[n].head->Start))
      t = mf->Queues[n].head->Start, seen=1;

  return seen ? t : -1;
}

int	      mkv_TruncFloat(MKFLOAT f) {
#ifdef MATROSKA_INTEGER_ONLY
  return (int)(f.v >> 32);
#else
  return (int)f;
#endif
}

#define	FTRACK	0xffffffff

void	      mkv_SetTrackMask(MatroskaFile *mf,unsigned int mask) {
  unsigned int	  i;

  if (mf->flags & MPF_ERROR)
    return;

  mf->trackMask = mask;

  for (i=0;i<mf->nTracks;++i)
    if (mask & (1<<i))
      ClearQueue(mf,&mf->Queues[i]);
}

int	      mkv_ReadFrame(MatroskaFile *mf,
			    unsigned int mask,unsigned int *track,
			    ulonglong *StartTime,ulonglong *EndTime,
			    ulonglong *FilePos,unsigned int *FrameSize,
			    unsigned int *FrameFlags)
{
  unsigned int	    i,j;
  struct QueueEntry *qe;

  if (mf->flags & MPF_ERROR)
    return -1;

  if (setjmp(mf->jb)!=0)
    return -1;

  do {
    // extract required frame, use block with the lowest timecode
    for (j=FTRACK,i=0;i<mf->nTracks;++i)
      if (!(mask & (1<<i)) && mf->Queues[i].head) {
	j = i;
	++i;
	break;
      }

    for (;i<mf->nTracks;++i)
      if (!(mask & (1<<i)) && mf->Queues[i].head &&
	  mf->Queues[j].head->Start > mf->Queues[i].head->Start)
	j = i;

    if (j != FTRACK) {
      qe = QGet(&mf->Queues[j]);

      *track = j;
      *StartTime = qe->Start;
      *EndTime = qe->End;
      *FilePos = qe->Position;
      *FrameSize = qe->Length;
      *FrameFlags = qe->flags;

      QFree(mf,qe);

      return 0;
    }
  } while (fillQueues(mf,mask)>=0);

  return EOF;
}

int	      mkv_ReadData(MatroskaFile *mf,ulonglong FilePos,
			   void *Buffer,unsigned int Count)
{
  int	rd;

  if (mf->flags & MPF_ERROR)
    return -1;

  rd = mf->cache->read(mf->cache,FilePos,Buffer,Count);
  if (rd<0) {
    const char	*ce = mf->cache->geterror(mf->cache);
    vsnprintf(mf->errmsg,sizeof(mf->errmsg),"I/O Error: %s",(va_list)&ce);
    return -1;
  }
  return rd;
}

#ifdef MATROSKA_COMPRESSION_SUPPORT
/*************************************************************************
 * Compressed streams support
 ************************************************************************/
struct CompressedStream {
  MatroskaFile	  *mf;
  z_stream	  zs;

  /* current compressed frame */
  ulonglong	  frame_pos;
  unsigned	  frame_size;
  char		  frame_buffer[2048];

  /* decoded data buffer */
  char		  decoded_buffer[2048];
  unsigned	  decoded_ptr;
  unsigned	  decoded_size;

  /* error handling */
  char		  errmsg[128];
};

CompressedStream  *cs_Create(/* in */	MatroskaFile *mf,
			     /* in */	unsigned tracknum,
			     /* out */	char *errormsg,
			     /* in */	unsigned msgsize)
{
  CompressedStream  *cs;
  TrackInfo	    *ti;
  int		    code;

  ti = mkv_GetTrackInfo(mf, tracknum);
  if (ti == NULL) {
    strlcpy(errormsg, "No such track.", msgsize);
    return NULL;
  }

  if (!ti->CompEnabled) {
    strlcpy(errormsg, "Track is not compressed.", msgsize);
    return NULL;
  }

  if (ti->CompMethod != COMP_ZLIB) {
    strlcpy(errormsg, "Unsupported compression method.", msgsize);
    return NULL;
  }

  cs = malloc(sizeof(*cs));
  if (cs == NULL) {
    strlcpy(errormsg, "Ouf of memory.", msgsize);
    return NULL;
  }

  memset(&cs->zs,0,sizeof(cs->zs));
  code = inflateInit(&cs->zs);
  if (code != Z_OK) {
    strlcpy(errormsg, "ZLib error.", msgsize);
    free(cs);
    return NULL;
  }

  cs->frame_size = 0;
  cs->decoded_ptr = cs->decoded_size = 0;
  cs->mf = mf;

  return cs;
}

void		  cs_Destroy(/* in */ CompressedStream *cs) {
  if (cs == NULL)
    return;

  inflateEnd(&cs->zs);
  free(cs);
}

/* advance to the next frame in matroska stream, you need to pass values returned
 * by mkv_ReadFrame */
void		  cs_NextFrame(/* in */ CompressedStream *cs,
			       /* in */ ulonglong pos,
			       /* in */ unsigned size)
{
  cs->zs.avail_in = 0;
  inflateReset(&cs->zs);
  cs->frame_pos = pos;
  cs->frame_size = size;
  cs->decoded_ptr = cs->decoded_size = 0;
}

/* read and decode more data from current frame, return number of bytes decoded,
 * 0 on end of frame, or -1 on error */
int		  cs_ReadData(CompressedStream *cs,char *buffer,unsigned bufsize)
{
  char	    *cp = buffer;
  unsigned  rd = 0;
  unsigned  todo;
  int	    code;
  
  do {
    /* try to copy data from decoded buffer */
    if (cs->decoded_ptr < cs->decoded_size) {
      todo = cs->decoded_size - cs->decoded_ptr;;
      if (todo > bufsize - rd)
	todo = bufsize - rd;

      memcpy(cp, cs->decoded_buffer + cs->decoded_ptr, todo);

      rd += todo;
      cp += todo;
      cs->decoded_ptr += todo;
    } else {
      /* setup output buffer */
      cs->zs.next_out = cs->decoded_buffer;
      cs->zs.avail_out = sizeof(cs->decoded_buffer);

      /* try to read more data */
      if (cs->zs.avail_in == 0 && cs->frame_size > 0) {
	todo = cs->frame_size;
	if (todo > sizeof(cs->frame_buffer))
	  todo = sizeof(cs->frame_buffer);

	if (mkv_ReadData(cs->mf, cs->frame_pos, cs->frame_buffer, todo)<0) {
	  strlcpy(cs->errmsg, mkv_GetLastError(cs->mf), sizeof(cs->errmsg));
	  return -1;
	}

	cs->zs.next_in = cs->frame_buffer;
	cs->zs.avail_in = todo;

	cs->frame_pos += todo;
	cs->frame_size -= todo;
      }

      /* try to decode more data */
      code = inflate(&cs->zs,Z_NO_FLUSH);
      if (code != Z_OK && code != Z_STREAM_END) {
	strlcpy(cs->errmsg, "ZLib error.", sizeof(cs->errmsg));
	return -1;
      }

      /* handle decoded data */
      if (cs->zs.avail_out == sizeof(cs->decoded_buffer)) /* EOF */
	break;

      cs->decoded_ptr = 0;
      cs->decoded_size = sizeof(cs->decoded_buffer) - cs->zs.avail_out;
    }
  } while (rd < bufsize);

  return rd;
}

/* return error message for the last error */
const char	  *cs_GetLastError(CompressedStream *cs)
{
  if (!cs->errmsg[0])
    return NULL;
  return cs->errmsg;
}
#endif
