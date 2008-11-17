/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(_CM_PRIVATE_H)
#define _CM_PRIVATE_H

#if !defined(_CMEDIA_PCI_H)
#include "cmedia_pci.h"
#endif

#if !defined(_PCI_H)
#include <PCI.h>
#endif

#if !defined(DEBUG)
#define DEBUG 0
#endif


#define NUM_CARDS 3
#define DEVNAME 32


#define EXPORT __declspec(dllexport)

#if defined(__POWERPC__)
#define EIEIO() __eieio()
#else
#define EIEIO()
#endif

#if DEBUG
#define ddprintf(x) dprintf x
#define KTRACE() kprintf("%s %d\n", __FILE__, __LINE__)
#else
#define ddprintf(x)
#define KTRACE()
#endif

/* neither value may be larger than 65536 */
#define PLAYBACK_BUF_SIZE 2048
#define RECORD_BUF_SIZE PLAYBACK_BUF_SIZE
/* always create this much, so larger buffers can be requested */
#define MIN_MEMORY_SIZE 32768

/* clock crystal frequency */
#define F_REF 24576000
/* data book says 80 MHz ... */
#define MIN_FREQ 80000000
/* tolerance for sample rate clock derivation - book has this at 0.005 */
#define TOLERANCE 0.001

/* there are five logical devices: midi, joystick, pcm, mux and mixer */

typedef struct _midi_dev 
{
	struct _cmedia_pci_dev *card;
	void *		driver;
	void *		cookie;
	int32		count;
	char		name[64];
} midi_dev;

typedef struct _joy_dev 
{
	void *		driver;
	char		name1[64];
} joy_dev;

typedef cmedia_pci_audio_format pcm_cfg;
typedef cmedia_pci_audio_buf_header pcm_buf_hdr;

enum 
{	/* these map to the mode enable bits in the CMX13 register */
	kPlayback = 1,
	kRecord = 2
};

typedef struct 
{
	struct _cmedia_pci_dev * card;
	char		name[DEVNAME];
	char		oldname[DEVNAME];
	sem_id		init_sem;
	int32		open_count;
	int32		open_mode;

/* playback from a cyclic, small-ish buffer */

	int32		wr_lock;
	int			dma_a;
	vuchar *	wr_1;
	vuchar *	wr_2;
	vuchar *	wr_cur;
	size_t		wr_size;
	int			wr_silence;
	int32		write_waiting;
	sem_id		write_sem;
	size_t		was_written;
	uint32		wr_skipped;
	sem_id		wr_entry;
	bigtime_t	wr_time;
	uint64		wr_total;
	sem_id		wr_time_sem;
	int32		wr_time_wait;

/* recording into a cyclic, somewhat larger buffer */

	int32		rd_lock;
	int			dma_c;
	vuchar *	rd_1;
	vuchar *	rd_2;
	vuchar *	rd_cur;
	size_t		rd_size;
	int32		read_waiting;
	sem_id		read_sem;
	size_t		was_read;
	bigtime_t	next_rd_time;
	bigtime_t	rd_time;
	uint32		rd_skipped;	/* count of misses */
	sem_id		rd_entry;
	uint64		rd_total;
	sem_id		rd_time_sem;
	int32		rd_time_wait;

/* buffers are owned by the device record (because of allocation) */

	pcm_cfg		config;

	sem_id		old_cap_sem;
	sem_id		old_play_sem;
} pcm_dev;

typedef struct 
{
	struct _cmedia_pci_dev * card;
	char		name[DEVNAME];
	int32		open_count;
} mux_dev;

typedef struct 
{
	struct _cmedia_pci_dev * card;
	char		name[DEVNAME];
	int32		open_count;
} mixer_dev;

typedef struct _cmedia_pci_dev 
{
	char		name[DEVNAME];	/* used for resources */
	int32		hardware;		/* spinlock */
	int			enhanced;		/* offset to port */
	int32		inth_count;
	int			dma_base;
	size_t		low_size;		/* size of low memory */
	vuchar *	low_mem;
	vuchar *	low_phys;		/* physical address */
	area_id		map_low;		/* area pointing to low memory */
	pci_info	info;
	midi_dev	midi;
	joy_dev		joy;
	pcm_dev		pcm;
	mux_dev		mux;
	mixer_dev	mixer;
} cmedia_pci_dev;


extern int32 num_cards;
extern cmedia_pci_dev cards[NUM_CARDS];


extern void set_direct(cmedia_pci_dev *, int, uchar, uchar);
extern uchar get_direct(cmedia_pci_dev *, int);
extern void set_indirect(cmedia_pci_dev *, int, uchar, uchar);
extern uchar get_indirect(cmedia_pci_dev *, int);
extern void increment_interrupt_handler(cmedia_pci_dev *);
extern void decrement_interrupt_handler(cmedia_pci_dev *);


extern bool midi_interrupt(cmedia_pci_dev *);
extern void midi_interrupt_op(int32 op, void * data);
extern bool dma_a_interrupt(cmedia_pci_dev *);
extern bool dma_c_interrupt(cmedia_pci_dev *);

extern void PCI_IO_WR(int offset, uint8 val);
extern uint8 PCI_IO_RD(int offset);
extern uint32 PCI_IO_RD_32(int offset);

extern generic_gameport_module * gameport;

#endif	/*	_CM_PRIVATE_H	*/

