#include <stdlib.h>
#include <stdio.h>

#include "mpg123.h"
#include "mpglib.h"

void InitMpgLib(void)
{
	make_decode_tables(32767);
	init_layer2();
	init_layer3(SBLIMIT);
}

void InitMP3(struct mpstr *mp) 
{
	memset(mp,0,sizeof(struct mpstr));

	mp->framesize = 0;
	mp->fsizeold = -1;
	mp->bsize = 0;
	mp->head = mp->tail = NULL;
	mp->fr.single = -1;
	mp->bsnum = 0;
	mp->synth_bo = 1;
	mp->bitindex = 0;
	mp->wordpointer = 0;
}

void ExitMP3(struct mpstr *mp)
{
	struct buf *b,*bn;
	
	b = mp->tail;
	while(b) {
		free(b->pnt);
		bn = b->next;
		free(b);
		b = bn;
	}
}

static struct buf *addbuf(struct mpstr *mp,char *buf,int size)
{
	struct buf *nbuf;

	nbuf = malloc( sizeof(struct buf) );
	if(!nbuf) {
		fprintf(stderr,"Out of memory!\n");
		return NULL;
	}
	nbuf->pnt = malloc(size);
	if(!nbuf->pnt) {
		free(nbuf);
		return NULL;
	}
	nbuf->size = size;
	memcpy(nbuf->pnt,buf,size);
	nbuf->next = NULL;
	nbuf->prev = mp->head;
	nbuf->pos = 0;

	if(!mp->tail) {
		mp->tail = nbuf;
	}
	else {
	  mp->head->next = nbuf;
	}

	mp->head = nbuf;
	mp->bsize += size;

	return nbuf;
}

static void remove_buf(struct mpstr *mp)
{
  struct buf *buf = mp->tail;
  
  mp->tail = buf->next;
  if(mp->tail)
    mp->tail->prev = NULL;
  else {
    mp->tail = mp->head = NULL;
  }
  
  free(buf->pnt);
  free(buf);

}

static int read_buf_byte(struct mpstr *mp)
{
	unsigned int b;

	int pos;

	pos = mp->tail->pos;
	while(pos >= mp->tail->size) {
		remove_buf(mp);
		pos = mp->tail->pos;
		if(!mp->tail) {
			fprintf(stderr,"Fatal error!\n");
			exit(1);
		}
	}

	b = mp->tail->pnt[pos];
	mp->bsize--;
	mp->tail->pos++;
	

	return b;
}

static void read_head(struct mpstr *mp)
{
	unsigned long head;

	head = read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);

	mp->header = head;
}

int decodeMP3(struct mpstr *mp,char *in,int isize,char *out,
		int osize,int *done)
{
	int len;

	if(osize < 4608) {
		fprintf(stderr,"To less out space\n");
		return MP3_ERR;
	}

	if(in) {
		if(addbuf(mp,in,isize) == NULL) {
			return MP3_ERR;
		}
	}

	/* First decode header */
	if(mp->framesize == 0) {
		if(mp->bsize < 4) {
			return MP3_NEED_MORE;
		}
		read_head(mp);
		decode_header(&mp->fr,mp->header);
		mp->framesize = mp->fr.framesize;
	}

	if(mp->fr.framesize > mp->bsize)
		return MP3_NEED_MORE;

	mp->wordpointer = mp->bsspace[mp->bsnum] + 512;
	mp->bsnum = (mp->bsnum + 1) & 0x1;
	mp->bitindex = 0;

	len = 0;
	while(len < mp->framesize) {
		int nlen;
		int blen = mp->tail->size - mp->tail->pos;
		if( (mp->framesize - len) <= blen) {
                  nlen = mp->framesize-len;
		}
		else {
                  nlen = blen;
                }
		memcpy(mp->wordpointer+len,mp->tail->pnt+mp->tail->pos,nlen);
                len += nlen;
                mp->tail->pos += nlen;
		mp->bsize -= nlen;
                if(mp->tail->pos == mp->tail->size) {
                   remove_buf(mp);
                }
	}

	*done = 0;
	if(mp->fr.error_protection)
           getbits(mp, 16);
        switch(mp->fr.lay) {
          case 1:
	    do_layer1(mp, &mp->fr,(unsigned char *) out,done);
            break;
          case 2:
	    do_layer2(mp, &mp->fr,(unsigned char *) out,done);
            break;
          case 3:
	    do_layer3(mp, &mp->fr,(unsigned char *) out,done);
            break;
        }

	mp->fsizeold = mp->framesize;
	mp->framesize = 0;

	return MP3_OK;
}

int set_pointer(struct mpstr *mp, long backstep)
{
  unsigned char *bsbufold;
  if(mp->fsizeold < 0 && backstep > 0) {
    fprintf(stderr,"Can't step back %ld!\n",backstep);
    return MP3_ERR;
  }
  bsbufold = mp->bsspace[mp->bsnum] + 512;
  mp->wordpointer -= backstep;
  if (backstep)
    memcpy(mp->wordpointer,bsbufold+mp->fsizeold-backstep,backstep);
  mp->bitindex = 0;
  return MP3_OK;
}
