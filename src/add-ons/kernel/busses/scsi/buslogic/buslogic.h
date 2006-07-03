/*
** $Id: buslogic.h,v 1.2 1998/04/21 00:51:57 swetland Exp $
** 
** Constants and Structures for BusLogic MultiMaster Controllers
** Copyright 1998, Brian J. Swetland <swetland@frotz.net>
**
** This file may be used under the terms of the Be Sample Code License.
*/

#define PCI_VENDOR_BUSLOGIC     0x104b
#define PCI_DEVICE_MULTIMASTER  0x1040


/* BusLogic MultiMaster Register Definitions
** cf. Technical Reference Manual, pp. 1-10 - 1-17
*/
#define BL_CONTROL_REG    (bl->iobase)
#define BL_CONTROL_RHARD  0x80           /* Controller Hard Reset           */
#define BL_CONTROL_RSOFT  0x40           /* Controller Soft Reset           */
#define BL_CONTROL_RINT   0x20           /* Reset (Acknowledge) Interrupts  */
#define BL_CONTROL_RSBUS  0x10           /* Reset SCSI Bus                  */

#define BL_STATUS_REG     (bl->iobase)
#define BL_STATUS_DACT    0x80           /* Diagnostic Active               */
#define BL_STATUS_DFAIL   0x40           /* Diagnostic Failure              */
#define BL_STATUS_INREQ   0x20           /* Initialization Required         */
#define BL_STATUS_HARDY   0x10           /* Host Adapter Ready              */
#define BL_STATUS_CPRBSY  0x08           /* Command/Param Out Register Busy */
#define BL_STATUS_DIRRDY  0x04           /* Data In Register Ready          */
#define BL_STATUS_CMDINV  0x01           /* Command Invalid                 */

#define BL_COMMAND_REG    (bl->iobase + 1)
#define BL_DATA_REG       (bl->iobase + 1)

#define BL_INT_REG        (bl->iobase + 2)
#define BL_INT_INTV       0x80           /* Interrupt Valid                 */
#define BL_INT_RSTS       0x08           /* SCSI Reset State                */
#define BL_INT_CMDC       0x04           /* Command Complete                */
#define BL_INT_MBOR       0x02           /* Mailbox Out Ready               */
#define BL_INT_IMBL       0x01           /* Incoming Mailbox Loaded         */


#define MAX_SCATTER 130

typedef struct _bl_ccb32
{
        /* buslogic ccb structure */
    uchar   opcode;         /* operation code - see CCB_OP_* below         */
    uchar   direction;      /* data direction control - see CCB_DIR_*      */
    uchar   length_cdb;     /* length of the cdb                           */
    uchar   length_sense;   /* length of sense data block                  */
    uint32  length_data;    /* length of data block                        */
    uint32  data;           /* 32bit physical pointer to data or s/g table */
    uchar   _reserved1;     /* set to zero                                 */
    uchar   _reserved2;     /* set to zero                                 */
    uchar   btstat;         /* Host Adapter Status Return                  */
    uchar   sdstat;         /* SCSI Device Status Return                   */
    uchar   target_id;      /* Target SCSI ID                              */
    uchar   lun_tag;        /* bits 0-2 = LUN, | with CCB_TAG_*            */
    uchar   cdb[12];        /* SCSI CDB area                               */
    uchar   ccb_control;    /* controle bits - see CCB_CONTROL_*           */
    uchar   link_id;        /* id number for linked CCB's                  */
    uint32  link;           /* 32bit physical pointer to a linked CCB      */
    uint32  sense;          /* 32bit physical pointer to the sense datablk */

        /* used by the driver */
    
    sem_id  done;           /* used by ISR for completion notification     */
    int completion_code;    /* completion code storage from mailbox        */
    struct _bl_ccb32 *next; /* chain pointer for CCB32 freelist            */
    uint _reserved[3];      /* padding                                     */
} BL_CCB32;

typedef struct 
{
        /* used by the driver */
    uchar sensedata[256];   /* data area for sense data return             */
    
    struct {
        uint length;        /* length of this SG segment (bytes)           */
        uint phys;          /* physical address of this SG segment         */
    } sg[MAX_SCATTER];      /* scatter/gather table                        */
} BL_PRIV;

#define BL_CCB_TAG_SIMPLE    0x20
#define BL_CCB_TAG_HEAD      0x60
#define BL_CCB_TAG_ORDERED   0xA0

#define BL_CCB_OP_INITIATE           0x00 
#define BL_CCB_OP_INITIATE_SG        0x02

/* returns dif between req/actual xmit bytecount */
#define BL_CCB_OP_INITIATE_RETLEN    0x03
#define BL_CCB_OP_INITIATE_RETLEN_SG 0x04
#define BL_CCB_OP_SCSI_BUS_RESET     0x81

#define BL_CCB_DIR_DEFAULT   0x00  /* transfer in direction native to scsi */
#define BL_CCB_DIR_INBOUND   0x08
#define BL_CCB_DIR_OUTBOUND  0x10
#define BL_CCB_DIR_NO_XFER   0x18

typedef struct 
{
    uint32 ccb_phys;
    uchar _reserved1;
    uchar _reserved2;
    uchar _reserved3;
    uchar action_code;
} BL_Out_Mailbox32;

#define BL_ActionCode_NotInUse   0x00
#define BL_ActionCode_Start      0x01
#define BL_ActionCode_Abort      0x02
 


typedef struct 
{
    uint32 ccb_phys;
    uchar btstat;
    uchar sdstat;
    uchar _reserved1;
    uchar completion_code;
} BL_In_Mailbox32;

#define BL_CompletionCode_NotInUse     0x00
#define BL_CompletionCode_NoError      0x01
#define BL_CompletionCode_HostAbort    0x02
#define BL_CompletionCode_NotFound     0x03
#define BL_CompletionCode_Error        0x04

#define MAX_CCB_COUNT 32

/* Host Adapter State Structure
**
*/
typedef struct 
{
    int id;                         /* board id 0, 1, ...    */
    int done;                       /* command complete from ISR */
    int irq;                        /* board's irq */
    int iobase;                     /* base io address */
	int scsi_id;					/* board's SCSI id */
	int wide;                       /* wide target id's allowed */
    long reqid;                     /* request counter for debugging */
    
	char productname[16];
	
    uint32 phys_to_virt;            /* adjustment for mapping BL_CCB32's */
    uint32 virt_to_phys;            /* between virt and phys addrs       */
    
    uint32 phys_mailboxes;          /* phys addr of mailboxes */
      
    sem_id hw_lock;                 /* lock for hardware and outbox access */
    sem_id ccb_lock;                /* lock protecting the ccb chain */
    sem_id ccb_count;               /* counting sem protecting the ccb chain */
    
    int box_count;
    int out_nextbox;
    int in_nextbox;
    BL_Out_Mailbox32  *out_boxes;
    BL_In_Mailbox32   *in_boxes;

    BL_CCB32 *ccb;                  /* table of MAX_CCB_COUNT CCB's */
    BL_CCB32 *first_ccb;            /* head of ccb freelist         */
} BusLogic;

