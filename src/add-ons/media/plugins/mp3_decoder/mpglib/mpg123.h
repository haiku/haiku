#ifndef _MPG123_H
#define _MPG123_H

#include        <stdio.h>
#include        <string.h>
#include        <signal.h>
#include        <unistd.h>
#include        <math.h>

#ifdef REAL_IS_FLOAT
#  define real float
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#else
#  define real double
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

/* AUDIOBUFSIZE = n*64 with n=1,2,3 ...  */
#define		AUDIOBUFSIZE		16384

#define         SBLIMIT                 32
#define         SSLIMIT                 18

#define         SCALE_BLOCK             12

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define MAXFRAMESIZE 1792

struct mpstr;

struct frame {
    int stereo;
    int jsbound;
    int single;
    int lsf;
    int mpeg25;
    int header_change;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int framesize; /* computed framesize */

    /* layer2 stuff */
    int II_sblimit;
    void *alloc;
};

extern unsigned int   get1bit(struct mpstr *mp);
extern unsigned int   getbits(struct mpstr *mp, int);
extern unsigned int   getbits_fast(struct mpstr *mp, int);
extern int            set_pointer(struct mpstr *mp, long backstep);

extern void make_decode_tables(long scaleval);
extern int do_layer3(struct mpstr *mp, struct frame *fr,unsigned char *,int *);
extern int do_layer2(struct mpstr *mp, struct frame *fr,unsigned char *,int *);
extern int do_layer1(struct mpstr *mp, struct frame *fr,unsigned char *,int *);
extern int decode_header(struct frame *fr,unsigned long newhead);

struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      real *full_gain[3];
      real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern int synth_1to1 (struct mpstr *mp, real *,int,unsigned char *,int *);
extern int synth_1to1_mono (struct mpstr *mp, real *,unsigned char *,int *);

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables(long scale);
extern void dct64(real *,real *,real *);

extern long freqs[9];
extern real muls[27][64];
extern real decwin[512+32];
extern real *pnts[5];

#endif
