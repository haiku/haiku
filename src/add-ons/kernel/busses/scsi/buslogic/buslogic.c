/*
** $Id: sim_buslogic.c,v 1.8 1998/04/21 00:54:52 swetland Exp $
**
** SCSI Interface Module for BusLogic MultiMaster Controllers
** Copyright 1998, Brian J. Swetland <swetland@frotz.net>
**
** Portions Copyright (c) 1994 by Be Incorporated.
** This file may be used under the terms of the Be Sample Code License.
*/


#include <BeBuild.h>
#include <iovec.h>
#include <KernelExport.h>

/*
** Debug options:
*/
#define DEBUG_BUSLOGIC   /* Print Debugging Messages                       */
#define xDEBUG_TRANSACTIONS
#define xSERIALIZE_REQS   /* only allow one req to the controller at a time */
#define xVERBOSE_IRQ      /* enable kprintf()s in the IRQ handler           */

#include <ByteOrder.h>
/*
** Macros for virt <-> phys translation
*/
#define PhysToVirt(n) ((void *) (((uint32) n) + bl->phys_to_virt))
#define VirtToPhys(n) (((uint32) n) + bl->virt_to_phys)

/*
** Debugging Macros
*/
#ifdef DEBUG_BUSLOGIC
#define d_printf dprintf
#ifdef DEBUG_TRANSACTIONS
#define dt_printf dprintf
#else
#define dt_printf(x...)
#endif
#else
#define d_printf(x...)
#endif

/* TODO:
**
** - endian issues: the MultiMaster is little endian, should use swap()
**   macro for PPC compatibility whenever exchanging addresses with it (DONE)
** - wrap phys addrs with ram_address()
** - support scatter-gather in the ccb_scsiio struct (DONE)
** - allocte bl_ccb_## semaphores on-demand (1/2 - only use 32)
** - error checking for *_sem functions... we're without a net right now
** - sync? (DONE - card handles this)
** - tagged queuing?
*/

#include <OS.h>
#include <KernelExport.h>
#include <PCI.h>
#include <CAM.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buslogic.h"

/*
** Constants for the SIM
*/
#define SIM_VERSION 0x01
#define HBA_VERSION 0x01

static char sim_vendor_name[]   = "Be, Inc.";
static char hba_vendor_name[]   = "BusLogic";
static char controller_family[] = "MultiMaster PCI";

static pci_module_info		*pci;
static cam_for_sim_module_info	*cam;

static char	cam_name[] = B_CAM_FOR_SIM_MODULE_NAME;
static char	pci_name[] = B_PCI_MODULE_NAME;

/*
** IO Macros
*/

/* XXX - fix me bjs */
#define inb(p) (*pci->read_io_8)(p)
#define outb(p,v) (*pci->write_io_8)(p,v)

#ifdef __i386__
#define toLE(x) (x)
#define unLE(x) (x)
#else
#define toLE(x) B_HOST_TO_LENDIAN_INT32(x)
#define unLE(x) B_LENDIAN_TO_HOST_INT32(x)
#endif


static int32
scsi_int_dispatch(void *data)
{
    BusLogic *bl = (BusLogic *) data;
    BL_CCB32 *bl_ccb;
    uchar intstat = inb(BL_INT_REG);

#if BOOT
#else
    if(!(intstat & BL_INT_INTV)) return B_UNHANDLED_INTERRUPT;
#endif
    if(intstat & BL_INT_CMDC) {
        bl->done = 1;
#ifdef VERBOSE_IRQ
        kprintf("buslogic_irq: Command Complete\n");
#endif
    }

	if(intstat & BL_INT_RSTS){
		kprintf("buslogic_irq: BUS RESET\n");
	}

        /* have we got mail? */
    if(intstat & BL_INT_IMBL){
        while(bl->in_boxes[bl->in_nextbox].completion_code){
            bl_ccb = (BL_CCB32 *)
                PhysToVirt(unLE(bl->in_boxes[bl->in_nextbox].ccb_phys));

#ifdef VERBOSE_IRQ
            kprintf("buslogic_irq: CCB %08x (%08x) done, cc=0x%02x\n",
                    unLE(bl->in_boxes[bl->in_nextbox].ccb_phys), (uint32) bl_ccb,
                    bl->in_boxes[bl->in_nextbox].completion_code);
#endif
            release_sem_etc(bl_ccb->done, 1, B_DO_NOT_RESCHEDULE);

            bl->in_boxes[bl->in_nextbox].completion_code = 0;
            bl->in_nextbox++;
            if(bl->in_nextbox == bl->box_count) bl->in_nextbox = 0;
        }
    }

        /* acknowledge the irq */
    outb(BL_CONTROL_REG, BL_CONTROL_RINT);

    return B_HANDLED_INTERRUPT;
}


/* Execute a command, with optional params send (in[in_len]) and
** results received (out[out_len])
*/
static int bl_execute(BusLogic *bl, uchar command,
                      void *in, int in_len,
                      void *out, int out_len)
{
    uchar status;
    uchar *_in, *_out;
#ifdef TIMEOUT
	int timeout;
#endif

    _in = (uchar *) in;
    _out = (uchar *) out;

    if(!(inb(BL_STATUS_REG) & BL_STATUS_HARDY)) {
        d_printf("buslogic: command 0x%02x %d/%d, not ready\n",
                 command, in_len, out_len);
        return 1;
    }

    outb(BL_COMMAND_REG, command);

#ifdef TIMEOUT
	timeout = 100;
#endif
    while(in_len){
        status = inb(BL_STATUS_REG);
        if(status & BL_STATUS_CMDINV) {
            d_printf("buslogic: command 0x%02x %d/%d invalid\n",
                     command, in_len, out_len);
            return 1;
        }
        if(status & BL_STATUS_CPRBSY) {
#ifdef TIMEOUT
			timeout--;
			if(!timeout) {
				d_printf("buslogic: command 0x%02 timed out (a)\n",command);
				return 1;
			}
			spin(100);
#endif
			continue;
		}
        outb(BL_COMMAND_REG, *_in);
        _in++;
        in_len--;
    }

#ifdef TIMEOUT
	timeout = 100;
#endif
    while(out_len){
        status = inb(BL_STATUS_REG);
        if(status & BL_STATUS_CMDINV) {
            d_printf("buslogic: command 0x%02x %d/%d invalid\n",
                     command, in_len, out_len);
            return 1;
        }
        if(status & BL_STATUS_DIRRDY) {
            *_out = inb(BL_DATA_REG);
            _out++;
            out_len--;
        } else {
#ifdef TIMEOUT
			timeout--;
			if(!timeout) {
				d_printf("buslogic: command 0x%02 timed out (b)\n",command);
				return 1;
			}
			spin(100);
#endif
		}
    }

#ifdef TIMEOUT
	timeout = 100;
#endif
    while(!(inb(BL_STATUS_REG) & BL_STATUS_HARDY)) {
#ifdef TIMEOUT
		timeout--;
		if(!timeout) {
			d_printf("buslogic: command 0x%02 timed out (c)\n",command);
			return 1;
		}
		spin(100);
#endif
	}

    return 0;
}

/* Initialize the BT-9X8 and confirm that it is operating as expected
*/
static long init_buslogic(BusLogic *bl)
{
    uchar status;
    uchar id[16];
    int i;
    char *str = bl->productname;

    d_printf("buslogic: init_buslogic()\n");

    dprintf("buslogic: reset: ");
    outb(BL_CONTROL_REG, BL_CONTROL_RHARD);
    spin(10000); /* give the controller some time to settle down from reset */

    for(i=0;i<1000;i++){
        spin(100000);
        status = inb(BL_STATUS_REG);
        if(status & BL_STATUS_DACT){
            dprintf(".");
            continue;
        }
        if(status & BL_STATUS_DFAIL){
            dprintf(" FAILED\n");
            return -1;
        }
        if(status & BL_STATUS_INREQ){
            dprintf(" OKAY\n");
            break;
        }
        if(status & BL_STATUS_HARDY) {
            dprintf(" READY\n");
            break;
        }
    }
    if(i==100) {
        dprintf(" TIMEOUT\n");
        return -1;
    }

    if(bl_execute(bl, 0x04, NULL, 0, id, 4)){
        d_printf("buslogic: can't id?\n");
        return B_ERROR;
    }
    d_printf("buslogic: Firmware Rev %c.%c\n",id[2],id[3]);

	id[0]=14;
	id[14]=id[2];

	if(bl_execute(bl, 0x8d, id, 1, id, 14)){
		d_printf("buslogic: cannot read extended config\n");
		return B_ERROR;
	}

	d_printf("buslogic: rev = %c.%c%c%c mb = %d, sgmax = %d, flags = 0x%02x\n",
			id[14], id[10], id[11], id[12], id[4], id[2] | (id[3]<<8), id[13]);
	if(id[13] & 0x01) bl->wide = 1;
	else bl->wide = 0;

	if(id[14] == '5'){
		*str++ = 'B';
		*str++ = 'T';
		*str++ = '-';
		*str++ = '9';
		*str++ = bl->wide ? '5' : '4';
		*str++ = '8';
		if(id[13] & 0x02) *str++ = 'D';
		*str++ = ' ';
		*str++ = 'v';
		*str++ = id[14];
		*str++ = '.';
		*str++ = id[10];
		*str++ = id[11];
		*str++ = id[12];
		*str++ = 0;
	} else {
		strcpy(str,"unknown");
	}
	if(bl_execute(bl, 0x0B, NULL, 0, id, 3)){
		d_printf("buslogic: cannot read config\n");
		return B_ERROR;
	}
	bl->scsi_id = id[2];
	d_printf("buslogic: Adapter SCSI ID = %d\n",bl->scsi_id);

    if(install_io_interrupt_handler(bl->irq, scsi_int_dispatch, bl, 0)
       == B_ERROR) d_printf("buslogic: can't install irq handler\n");

        /* are we getting INTs? */
    bl->done = 0;
    spin(10000);
    outb(BL_COMMAND_REG, 0x00);
    spin(1000000);
    if(bl->done) {
        d_printf("buslogic: interrupt test passed\n");
    } else {
        d_printf("buslogic: interrupt test failed\n");
        return B_ERROR;
    }

        /* strict round-robin on */
    id[0] = 0;
    if(bl_execute(bl,0x8F, id, 1, NULL, 0)){
        d_printf("buslogic: cannot enable strict round-robin mode\n");
        return B_ERROR;
    }


    id[0] = bl->box_count;
{ int mbaddr = toLE(bl->phys_mailboxes);
    memcpy(id + 1, &(mbaddr),4);
}
    if(bl_execute(bl, 0x81, id, 5, NULL, 0)){
        d_printf("buslogic: cannot init mailboxes\n");
        return B_ERROR;
    }
    d_printf("buslogic: %d mailboxes @ 0x%08xv/0x%08lxp\n",
             bl->box_count, (uint) bl->out_boxes, bl->phys_mailboxes);

    return B_NO_ERROR;
}



/* sim_invalid()
**
** Generic invalid XPT command response
*/
static long sim_invalid(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    d_printf("sim_invalid\n");
    return B_ERROR;
}


/* Convert a CCB_SCSIIO into a BL_CCB32 and (possibly SG array).
**
**
*/

static long sim_execute_scsi_io(BusLogic *bl, CCB_HEADER *ccbh)
{
    CCB_SCSIIO *ccb;
    int cdb_len;
    BL_CCB32 *bl_ccb;
    BL_PRIV *priv;
    uint32 priv_phys;
    uint32 bl_ccb_phys;
    physical_entry entries[2];
    physical_entry *scratch;
    uint32 tmp;
    int i,t,req;

    ccb = (CCB_SCSIIO *) ccbh;

#ifdef DEBUG_BUSLOGIC
    req = atomic_add(&(bl->reqid),1);
#endif

        /* valid cdb len? */
    cdb_len = ccb->cam_cdb_len;
    if (cdb_len != 6 && cdb_len != 10 && cdb_len != 12) {
        ccb->cam_ch.cam_status = CAM_REQ_INVALID;
        return B_ERROR;
    }

        /* acquire a CCB32 block */
    acquire_sem(bl->ccb_count);

        /* protect the freelist and unchain the CCB32 from it */
    acquire_sem(bl->ccb_lock);
    bl_ccb = bl->first_ccb;
    bl->first_ccb = bl_ccb->next;
    release_sem(bl->ccb_lock);

    bl_ccb_phys = VirtToPhys(bl_ccb);


        /* get contiguous area for bl_ccb in the private data area */
    get_memory_map((void *)ccb->cam_sim_priv, 4096, entries, 2);

	priv_phys = (uint32) entries[0].address;
	priv = (BL_PRIV *) ccb->cam_sim_priv;

        /* copy over the CDB */
    if(ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
        memcpy(bl_ccb->cdb, ccb->cam_cdb_io.cam_cdb_ptr, cdb_len);
    } else {
        memcpy(bl_ccb->cdb, ccb->cam_cdb_io.cam_cdb_bytes, cdb_len);
    }

        /* fill out the ccb header */
    bl_ccb->direction = BL_CCB_DIR_DEFAULT;
    bl_ccb->length_cdb = cdb_len;
    bl_ccb->length_sense = ccb->cam_sense_len;
    bl_ccb->_reserved1 = bl_ccb->_reserved2 = 0;
    bl_ccb->target_id = ccb->cam_ch.cam_target_id;
    bl_ccb->lun_tag = ccb->cam_ch.cam_target_lun & 0x07;
    bl_ccb->ccb_control = 0;
    bl_ccb->link_id = 0;
    bl_ccb->link = 0;
    bl_ccb->sense = toLE(priv_phys);

        /* okay, this is really disgusting and could potentially
           break if physical_entry{} changes format... we use the
           sg list as a scratchpad.  Disgusting, but a start */

    scratch = (physical_entry *) priv->sg;


	if(ccb->cam_ch.cam_flags & CAM_SCATTER_VALID){
		/* we're using scatter gather -- things just got trickier */
		iovec *iov = (iovec *) ccb->cam_data_ptr;
		int j,sgcount = 0;

		/* dprintf("buslogic: sg count = %d\n",ccb->cam_sglist_cnt);*/
          /* multiple entries, use SG */
		bl_ccb->opcode = BL_CCB_OP_INITIATE_RETLEN_SG;
		bl_ccb->data = toLE(priv_phys + 256);

		/* for each entry in the sglist we were given ... */
		for(t=0,i=0;i<ccb->cam_sglist_cnt;i++){
			/* map it ... */
		    get_memory_map(iov[i].iov_base, iov[i].iov_len, &(scratch[sgcount]),
	                   MAX_SCATTER - sgcount);

			/* and make a bl sgentry for each chunk ... */
			for(j=sgcount;scratch[j].size && j<MAX_SCATTER;j++){
	            t += scratch[j].size;
				sgcount++;
	            dt_printf("buslogic/%d: SG %03d - 0x%08x (%d)\n",req,
			  j, (uint32) scratch[j].address, scratch[j].size);

	            tmp = priv->sg[j].length;
	            priv->sg[j].length = toLE(priv->sg[j].phys);
	            priv->sg[j].phys = toLE(tmp);
	        }

			if(scratch[j].size) panic("egads! sgseg overrun in BusLogic SIM");
		}
        if(t != ccb->cam_dxfer_len){
            dt_printf("buslogic/%d: error, %d != %d\n",req,t,ccb->cam_dxfer_len);
            ccb->cam_ch.cam_status = CAM_REQ_INVALID;

                /* put the CCB32 back on the freelist and release our lock */
            acquire_sem(bl->ccb_lock);
            bl_ccb->next = bl->first_ccb;
            bl->first_ccb = bl_ccb;
            release_sem(bl->ccb_lock);
            release_sem(bl->ccb_count);
            return B_ERROR;
        }
        /* total bytes in DataSegList */
        bl_ccb->length_data = toLE(sgcount * 8);
	} else {
	    get_memory_map((void *)ccb->cam_data_ptr, ccb->cam_dxfer_len, scratch,
	                   MAX_SCATTER);

	    if(scratch[1].size){
	            /* multiple entries, use SG */
	        bl_ccb->opcode = BL_CCB_OP_INITIATE_RETLEN_SG;
	        bl_ccb->data = toLE(priv_phys + 256);
	        for(t=0,i=0;scratch[i].size && i<MAX_SCATTER;i++){
	            t += scratch[i].size;
	            dt_printf("buslogic/%d: SG %03d - 0x%08x (%d)\n",req,
						  i, (uint32) scratch[i].address, scratch[i].size);

	            tmp = priv->sg[i].length;
	            priv->sg[i].length = toLE(priv->sg[i].phys);
	            priv->sg[i].phys = toLE(tmp);
	        }
	        if(t != ccb->cam_dxfer_len){
	            dt_printf("buslogic/%d: error, %d != %d\n",req,t,ccb->cam_dxfer_len);
	            ccb->cam_ch.cam_status = CAM_REQ_INVALID;

	                /* put the CCB32 back on the freelist and release our lock */
	            acquire_sem(bl->ccb_lock);
	            bl_ccb->next = bl->first_ccb;
	            bl->first_ccb = bl_ccb;
	            release_sem(bl->ccb_lock);
	            release_sem(bl->ccb_count);
	            return B_ERROR;
	        }
	            /* total bytes in DataSegList */
	        bl_ccb->length_data = toLE(i * 8);

	    } else {
	        bl_ccb->opcode = BL_CCB_OP_INITIATE_RETLEN;
	            /* single entry, use direct */
	        t = bl_ccb->length_data = toLE(ccb->cam_dxfer_len);
	        bl_ccb->data = toLE((uint32) scratch[0].address);
	    }
	}

    dt_printf("buslogic/%d: targ %d, dxfr %d, scsi op = 0x%02x\n",req,
			  bl_ccb->target_id, t, bl_ccb->cdb[0]);

    acquire_sem(bl->hw_lock);

        /* check for box in use state XXX */
    bl->out_boxes[bl->out_nextbox].ccb_phys = toLE(bl_ccb_phys);
    bl->out_boxes[bl->out_nextbox].action_code = BL_ActionCode_Start;
    bl->out_nextbox++;
    if(bl->out_nextbox == bl->box_count) bl->out_nextbox = 0;
    outb(BL_COMMAND_REG, 0x02);

#ifndef SERIALIZE_REQS
    release_sem(bl->hw_lock);
#endif
/*    d_printf("buslogic/%d: CCB %08x (%08xv) waiting on done\n",
	  req, bl_ccb_phys, (uint32) bl_ccb);*/
    acquire_sem(bl_ccb->done);
/*    d_printf("buslogic/%d: CCB %08x (%08xv) done\n",
	  req, bl_ccb_phys, (uint32) bl_ccb);*/

#ifdef SERIALIZE_REQS
    release_sem(bl->hw_lock);
#endif

    if(bl_ccb->btstat){
            /* XXX - error state xlat goes here */
        switch(bl_ccb->btstat){
		case 0x11:
			ccb->cam_ch.cam_status = CAM_SEL_TIMEOUT;
			break;
		case 0x12:
			ccb->cam_ch.cam_status = CAM_DATA_RUN_ERR;
			break;
		case 0x13:
			ccb->cam_ch.cam_status = CAM_UNEXP_BUSFREE;
			break;
		case 0x22:
		case 0x23:
			ccb->cam_ch.cam_status = CAM_SCSI_BUS_RESET;
			break;
		case 0x34:
			ccb->cam_ch.cam_status = CAM_UNCOR_PARITY;
			break;
		default:
	        ccb->cam_ch.cam_status = CAM_REQ_INVALID;
		}
        dt_printf("buslogic/%d: error stat %02x\n",req,bl_ccb->btstat);
    } else {
        dt_printf("buslogic/%d: data %d/%d, sense %d/%d\n", req,
				  bl_ccb->length_data, ccb->cam_dxfer_len,
				  bl_ccb->length_sense, ccb->cam_sense_len);

        ccb->cam_resid = bl_ccb->length_data;

            /* under what condition should we do this? */
        memcpy(ccb->cam_sense_ptr, priv->sensedata, ccb->cam_sense_len);

        ccb->cam_scsi_status = bl_ccb->sdstat;

        if(bl_ccb->sdstat == 02){
            ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR | CAM_AUTOSNS_VALID;
            ccb->cam_sense_resid = 0;
            dt_printf("buslogic/%d: error scsi\n",req);
        } else {
            ccb->cam_ch.cam_status = CAM_REQ_CMP;
            ccb->cam_sense_resid = bl_ccb->length_sense;
            dt_printf("buslogic/%d: success scsi\n",req);
                /* put the CCB32 back on the freelist and release our lock */
            acquire_sem(bl->ccb_lock);
            bl_ccb->next = bl->first_ccb;
            bl->first_ccb = bl_ccb;
            release_sem(bl->ccb_lock);
            release_sem(bl->ccb_count);
            return 0;

        }
    }

        /* put the CCB32 back on the freelist and release our lock */
    acquire_sem(bl->ccb_lock);
    bl_ccb->next = bl->first_ccb;
    bl->first_ccb = bl_ccb;
    release_sem(bl->ccb_lock);
    release_sem(bl->ccb_count);
    return B_ERROR;
}


/*
** sim_path_inquiry returns info on the target/lun.
*/
static long sim_path_inquiry(BusLogic *bl, CCB_HEADER *ccbh)
{
    CCB_PATHINQ	*ccb;
    d_printf("buslogic: sim_path_inquiry()\n");

    ccb = (CCB_PATHINQ *) ccbh;

    ccb->cam_version_num = SIM_VERSION;
    ccb->cam_target_sprt = 0;
    ccb->cam_hba_eng_cnt = 0;
    memset (ccb->cam_vuhba_flags, 0, VUHBA);
    ccb->cam_sim_priv = SIM_PRIV;
    ccb->cam_async_flags = 0;
    ccb->cam_initiator_id = bl->scsi_id;
	ccb->cam_hba_inquiry = bl->wide ? PI_WIDE_16 : 0;
    strncpy (ccb->cam_sim_vid, sim_vendor_name, SIM_ID);
    strncpy (ccb->cam_hba_vid, hba_vendor_name, HBA_ID);
    ccb->cam_osd_usage = 0;
    ccbh->cam_status = CAM_REQ_CMP;
    return 0;
}


/*
** sim_extended_path_inquiry returns info on the target/lun.
*/
static long sim_extended_path_inquiry(BusLogic *bl, CCB_HEADER *ccbh)
{
    CCB_EXTENDED_PATHINQ *ccb;

    sim_path_inquiry(bl, ccbh);
    ccb = (CCB_EXTENDED_PATHINQ *) ccbh;
    sprintf(ccb->cam_sim_version, "%d.0", SIM_VERSION);
    sprintf(ccb->cam_hba_version, "%d.0", HBA_VERSION);
    strncpy(ccb->cam_controller_family, controller_family, FAM_ID);
    strncpy(ccb->cam_controller_type, bl->productname, TYPE_ID);
    return 0;
}


/*
** sim_release_queue unfreezes the target/lun's request queue.
*/
static long sim_sim_release_queue(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    return B_ERROR;
}


/*
** sim_set_async_callback registers a callback routine for the
** target/lun;
*/
static long sim_set_async_callback(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    return B_ERROR;
}


/*
** sim_abort aborts a pending or queued scsi operation.
*/
static long sim_abort(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    return B_ERROR;
}


/*
** sim_reset_bus resets the scsi bus.
*/
static long sim_reset_bus(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_CMP;
    return 0;
}


/*
** sim_reset_device resets the target/lun with the scsi "bus
** device reset" command.
*/
static long sim_reset_device(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    return B_ERROR;
}


/*
** sim_terminate_process terminates the scsi i/o request without
** corrupting the medium.  It is used to stop lengthy requests
** when a higher priority request is available.
**
** Not yet implemented.
*/
static long sim_terminate_process(BusLogic *bl, CCB_HEADER *ccbh)
{
    ccbh->cam_status = CAM_REQ_INVALID;
    return B_ERROR;
}


/*
** scsi_sim_action performes the scsi i/o command embedded in the
** passed ccb.
**
** The target/lun ids are assumed to be in range.
*/
static long sim_action(BusLogic *bl, CCB_HEADER *ccbh)
{
    static long (*sim_functions[])(BusLogic *, CCB_HEADER *) = {
        sim_invalid,		/* do nothing */
        sim_execute_scsi_io,	/* execute a scsi i/o command */
        sim_invalid,		/* get device type info */
        sim_path_inquiry,	/* path inquiry */
        sim_sim_release_queue,	/* release a frozen SIM queue */
        sim_set_async_callback,	/* set async callback parameters */
        sim_invalid,		/* set device type info */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_invalid,		/* invalid function code */
        sim_abort,		/* abort the selected CCB */
        sim_reset_bus,		/* reset a SCSI bus */
        sim_reset_device,	/* reset a SCSI device */
        sim_terminate_process	/* terminate an i/o process */
    };
    uchar op;

    /* d_printf("buslogic: sim_execute(), op = %d\n", ccbh->cam_func_code); */

		/* check for function codes out of range of dispatch table */
    op = ccbh->cam_func_code;
    if ((op >= sizeof (sim_functions) / sizeof (long (*)())) &&
        (op != XPT_EXTENDED_PATH_INQ)) {
            /* check for our vendor-uniques (if any) here... */
        ccbh->cam_status = CAM_REQ_INVALID;
        return -1;
    }

    ccbh->cam_status = CAM_REQ_INPROG;
    if (op == XPT_EXTENDED_PATH_INQ) {
        return sim_extended_path_inquiry(bl, ccbh);
    } else {
        return (*sim_functions [op])(bl, ccbh);
    }
}


static char *hextab = "0123456789ABCDEF";

/*
** Allocate the actual memory for the cardinfo object
*/
static BusLogic *create_cardinfo(int num, int iobase, int irq)
{
    uchar *a;
    area_id aid;
    int i;
    physical_entry entries[5];
    char name[9] = { 'b', 'l', '_', 'c', 'c', 'b', '0', '0', 0 };

    BusLogic *bl = (BusLogic *) malloc(sizeof(BusLogic));

#ifndef __i386__
	i = map_physical_memory("bl_regs", iobase,  4096,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &a);
	iobase = (uint32) a;
	if(i < 0) {
		dprintf("buslogic: can't map registers...\n");
	}
#endif

    bl->id = num;
    bl->iobase = iobase;
    bl->irq = irq;
    bl->out_nextbox = bl->in_nextbox = 0;

        /* create our 20k workspace.  First 4k goes to 510 mailboxes.
        ** remaining 16k is used for up to 256 CCB32's
        */
    a = NULL;

#if BOOT
        /* life in the bootstrap is a bit different... */
        /* can't be sure of getting contig pages -- scale
           stuff down so we can live in just one page */
    bl->box_count = 4;
    if(!(a = malloc(4096*2))) {
        free(bl);
        return NULL;
    }
    a = (uchar *) ((((uint32) a) & 0xFFFFF000) + 0x1000);
    get_memory_map(a, 4096, entries, 2);
#else
    bl->box_count = MAX_CCB_COUNT;
    aid = create_area("bl_workspace", (void **)&a, B_ANY_KERNEL_ADDRESS, 4096*5,
		B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
    if(aid == B_ERROR || aid == B_BAD_VALUE || aid == B_NO_MEMORY) {
        free(bl);
        return NULL;
    }
    get_memory_map(a, 4096*5, entries, 2);
#endif

        /* figure virtual <-> physical translations */
    bl->phys_to_virt = ((uint) a) - ((uint) entries[0].address);
    bl->virt_to_phys = (((uint) entries[0].address - (uint) a));
    bl->phys_mailboxes = (uint) entries[0].address;

        /* initialize all mailboxes to empty */
    bl->out_boxes = (BL_Out_Mailbox32 *) a;
    bl->in_boxes = (BL_In_Mailbox32 *) (a + (8 * bl->box_count));
    for(i=0;i<bl->box_count;i++){
        bl->out_boxes[i].action_code = BL_ActionCode_NotInUse;
        bl->in_boxes[i].completion_code = BL_CompletionCode_NotInUse;
    }

        /* setup the CCB32 cache */
#if BOOT
    bl->ccb = (BL_CCB32 *) (((uchar *)a) + 1024);
#else
    bl->ccb = (BL_CCB32 *) (((uchar *)a) + 4096);
#endif
    bl->first_ccb = NULL;
    for(i=0;i<bl->box_count;i++){
        name[6] = hextab[(i & 0xF0) >> 4];
        name[7] = hextab[i & 0x0F];
        bl->ccb[i].done = create_sem(0, name);
        bl->ccb[i].next = bl->first_ccb;
        bl->first_ccb = &(bl->ccb[i]);
    }

    bl->hw_lock = create_sem(1, "bl_hw_lock");
    bl->ccb_lock = create_sem(1, "bl_ccb_lock");
    bl->ccb_count = create_sem(MAX_CCB_COUNT, "bl_ccb_count");
    bl->reqid = 0;

    return bl;
}


/*
** Multiple Card Cruft
*/
#define MAXCARDS 4

static BusLogic *cardinfo[MAXCARDS] = { NULL, NULL, NULL, NULL };

static long sim_init0(void)                { return init_buslogic(cardinfo[0]); }
static long sim_init1(void)                { return init_buslogic(cardinfo[1]); }
static long sim_init2(void)                { return init_buslogic(cardinfo[2]); }
static long sim_init3(void)                { return init_buslogic(cardinfo[3]); }
static long sim_action0(CCB_HEADER *ccbh)  { return sim_action(cardinfo[0],ccbh); }
static long sim_action1(CCB_HEADER *ccbh)  { return sim_action(cardinfo[1],ccbh); }
static long sim_action2(CCB_HEADER *ccbh)  { return sim_action(cardinfo[2],ccbh); }
static long sim_action3(CCB_HEADER *ccbh)  { return sim_action(cardinfo[3],ccbh); }


static long (*sim_init_funcs[MAXCARDS])(void) = {
    sim_init0, sim_init1, sim_init2, sim_init3
};

static long (*sim_action_funcs[MAXCARDS])(CCB_HEADER *) = {
    sim_action0, sim_action1, sim_action2, sim_action3
};


/*
** Detect the controller and register the SIM with the CAM layer.
** returns the number of controllers installed...
*/
static int
sim_install_buslogic(void)
{
    int i, iobase, irq;
    int cardcount = 0;
    pci_info h;
    CAM_SIM_ENTRY entry;

   /* d_printf("buslogic: sim_install()\n"); */

    for (i = 0; ; i++) {
        if ((*pci->get_nth_pci_info) (i, &h) != B_NO_ERROR) {
            /* if(!cardcount) d_printf("buslogic: no controller found\n"); */
            break;
        }

        if ((h.vendor_id == PCI_VENDOR_BUSLOGIC) &&
            (h.device_id == PCI_DEVICE_MULTIMASTER)) {

#ifdef __i386__
            iobase = h.u.h0.base_registers[0];
#else
            iobase = h.u.h0.base_registers[1];
#endif
            irq = h.u.h0.interrupt_line;

            d_printf("buslogic%d: controller @ 0x%08x, irq %d\n",
                     cardcount, iobase, irq);
    		if((irq == 0) || (irq > 128)) {
				dprintf("buslogic%d: bad irq %d\n",cardcount,irq);
        		continue;
    		}

            if(cardcount == MAXCARDS){
                d_printf("buslogic: too many controllers!\n");
                return cardcount;
            }

            if((cardinfo[cardcount] = create_cardinfo(cardcount,iobase,irq))){
                entry.sim_init = sim_init_funcs[cardcount];
                entry.sim_action = sim_action_funcs[cardcount];
                (*cam->xpt_bus_register)(&entry);
                cardcount++;
            } else {
                d_printf("buslogic: cannot allocate cardinfo\n");
            }
        }
    }

    return cardcount;
}

static status_t std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		if (get_module(pci_name, (module_info **) &pci) != B_OK)
			return B_ERROR;

		if (get_module(cam_name, (module_info **) &cam) != B_OK) {
			put_module(pci_name);
			return B_ERROR;
		}

		if(sim_install_buslogic()){
			return B_OK;
		}

		put_module(pci_name);
		put_module(cam_name);
		return B_ERROR;

	case B_MODULE_UNINIT:
		put_module(pci_name);
		put_module(cam_name);
		return B_OK;

	default:
		return B_ERROR;
	}
}

#if BUILD_LOADABLE
static
#endif
sim_module_info sim_buslogic_module = {
	{ "busses/scsi/buslogic/v1", 0, &std_ops }
};

#if BUILD_LOADABLE
_EXPORT module_info  *modules[] =
{
	(module_info *) &sim_buslogic_module,
	NULL
};
#endif
