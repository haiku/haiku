
struct buf {
        unsigned char *pnt;
	long size;
	long pos;
        struct buf *next;
        struct buf *prev;
};

struct framebuf {
	struct buf *buf;
	long pos;
	struct frame *next;
	struct frame *prev;
};

struct mpstr {
	struct buf *head,*tail;
	int bsize;
	int framesize;
        int fsizeold;
	struct frame fr;
        unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned long header;
	int bsnum;
	real synth_buffs[2][2][0x110];
        int  synth_bo;
};

#ifndef BOOL
#define BOOL int
#endif

#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1


#ifdef __cplusplus
extern "C" {
#endif
  
BOOL InitMP3(struct mpstr *mp);
int decodeMP3(struct mpstr *mp,char *inmemory,int inmemsize,
     char *outmemory,int outmemsize,int *done);
void ExitMP3(struct mpstr *mp);

#ifdef __cplusplus
}
#endif


