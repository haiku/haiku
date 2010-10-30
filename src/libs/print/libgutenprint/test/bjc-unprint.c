/* $Id: bjc-unprint.c,v 1.10 2004/09/17 18:38:27 rleigh Exp $ */
/*
 * Convert BJC-printjobs to xbm files, one for each color channel
 *
 * Copyright 2001 Andy Thaller <thaller@ph.tum.de>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/util.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *efnlc= 0,*efnlm= 0,*efnc= 0,*efnm= 0,*efny= 0,*efnk= 0;

int lc_cnt= 0,lm_cnt= 0,ly_cnt= 0,c_cnt= 0,m_cnt= 0,y_cnt= 0,k_cnt= 0;

FILE *outfilelc= 0;
FILE *outfilelm= 0;
FILE *outfilely= 0;
FILE *outfilec= 0;
FILE *outfilem= 0;
FILE *outfiley= 0;
FILE *outfilek= 0;

int xsize,ysize,pixcnt;

typedef struct scanline_t_ {
  int size;
  int osize;
  unsigned char *buf;
  int xmin;
  int xmax;
  int width;
  int y;
  struct scanline_t_ *next;
} scanline_t;

typedef struct bitimage_t_ {
  unsigned char *buf;
  int y0;
  int width;
  int height;
  int xmin;
  int xmax;
} bitimage_t;

bitimage_t *bitimage_new (void);
int last_bit (unsigned char i);
int first_bit (unsigned char i);
void rle_info (const unsigned char *inbuf, int n, int *first, int *last,
               int *width, int *length);
int rle_decode (unsigned char *inbuf, int n, unsigned char *outbuf, int max);
scanline_t* scanline_new (void);
scanline_t *scanline_store (scanline_t *line, int y, unsigned char *buf,
                            int size);
bitimage_t *scanlines2bitimage (scanline_t *slimg);
char conv (char i);
void save2xbm (const char *filename,char col, bitimage_t *img,
               int xmin, int ymin, int xmax, int ymax);
int nextcmd (FILE *infile, unsigned char *inbuff, int *cnt);
int process(FILE *infile, scanline_t *sf[7], int *xmin_, int *xmax_,
            int *ymin_, int *ymax_);


bitimage_t *bitimage_new(void)
{
  bitimage_t *tmp= (bitimage_t*) stp_malloc (sizeof(bitimage_t));
  tmp->buf= 0;
  tmp->y0= 0;
  tmp->width= 0;
  tmp->height= 0;
  tmp->xmin= 0;
  tmp->xmax= 0;
  return tmp;
}

int last_bit(unsigned char i) {
  if (i & 0x1) return 0;
  if (i & 0x2) return 1;
  if (i & 0x4) return 2;
  if (i & 0x8) return 3;
  if (i & 0x10) return 4;
  if (i & 0x20) return 5;
  if (i & 0x40) return 6;
  if (i & 0x80) return 7;
  return 0;
}

int first_bit(unsigned char i) {
  if (i & 0x80) return 7;
  if (i & 0x40) return 6;
  if (i & 0x20) return 5;
  if (i & 0x10) return 4;
  if (i & 0x8) return 3;
  if (i & 0x4) return 2;
  if (i & 0x2) return 1;
  if (i & 0x1) return 0;
  return 0;
}

void rle_info(const unsigned char *inbuf, int n, int *first, int *last, int *width, int *length)
{
  const char *ib= (const char *)inbuf;
  char cnt;
  int o= 0;
  int i=0,j,num,f=-1,l=0;

  *first= 0;
  *last= 0;
  *width= 0;

  if (n<=0) return;
  while (i<n) {
    cnt= ib[i];
    if (cnt<0) {
      num= 1-cnt;
      if (inbuf[i+1]) {
	if (f<0) f=o*8 + first_bit(inbuf[i+1]);
	l= o*8 + last_bit(inbuf[i+1]);
      }
      o+= num;
      i+= 2;
    } else {
      num= cnt+1;
      for (j=0; j<num; j++) {
	if (inbuf[i+j+1]) {
	  if (f<0) f=(o+j)*8 + first_bit(inbuf[i+j+1]);
	  l= o*8 + last_bit(inbuf[i+j+1]);
	}
      }
      o+= num;
      i+= num+1;
    }
  }

  if (first) *first= f;
  if (last) *last= l;
  if (length) *length= o;
  if (width) *width= (f<0) ? 0 : l-f+1;
}

int rle_decode(unsigned char *inbuf, int n, unsigned char *outbuf,int max)
{
  const char *ib= (const char *)inbuf;
  char cnt;
  int o= 0;
  int i=0,j,num;

  if (n<=0) return 0;
  while (i<n) {
    cnt= ib[i];
    if (cnt<0) {
      num= 1-cnt;
      for (j=0; j<num; j++) outbuf[o+j]=inbuf[i+1];
      o+= num;
      i+= 2;
    } else {
      num= cnt+1;
      for (j=0; j<num; j++) outbuf[o+j]=inbuf[i+j+1];
      o+= num;
      i+= num+1;
    }
  }

  return o;
}


scanline_t* scanline_new(void)
{
  scanline_t* tmp= (scanline_t*) stp_malloc (sizeof(scanline_t));
  tmp->size= 0;
  tmp->osize= 0;
  tmp->buf= 0;
  tmp->xmin= 0;
  tmp->xmax= 0;
  tmp->width= 0;
  tmp->y= 0;
  tmp->next= 0;
  return tmp;
}

scanline_t *scanline_store(scanline_t *line, int y, unsigned char *buf, int size) {
  if (!line && !(line= scanline_new()))
    return 0;
  line->size= size;
  line->buf= (unsigned char *) stp_malloc (size);
  memcpy(line->buf,buf,size);
  rle_info(buf,size,&line->xmin,&line->xmax,&line->width,&line->osize);
  /* fprintf(stderr,"%d %d %d %d  ",size,line->xmin,line->xmax,line->width); */
  line->y= y;
  return line;
}


bitimage_t *scanlines2bitimage(scanline_t *slimg)
{
  bitimage_t *img= 0;
  scanline_t *sl;
  int w= 0;
  int y0= -1;
  int y= 0;
  int h;

  for (sl=slimg; sl!=0; sl=sl->next) {
    if (sl->width) {
      if (w<sl->osize) w= sl->osize;
      if (y0<0) y0= sl->y;
      y= sl->y;
    }
  }
  h= y-y0+1;

  if ((!w) || (!h))
    return 0;

  img= bitimage_new();

  img->buf= (unsigned char*) stp_malloc(h*w);
  memset(img->buf,0,h*w);
  img->width= w;
  img->height= h;
  img->y0= y0;

  for (sl=slimg; sl!=0; sl=sl->next) {
    y= sl->y- y0;
    if ((y>=0) && (y<h)) {
      rle_decode(sl->buf,sl->size,img->buf+y*w,w);
    }
  }

  return img;
}

char conv(char i) {
  char o= 0;
  if (i & 0x80) o|=0x01;
  if (i & 0x40) o|=0x02;
  if (i & 0x20) o|=0x04;
  if (i & 0x10) o|=0x08;
  if (i & 0x08) o|=0x10;
  if (i & 0x04) o|=0x20;
  if (i & 0x02) o|=0x40;
  if (i & 0x01) o|=0x80;
  return o;
}


void save2xbm(const char *filename,char col, bitimage_t *img,
	      int xmin, int ymin, int xmax, int ymax)
{
  char *outfilename= (char*) stp_malloc(strlen(filename)+16);
  FILE *o;
  int i,j,k,i0,i1,j0,j1,w,h;

  if (!img) return;

  if (col)
    sprintf(outfilename,"%s_%c.xbm",filename,col);
  else
    sprintf(outfilename,"%s.xbm",filename);

  if (!(o= fopen(outfilename,"w"))) {
    stp_free(outfilename);
    return;
  }

  i0= (ymin>0) ? ymin-img->y0 : 0;
  i1= (ymax>0) ? ymax-img->y0 : img->height;
  j0= (xmin>0 && xmin<img->width*8) ? xmin/8 : 0;
  j1= (xmax>0 && xmax<img->width*8) ? xmax/8 : img->width-1;

  w= j1-j0+1;
  h= i1-i0;

  if (col)
    fprintf(o,"#define %s_%c_width %d\n#define %s_%c_height %d\nstatic char %s_%c_bits[] = {",
	    outfilename,col,w*8,
	    outfilename,col,h,
	    outfilename,col);
  else
    fprintf(o,"#define %s_width %d\n#define %s_height %d\nstatic char %s_bits[] = {",
	    outfilename,w*8,
	    outfilename,h,
	    outfilename);

  fprintf(stderr,"%d %d %d %d\n",i0,h,j0,w);
  for (i=i0,k=0; i<i1; i++) {
    for (j=j0; j<=j1; j++) {
      if (k%16 == 0) fprintf(o,"\n");
      fprintf(o,"0x%02x, ",(i>=0 && i<img->height)? conv(img->buf[i*img->width+j])&0xff : 0);
      k++;
    }
  }

  fprintf(o," 0x00 };\n");

  fclose(o);
}

int nextcmd(FILE *infile,unsigned char *inbuff,int *cnt)
{
  unsigned char c1=0,c2=0;
  int c;
  int cmd;

  while (!feof(infile) && c1!=0x1b && (c2!=0x28 && c2!=0x5b)) {
    c1=c2; c2=fgetc(infile);
  }
  if (feof(infile)) return 0;

  cmd=fgetc(infile);
  if (feof(infile)) return 0;

  c1= fgetc(infile);
  if (feof(infile)) return 0;
  c2= fgetc(infile);
  if (feof(infile)) return 0;

  *cnt= c1+256*c2;

  if ((c=fread(inbuff,1,*cnt,infile) != *cnt)) {
    fprintf(stderr,"read error for 0x%02x - not enough data (%d/%d)!\n",cmd,
	    c,*cnt);
    return 0;
  }

  return cmd;
}

int process(FILE *infile,scanline_t *sf[7],int *xmin_,int *xmax_,int *ymin_,int *ymax_)
{
  unsigned char inbuff[65540];
  scanline_t *sl[7], *nsl;

  int xres,yres;
  int xmin=1000000,xmax=0,ymin=-1,ymax=0,width=0;
  int col=0;
  int line=0;
  int i;
  int cnt;
  int cmd;

  for (i=0; i<7; i++) sf[i]= sl[i]= 0;

  if (!infile) return 0;
  fseek(infile,0,SEEK_SET);

  while ((cmd= nextcmd(infile,inbuff,&cnt))) {
    switch(cmd) {
    case 0x64:
      yres=inbuff[0]*256+inbuff[1];
      xres=inbuff[2]*256+inbuff[3];
      fprintf(stderr,"res=%dx%ddpi\n",xres,yres);
      break;
    case 0x65:
      line+=(inbuff[0]*256+inbuff[1]);
      break;
    case 0x41:
      switch(inbuff[0]) {
      case 0x4b: col= 0; break; /* black        */
      case 0x59: col= 1; break; /* yellow       */
      case 0x4d: col= 2; break; /* magenta      */
      case 0x43: col= 3; break; /* cyan         */
      case 0x79: col= 4; break; /* lightyellow  */
      case 0x6d: col= 5; break; /* lightmagenta */
      case 0x63: col= 6; break; /* lightcyan    */
      default:
	fprintf(stderr,"unkown color component 0x%02x\n",inbuff[0]);
	exit(-1);
      }

      nsl= scanline_store(0,line,inbuff+1,cnt-1);

      if (nsl) {
	if (nsl->width) {
	  if (ymin<0) ymin=line;
	  ymax=line;
	  if (nsl->xmin<xmin) xmin=nsl->xmin;
	  if (nsl->xmax>xmax) xmax=nsl->xmax;
	  if (nsl->width>width) width=nsl->width;
	  if (!sf[col])
	    sf[col]=nsl;
	  else
	    sl[col]->next= nsl;
	  sl[col]= nsl;
	} else {
	  stp_free (nsl);
	  nsl= 0;
	}

	if (fgetc(infile)!=0x0d) {
	  fprintf(stderr,"COLOR LINE NOT TERMINATED BY 0x0d @ %lx!!\n",
		  ftell(infile));
	}
      }
      break;
    case 0x62:
      break;
    default:
      ;
    }
  }

  fprintf(stderr,"size=%d..%d:%d..%d = %dx%d\n",xmin,xmax,ymin,ymax,width,ymax-ymin+1);

  *xmin_= xmin;
  *xmax_= xmax;
  *ymin_= ymin;
  *ymax_= ymax;

  return 1;
}

/*
const char *fbin(char b) {
  static char bf[9];

  bf[0]= (b & 0x80) ? '1' : '0';
  bf[1]= (b & 0x40) ? '1' : '0';
  bf[2]= (b & 0x20) ? '1' : '0';
  bf[3]= (b & 0x10) ? '1' : '0';
  bf[4]= (b & 0x08) ? '1' : '0';
  bf[5]= (b & 0x04) ? '1' : '0';
  bf[6]= (b & 0x02) ? '1' : '0';
  bf[7]= (b & 0x01) ? '1' : '0';
  bf[8]=0;
  return bf;
}
*/

int main(int argc, char **argv)
{
  char *infilename= 0, *outfilename=0;
  FILE *infile= 0;
  scanline_t *scanlines[7];
  char colname[7] = { 'K', 'Y', 'M', 'C', 'y', 'm', 'c' };
  int xmin,xmax;
  int ymin,ymax;

  if (argc>1) {
    if (argc>2)
      outfilename= argv[2];
    else
      outfilename= argv[1];

    infilename= argv[1];
    infile= fopen(infilename,"r");

    xsize=ysize=0;

    process(infile,scanlines,&xmin,&xmax,&ymin,&ymax);

    save2xbm(outfilename,colname[0],scanlines2bitimage(scanlines[0]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[1],scanlines2bitimage(scanlines[1]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[2],scanlines2bitimage(scanlines[2]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[3],scanlines2bitimage(scanlines[3]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[4],scanlines2bitimage(scanlines[4]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[5],scanlines2bitimage(scanlines[5]),xmin,ymin,xmax,ymax);
    save2xbm(outfilename,colname[6],scanlines2bitimage(scanlines[6]),xmin,ymin,xmax,ymax);

  } else {
    fprintf(stderr,"\nusage: bjc-unprint INFILE [OUTFILE]\n\n");
  }

  return 0;
}

