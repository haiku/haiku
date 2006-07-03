/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/*
** 53c8xx Driver - Data structures and shared constants 
*/

/* status codes signaled from SCRIPTS to driver */
#define status_ready           0x10  // idle loop interrupted by driver
#define status_reselected      0x11  // select or idle interrupted by reselection
#define status_timeout         0x12  // select timed out
#define status_selected        0x13  // select succeeded
#define status_complete        0x14  // transaction completed
#define status_disconnect      0x15  // device disconnected in the middle
#define status_badstatus       0x16  // snafu in the status phase
#define status_overrun         0x17  // data overrun occurred
#define status_underrun        0x18  // data underrun occurred
#define status_badphase        0x19  // weird phase transition occurred
#define status_badmsg          0x1a  // bad msg received
#define status_badextmsg       0x1b  // bad extended msg received
#define status_selftest        0x1c  // used by selftest stub
#define status_iocomplete      0x1d  
#define status_syncin          0x1e
#define status_widein          0x1f
#define status_ignore_residue  0x20

/* status codes private to driver */
#define status_inactive        0x00  // no request pending
#define status_queued          0x01  // start request is in the startqueue
#define status_selecting       0x02  // attempting to select
#define status_active          0x03  // SCRIPTS is handling it
#define status_waiting         0x04  // Waiting for reselection

#define OP_NDATA_IN  0x09000000L
#define OP_NDATA_OUT 0x08000000L
#define OP_WDATA_IN  0x01000000L
#define OP_WDATA_OUT 0x00000000L

#define OP_END      0x98080000L
#define ARG_END     (status_iocomplete)

typedef struct
{
	uint32 count;
	uint32 address;
} SymInd;

#define PATCH_DATAIN ((Ent_do_datain/4) + 1)
#define PATCH_DATAOUT ((Ent_do_dataout/4) + 1)


#define ctxt_device    0
#define ctxt_sendmsg   1
#define ctxt_recvmsg   2
#define ctxt_extdmsg   3
#define ctxt_syncmsg   4
#define ctxt_status    5
#define ctxt_command   6
#define ctxt_widemsg   7
#define ctxt_program   8

typedef struct 
{
	uchar _command[12];       /*  0 - 11 */
	uchar _syncmsg[2];        /* 12 - 13 */
	uchar _widemsg[2];        /* 14 - 15 */
	uchar _sendmsg[8];        /* 16 - 23 */
	uchar _recvmsg[1];        /*      24 */
	uchar _extdmsg[1];        /*      25 */
	uchar _status[1];         /*      26 */
	uchar _padding[1];        /*      27 */
 
	SymInd device;            /*      28 */
	SymInd sendmsg;           /*      36 */
	SymInd recvmsg;           /*      44 */
	SymInd extdmsg;           /*      52 */
	SymInd syncmsg;           /*      60 */
	SymInd status;            /*      68 */
	SymInd command;           /*      76 */
	SymInd widemsg;           /*      84 */
	
/* MUST be dword aligned! */
	SymInd table[131];        /*      92 --- 129 entries, 1 eot, 1 scratch */ 
} SymPriv;

#define ADJUST_PRIV_TO_DSA    28
#define ADJUST_PRIV_TO_TABLE  92

typedef struct _SymTarg
{
	struct _Symbios *adapter;
	struct _SymTarg *next;
	
	uchar device[4];     /* symbios register defs for the device */
	int sem_targ;        /* mutex allowing only one req per target */
	int sem_done;        /* notification semaphore */
	CCB_SCSIIO *ccb;     /* ccb for the current request for this target or NULL */
	
	SymPriv *priv;	     /* priv data area within ccb */
	uint32 priv_phys;    /* physical address of priv */
	uint32 table_phys;   /* physical address of sgtable */
	uint32 datain_phys;  
	uint32 dataout_phys;
	
	int inbound;         /* read data from device */
	
	uint32 period;       /* sync period */
	uint32 offset;       /* sync offset */
	uint32 wide;
	
	uint32 flags;
	uint32 status;
	uint32 id;	
} SymTarg;

#define tf_ask_sync   0x0001
#define tf_ask_wide   0x0002
#define tf_is_sync    0x0010
#define tf_is_wide    0x0020
#define tf_ignore     0x0100

typedef struct _Symbios
{
	uint32 num;           /* card number */
	uint32 iobase;        /* io base address */ 
	uint32 irq;           /* assigned irq */
	
	char *name;           /* device type name */
	uint32 host_targ_id; 
	uint32 max_targ_id;
	int reset;

	int registered;
		
	uint32 *script;       /* 1 page of on/offboard scripts ram */
	uint32 sram_phys;     /* physical address thereof */
	
	SymTarg targ[16];     /* one targ descriptor per target */
	spinlock hwlock;      /* lock protecting register access */
	
	SymTarg *startqueue;   /* target being started */
	SymTarg *startqueuetail;
	SymTarg *active;      /* target currently being interacted with */
	                      /* null if IDLE, == startqueue if starting */

	enum {
		OFFLINE, IDLE, START, ACTIVE, TEST
	} status;
	
	struct {
		uint period;    /* negotiated period */
		uint  period_ns; /* configured period in ns */
		uchar scntl3;    /* values for scntl3 SCF and CCF bits */
		uchar sxfer;     /* values for xfer TP2-0 bits */
	} syncinfo[16];
	uint32 syncsize;     /* number of syncinfo entries to look at */
	uint32 idmask;
	
	uint32 scntl3;
	uint32 sclk;         /* SCLK in KHz */
	uint32 maxoffset;
	
	uint32 op_in;
	uint32 op_out;
} Symbios;
