/*
	Copyright 1998-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/*
** 53c8xx.c - Symbios 53c8xx SIM
*/

#define DEBUG_SYMBIOS  1   /* Print Debugging Messages */
#define DEBUG_ISR      0   /* messages in ISR... leave off */
#define DEBUG_PM       0   /* messages when Phase Mismatch occurs... leave off */
#define DEBUG_SAFETY   0   /* don't load driver if serial debug off */

#define USE_STATFS     0

/*
** Debugging Macros
*/
#if DEBUG_SYMBIOS
#define d_printf dprintf
#else
#define d_printf(x...)
#endif

#include <OS.h>
#include <KernelExport.h>
#include <PCI.h>
#include <CAM.h>
#include <ByteOrder.h>

/* shorthand byteswapping macros */
#define LE(n) B_HOST_TO_LENDIAN_INT32(n)
#define HE(n) B_LENDIAN_TO_HOST_INT32(n)

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <iovec.h>

#include "53c8xx.h"

#include "symbios.h"

#if USE_STATFS
#include <stat_module.h>

static int32
stat_controller(void *stats, char **buf)
{
	Symbios *s = (Symbios *) stats;

	if(*buf = (char *) malloc(256)){
		sprintf(*buf,
				"Chipset:   %s\n"
				"IO Base:   0x%08x\n"
				"SRAM Base: 0x%08x\n"
				"IRQ Line:  %d\n"
				"SCSI Bus:  %s\n",
				s->name, s->iobase, s->sram_phys, s->irq,
				s->max_targ_id > 7 ? "Wide" : "Narrow");
		return strlen(*buf);
	} else {
		return 0;
	}
}

static int32
stat_target(void *stats, char **buf)
{
	SymTarg *st = (SymTarg *) stats;

	if(st->flags & tf_ignore){
		*buf = NULL;
		return 0;
	}

	if(*buf = (char *) malloc(256)){
		if(st->offset){
			sprintf(*buf,
					"Width:     %s\n"
					"Transfer:  Sync\n"
					"Period:    %d ns\n"
					"Offset:    %d\n",
					st->wide ? "Wide" : "Narrow",
					st->period,
					st->offset
					);
		} else {
			sprintf(*buf,
					"Width:     %s\n"
					"Transfer:  Async\n",
					st->wide ? "Wide" : "Narrow");
		}
		return strlen(*buf);
	} else {
		return 0;
	}
}


static void register_stats(Symbios *s)
{
	char buf[128];
	stat_module_info_t *stats;
	int i;

	if(s->registered) return;

	if(get_module("generic/stat_module", (module_info**) &stats) == B_OK){
		sprintf(buf,"scsi/53c8xx/%d/info",s->num);
		stats->register_statistics(buf, s, stat_controller);
		for(i=0;i<=s->max_targ_id;i++){
			sprintf(buf,"scsi/53c8xx/%d/targ_%x",s->num,i);
			stats->register_statistics(buf, &(s->targ[i]), stat_target);
		}
		s->registered = 1;
	} else {
		dprintf("symbios: cannot find stats module...\n");
	}
}

#else
#define register_stats(x)

#endif

/*
** Constants for the SIM
*/
#define SIM_VERSION 0x01
#define HBA_VERSION 0x01

static char sim_vendor_name[]   = "Be, Inc.";
static char hba_vendor_name[]   = "Symbios";

static pci_module_info		*pci;
static cam_for_sim_module_info	*cam;

static char	cam_name[] = B_CAM_FOR_SIM_MODULE_NAME;
static char	pci_name[] = B_PCI_MODULE_NAME;

/*
** Supported Device / Device Attributes table
*/
#define symf_sram       0x0001      /* on board SCRIPTS ram */
#define symf_doubler    0x0002      /* SCLK doubler available */
#define symf_quadrupler 0x0004      /* SCLK quadrupler available */
#define symf_untested   0x1000      /* never actually tested one of these */
#define symf_wide       0x0008      /* supports WIDE bus */
#define symf_short      0x0010      /* short max period (8) */

static struct {
	uint32 id;
	uint32 rev;
	char *name;
	int flags;
} devinfo[] = {
	{ 0x0001, 0x10, "53c810a", symf_short },
	{ 0x0001, 0x00, "53c810",  symf_short },
	{ 0x0006, 0x00, "53c860",  symf_wide | symf_short | symf_untested },
	{ 0x0004, 0x00, "53c815",  symf_short | symf_untested },
	{ 0x0002, 0x00, "53c820",  symf_wide | symf_short | symf_untested },
	{ 0x0003, 0x10, "53c825a", symf_wide | symf_short |symf_sram | symf_untested },
	{ 0x0003, 0x00, "53c825",  symf_wide | symf_short | symf_untested },
	{ 0x000f, 0x02, "53c875",  symf_wide | symf_sram | symf_doubler },
	{ 0x000f, 0x00, "53c875",  symf_wide | symf_sram },
	{ 0x008f, 0x00, "53c875j", symf_wide | symf_sram | symf_doubler },
	{ 0x000d, 0x00, "53c885",  symf_wide | symf_sram | symf_untested },
	{ 0x000c, 0x00, "53c895",  symf_wide | symf_sram | symf_quadrupler | symf_untested },
	{ 0x000b, 0x00, "53c896",  symf_wide | symf_sram | symf_untested },
	{ 0, 0, NULL, 0 }
};

#include "scripts.c"

static void
setparams(SymTarg *t, uint period, uint offset, uint wide)
{
	Symbios *s = t->adapter;

	if(wide){
		if(!t->wide) kprintf("symbios%ld: target %ld wide\n",s->num,t->id);
		t->wide = 1;
	} else {
		t->wide = 0;
	}

	if(period){
		int i;
		for(i=0;i<s->syncsize;i++){
			if(period <= s->syncinfo[i].period){
				t->period = s->syncinfo[i].period;
				t->offset = offset;

				t->device[3] = s->syncinfo[i].scntl3;
				if(t->wide) t->device[3] |= 0x08;
				t->device[2] = t->id;
				t->device[1] = s->syncinfo[i].sxfer | (offset & 0x0f);
				t->device[0] = 0;
				kprintf("symbios%ld: target %ld sync period=%ld, offset=%d\n",
						s->num, t->id, t->period, offset);
				return;
			}
		}
	}

	t->period = 0;
	t->offset = 0;

	t->device[3] = s->scntl3; /* scntl3 - clock divisor */
	if(t->wide) t->device[3] |= 0x08;
	t->device[2] = t->id;     /* dest id */
	t->device[1] = 0;         /* sync xfer */
	t->device[0] = 0;         /* reserved */
}

static long init_symbios(Symbios *s, int restarting);

/*
** IO Macros
*/

/* XXX - fix me bjs */
#define inb(p)     (*pci->read_io_8)(s->iobase + p)
#define outb(p,v)  (*pci->write_io_8)(s->iobase + p,v)
#define inw(p)     (*pci->read_io_16)(s->iobase + p)
#define outw(p,v)  (*pci->write_io_16)(s->iobase + p,v)
#define in32(p)    (*pci->read_io_32)(s->iobase + p)
#define out32(p,v) (*pci->write_io_32)(s->iobase + p,v)


/* patch in an external symbol */
#define RESOLV(sname,value) \
{ int i; \
	d_printf("symbios%d: relocting %d instances of %s to 0x%08x\n", \
			 s->num,sizeof(E_##sname##_Used)/4,#sname,value); \
	for(i=0;i<(sizeof(E_##sname##_Used)/4);i++) { \
		scr[E_##sname##_Used[i]] = ((uint32) value); \
	} \
}

/* calc phys addr of a ptr inside the sram area */
#define PHADDR(lvar) ((s->sram_phys) + (((uint32) &(lvar)) - ((uint32) s->script)))

/* calc phys addr of a ptr inside the priv area */
/*#define PPHADDR(ptr) (st->priv_phys + (((uint32) ptr) - ((uint32) st->priv)))*/
#define PPHADDR(ptr) (phys + (((uint32) ptr) - ((uint32) sp)))

/* prepare a Targ's scripts indirect table for one or more exec_io's */
static void prep_io(SymPriv *sp, uint32 phys)
{
	sp->syncmsg.address = LE(PPHADDR(sp->_syncmsg));
	sp->syncmsg.count = LE(3);

	sp->widemsg.address = LE(PPHADDR(sp->_widemsg));
	sp->widemsg.count = LE(2);

	sp->sendmsg.address = LE(PPHADDR(sp->_sendmsg));

	sp->recvmsg.address = LE(PPHADDR(sp->_recvmsg));
	sp->recvmsg.count = LE(1);

	sp->status.address = LE(PPHADDR(sp->_status));
	sp->status.count = LE(1);

	sp->extdmsg.address = LE(PPHADDR(sp->_extdmsg));
	sp->extdmsg.count = LE(1);

	sp->command.address = LE(PPHADDR(sp->_command));
}

/*
 * actually execute an io transaction via SCRIPTS
 * you MUST hold st->sem_targ before calling this
 *
 */
static void exec_io(SymTarg *st, void *cmd, int cmdlen, void *msg, int msglen,
				   void *data, int datalen, int sg)
{
	cpu_status former;
	Symbios *s = st->adapter;

	memcpy((void *) &(st->priv->device.count), st->device, 4);

	st->priv->sendmsg.count = LE(msglen);
	memcpy(st->priv->_sendmsg, msg, msglen);
	st->priv->command.count = LE(cmdlen);
	memcpy(st->priv->_command, cmd, cmdlen);

	st->table_phys = st->priv_phys + ADJUST_PRIV_TO_TABLE;

	if(datalen){
		int i,sgcount;
		uint32 opcode;
		SymInd *t = st->priv->table;
		physical_entry *pe = (physical_entry *) &(st->priv->table[1]);

		if(st->inbound){
			opcode = s->op_in;
			st->datain_phys = st->table_phys;
			st->dataout_phys = s->sram_phys + Ent_phase_dataerr;
		} else {
			opcode = s->op_out;
			st->dataout_phys = st->table_phys;
			st->datain_phys = s->sram_phys + Ent_phase_dataerr;
		}

		if(sg) {
			iovec *vec = (iovec *) data;
			for(sgcount=0,i=0;i<datalen;i++){
				get_memory_map(vec[i].iov_base, vec[i].iov_len, &pe[sgcount], 130-sgcount);
				while(pe[sgcount].size && (sgcount < 130)){
					t[sgcount].address = LE((uint32) pe[sgcount].address);
					t[sgcount].count = LE(opcode | pe[sgcount].size);
					sgcount++;
				}
				if((sgcount == 130) && pe[sgcount].size){
					panic("symbios: sg list overrun");
				}
			}
		} else {
			get_memory_map(data, datalen, pe, 130);
			for(i=0;pe[i].size;i++){
				t[i].address = LE((uint32) pe[i].address);
				t[i].count = LE(opcode | pe[i].size);
			}
			sgcount = i;
		}
		t[sgcount].count = LE(OP_END);
		t[sgcount].address = LE(ARG_END);

//		for(i=0;i<=sgcount;i++){
//			dprintf("sym: %04d - %08x %08x\n",i,t[i].address,t[i].count);
//		}
	} else {
		st->datain_phys = s->sram_phys + Ent_phase_dataerr;
		st->dataout_phys = s->sram_phys + Ent_phase_dataerr;
	}

//	dprintf("sym: pp = %08x  di = %08x  do = %08x\n",st->priv_phys,st->datain_phys,st->dataout_phys);

	st->status = status_queued;

/*	dprintf("symbios: enqueueing %02x %02x %02x ... for %d (%d bytes %s)\n",
			((uchar *)cmd)[0],((uchar *)cmd)[1],((uchar *)cmd)[2],
			st->device[2],datalen,st->inbound?"IN":"OUT");
*/
	former = disable_interrupts();
	acquire_spinlock(&(s->hwlock));

	/* enqueue the request */
	if(s->startqueuetail){
		s->startqueuetail->next = st;
	} else {
		s->startqueue = st;
	}
	st->next = NULL;
	s->startqueuetail = st;

	/* If the adapter is idle, signal it so that this request may be started */
	if(s->status == IDLE) outb(sym_istat, sym_istat_sigp);

	release_spinlock(&(s->hwlock));
	restore_interrupts(former);

	/* wait for completion */
	acquire_sem(st->sem_done);

#if 0
	if(acquire_sem_etc(st->sem_done, 1, B_TIMEOUT, 10*1000000) != B_OK){
		kprintf("sym: targ %d never finished,  argh...\n",st->device[2]);
		init_symbios(st->adapter,1);
		st->state = sTIMEOUT;
		return;
	}
#endif
}


#if DEBUG_ISR
#define kp kprintf
#else
#define kp(x...)
#endif

static int32
scsi_int_dispatch(void *data)
{
	Symbios *s = (Symbios *) data;
	int reselected = 0;
	uchar istat;

	if(s->reset) return B_UNHANDLED_INTERRUPT;

	acquire_spinlock(&(s->hwlock));
	istat = inb(sym_istat);

	if(istat & sym_istat_dip){
		uchar dstat = inb(sym_dstat);

		if(dstat & sym_dstat_sir){
			/* Handle and interrupt from the SCRIPTS program */
		 	uint32 status = HE(in32(sym_dsps));
	//		kprintf("<%02x>",status);

			switch(status){
			case status_ready:
				kp("sym: ready\n");
				break;

			case status_iocomplete:
				kp("sym: done %08x\n",s->active);
				if(s->active){
					/* io is complete -- any more io is an error */
					s->active->datain_phys = s->sram_phys + Ent_phase_dataerr;
					s->active->dataout_phys = s->sram_phys + Ent_phase_dataerr;
				}
				break;

			case status_reselected:{
				uint32 id = inb(sym_ssid);
				if(id & sym_ssid_val) {
					s->active = &(s->targ[id & s->idmask]);
					kp("sym: resel %08x\n",s->active);
					if(s->active->status != status_waiting){
						s->active = NULL;
						kprintf("symbios: bad reselect %ld\n",id & sym_ssid_encid);
					} else {
						reselected = 1;
					}
				} else {
					kprintf("symbios: invalid reselection!?\n");
				}
				break;
			}

			case status_timeout:
				/* inform the unlucky party and dequeue it */
				kp("sym: timeout %08lx\n",s->startqueue);
				if(s->startqueue){
					s->startqueue->status = status_timeout;
					release_sem_etc(s->startqueue->sem_done, 1, B_DO_NOT_RESCHEDULE);
					if(!(s->startqueue = s->startqueue->next)){
						s->startqueuetail = NULL;
					}
				}
				break;

			case status_selected:
				/* selection succeeded.  Remove from start queue and make active */
				kp("sym: selected %08lx\n",s->startqueue);
				if(s->startqueue){
					s->active = s->startqueue;
					s->active->status = status_active;
					if(!(s->startqueue = s->startqueue->next)){
						s->startqueuetail = NULL;
					}
				}
				break;

			case status_syncin:
				setparams(s->active,
						  s->active->priv->_syncmsg[1]*4,
						  s->active->priv->_syncmsg[2],
						  s->active->wide);
				break;

			case status_widein:
				setparams(s->active, s->active->period, s->active->offset,
						  s->active->priv->_widemsg[1]);
				break;

			case status_ignore_residue:
				kprintf("ignore residue 0x%02x\n",s->active->priv->_extdmsg[0]);
				break;

			case status_disconnect:
				kp("sym: disc %08lx\n",s->active);
				/* device disconnected. make inactive */
				if(s->active){
					s->active->status = status_waiting;
					s->active = NULL;
				}
				break;

			case status_badmsg:
				kp("sym: badmsg %02x\n",s->active->priv->_recvmsg[0]);

			case status_complete:
			case status_badstatus:
			case status_overrun:
			case status_underrun:
			case status_badphase:
			case status_badextmsg:
				kp("sym: error %08lx / %02x\n",s->active,status);
				/* transaction completed successfully or in error. report our status. */
				if(s->active){
					s->active->status = status;
					release_sem_etc(s->active->sem_done, 1, B_DO_NOT_RESCHEDULE);
					s->active = NULL;
				}
				break;

			case status_selftest:
				/* signal a response to the selftest ... don't actually start up the
				   SCRIPTS like we normally do */
				s->status = OFFLINE;
				goto done;
				break;

			default:
				kp("sym: int 0x%08lx ...\n",status);
			}
			goto reschedule;
		} else {
			kprintf("symbios: weird error, dstat = %02x\n",dstat);
		}
	}

	if(istat & sym_istat_sip){
		uchar sist0;
		sist0 = inb(sym_sist1);

		if(sist0 & sym_sist1_sbmc){
			kprintf("sym: SBMC %02x!\n",inb(sym_stest4) & 0xc0);
		}

		if(sist0 & sym_sist1_sto){
			/* select timeout */
			kp("sym: Timeout %08lx\n",s->startqueue);
			/* inform the unlucky party and dequeue it */
			if(s->startqueue){
				s->startqueue->status = status_timeout;
				release_sem_etc(s->startqueue->sem_done, 1, B_DO_NOT_RESCHEDULE);
				if(!(s->startqueue = s->startqueue->next)){
					s->startqueuetail = NULL;
				}
			} else {
				kprintf("symbios: ghost target timed out\n");
			}
			inb(sym_sist0);	//   apparently we MUST read sist0 as well
			goto reschedule;
		}

		sist0 = inb(sym_sist0) & 0x8f;

		if(sist0 && s->active){
			if(sist0 & sym_sist0_ma){
				/* phase mismatch -- we experienced a disconnect while in
				   a DataIn or DataOut... gotta figure out how much we
				   transferred, update the sgtable, take the FIFOs into
				   account, etc (see 9-9 in Symbios PCI-SCSI Programming Guide) */
				SymInd *t;
				uint32 dfifo_val, bytesleft, dbc;
				uint32 dsp = HE(in32(sym_dsp));
				uint32 n = (dsp - s->active->table_phys) / 8 - 1;

				if((dsp < s->active->priv_phys) || (n > 129)) {
					/* we mismatched during some other phase ?! */
					kprintf("Phase Mismatch (dsp = 0x%08lx)\n",dsp);
					goto reschedule;
				}

				t = &(s->active->priv->table[n]);

#if 0
				t->count = HE(t->count);
				t->address = HE(t->count);
#endif
				/* dbc initially = table[n].count, counts down */
				dbc = (uint32) HE(in32(sym_dbc)) & 0x00ffffffL;

				t->count &= 0xffffff;
#if DEBUG_PM
				kprintf("PM(%s) dbc=0x%08x, n=%02d, a=0x%08x, l=0x%08x\n",
						s->active->inbound ? " in" : "out", dbc, n, t->address, t->count);
#endif
				if(s->active->inbound){
					/* data in is easy... flush happens automatically */
					t->address += t->count - dbc;
					t->count = dbc;
#if DEBUG_PM
					kprintf("                              a=0x%08x, l=0x%08x\n",
					t->address, t->count);
#endif
					s->active->datain_phys = s->active->table_phys + 8*(t->count ? n : n+1);
					t->count |= s->op_in;
#if 0
					t->count = LE(t->count);
					t->address = LE(t->address);
#endif
					goto reschedule;
				} else {
					if(inb(sym_ctest5) & 0x20){
						/* wide FIFO */
						dfifo_val = ((inb(sym_ctest5) & 0x03) << 8) | inb(sym_dfifo);
						bytesleft = (dfifo_val - (dbc & 0x3ff)) & 0x3ff;
					} else {
						dfifo_val = (inb(sym_dfifo) & 0x7f);
						bytesleft = (dfifo_val - (dbc & 0x7f)) & 0x7f;
					}
					if(inb(sym_sstat0) & 0x20) bytesleft++;
					if(inb(sym_sstat2) & 0x20) bytesleft++;
					if(inb(sym_sstat0) & 0x40) bytesleft++;
					if(inb(sym_sstat2) & 0x40) bytesleft++;

					/* clear fifo */
					outb(sym_ctest3, 0x04);

					t->address += t->count - dbc;
					t->count = dbc;

					/* adjust for data that didn't make it to the target */
					t->address -= bytesleft;
					t->count += bytesleft;
#if DEBUG_PM
					kprintf("                              a=0x%08x, l=0x%08x\n",
					t->address, t->count);
#endif
					s->active->dataout_phys = s->active->table_phys + 8*(t->count ? n : n+1);
					t->count |= s->op_out;
#if 0
					t->count = LE(t->count);
					t->address = LE(t->address);
#endif
					spin(10);
					goto reschedule;
				}
			}

			if(sist0 & sym_sist0_udc){
				kprintf("symbios: Unexpected Disconnect (dsp = 0x%08lx)\n", in32(sym_dsp));
			}

			if(sist0 & sym_sist0_sge){
				kprintf("symbios: SCSI Gross Error\n");
			}

			if(sist0 & sym_sist0_rst){
				kprintf("symbios: SCSI Reset\n");
			}

			if(sist0 & sym_sist0_par){
				kprintf("symbios: Parity Error\n");
			}

			s->active->status = status_badphase;
			release_sem_etc(s->active->sem_done, 1, B_DO_NOT_RESCHEDULE);
			s->active = NULL;

			goto reschedule;
		}
	} else {
		/* nothing happened... must be somebody else's problem */
		release_spinlock(&(s->hwlock));
		 return B_UNHANDLED_INTERRUPT;
	}

reschedule:
	/* start the SCRIPTS processor at one of three places, depending on state
	**
	** 1. If there is an active transaction, insure that the script is patched
	**    correctly, the DSA is loaded, and start up at "switch".
	**
	** 2. If there is a transaction at the head of the startqueue, set the DSA
	**    and start up at "start" to try to select the target and start the
	**    transaction
	**
	** 3. If there is nothing else to do, go to "idle" and wait for signal or
	**    reselection
	*/

	if(s->active){
		out32(sym_dsa, s->active->priv_phys + ADJUST_PRIV_TO_DSA);
		s->script[PATCH_DATAIN] = LE(s->active->datain_phys);
		s->script[PATCH_DATAOUT] = LE(s->active->dataout_phys);

		s->active->status = status_active;
		s->status = ACTIVE;
		if(reselected){
			out32(sym_dsp, LE(s->sram_phys + Ent_switch_resel));
			//kp("sym: restart @ %08x / reselected\n",Ent_switch_resel);
		} else {
			out32(sym_dsp, LE(s->sram_phys + Ent_switch));
			//kp("sym: restart @ %08x / selected\n", Ent_switch);
		}
	} else {
		if(s->startqueue){
			out32(sym_dsa, LE(s->startqueue->priv_phys + ADJUST_PRIV_TO_DSA));

			s->startqueue->status = status_selecting;
			s->status = START;
			out32(sym_dsp, LE(s->sram_phys + Ent_start));
			//kp("sym: restart @ %08x / started\n", Ent_start);
		} else {
			s->status = IDLE;
			out32(sym_dsp, LE(s->sram_phys + Ent_idle));
			//kp("sym: restart @ %08x / idle\n", Ent_idle);
		}
	}

done:
	release_spinlock(&(s->hwlock));
    return B_HANDLED_INTERRUPT;
}


/* init the adapter... if restarting, no bus reset or whathaveyou */
static long init_symbios(Symbios *s, int restarting)
{
    d_printf("symbios%ld: init_symbios()\n",s->num);

	if(restarting){
		s->reset = 1;
	} else {
		s->reset = 0;
	}

	if(!restarting){
		/* reset the SCSI bus */
		dprintf("symbios%ld: scsi bus reset\n",s->num);
		outb(sym_scntl1, sym_scntl1_rst);
		spin(25);
		outb(sym_scntl1, 0);
		spin(250000);

		/* clear ints */
		inb(sym_istat);
		inb(sym_sist0);
		inb(sym_sist1);
		inb(sym_dstat);

		install_io_interrupt_handler(s->irq, scsi_int_dispatch, s, 0);
		d_printf("symbios%ld: registered interrupt handler for irq %ld\n",s->num,s->irq);
	}

	/* enable irqs, prefetch, no 53c700 compat */
	outb(sym_dcntl, sym_dcntl_com);

	/* enable all DMA ints */
	outb(sym_dien, sym_dien_sir | sym_dien_mdpe | sym_dien_bf | sym_dien_abrt
		 | sym_dien_iid);
	/* enable all fatal SCSI ints */
	outb(sym_sien0, sym_sien0_ma | sym_sien0_sge | sym_sien0_udc | sym_sien0_rst |
		 sym_sien0_par);
	outb(sym_sien1, sym_sien1_sto | sym_sien1_sbmc); // XXX

	/* sel / hth timeouts */
	outb(sym_stime0, 0xbb);

	/* clear ints */
	inb(sym_istat);
	inb(sym_sist0);
	inb(sym_sist1);
	inb(sym_dstat);

	/* clear ints */
	inb(sym_sist0);
	inb(sym_sist1);
	inb(sym_dstat);

	if(restarting){
		s->reset = 0;
	} else {
		int i;
		s->status = TEST;

		dprintf("symbios%ld: selftest ",s->num);
		out32(sym_dsp, LE(s->sram_phys + Ent_test));
		for(i=0;(s->status == TEST) && i<10;i++) {
			dprintf(".");
			spin(10000);
		}
		if(s->status == TEST){
			dprintf("FAIL\n");
			return B_ERROR; //XXX teardown
		} else {
			dprintf("PASS\n");
		}
	}

	s->status = IDLE;
	out32(sym_dsp, LE(s->sram_phys + Ent_idle));
    d_printf("symbios%ld: started script\n",s->num);

    return B_NO_ERROR;
}

/* When an inquiry succeeds the negotiator gets the option to attempt to
** request a better transfer agreement with the target.
*/
static void negotiator(Symbios *s, SymTarg *targ, uchar *ident, uchar *msg)
{
	if(ident[7] & 0x20){ /* wide supported */
		if(targ->flags & tf_ask_wide){
			uchar cmd[6] = { 0, 0, 0, 0, 0, 0 };
			targ->flags &= (~tf_ask_wide); /* only ask once */

			dprintf("symbios%ld: negotiating wide xfer with target %ld\n",
			s->num,targ->id);

			msg[1] = 0x01; /* extended message */
			msg[2] = 0x02; /* length           */
			msg[3] = 0x03; /* sync negotiate   */
			msg[4] = 0x01; /* 16 bit wide      */

			exec_io(targ, cmd, 6, msg, 5, NULL, 0, 0);
		}
	}

	if(ident[7] & 0x10){ /* sync supported */
		if(targ->flags & tf_ask_sync) {
			uchar cmd[6] = { 0, 0, 0, 0, 0, 0 };

			targ->flags &= (~tf_ask_sync); /* only ask once */

			dprintf("symbios%ld: negotiating sync xfer with target %ld\n",
			s->num,targ->id);

			msg[1] = 0x01; /* extended message */
			msg[2] = 0x03; /* length           */
			msg[3] = 0x01; /* sync negotiate   */
			msg[4] = s->syncinfo[0].period / 4; /* sync period / 4  */
			msg[5] = s->maxoffset;

			exec_io(targ, cmd, 6, msg, 6, NULL, 0, 0);
		}
	}
}

/* Convert a CCB_SCSIIO into a BL_CCB32 and (possibly SG array).
**
*/
static long sim_execute_scsi_io(Symbios *s, CCB_HEADER *ccbh)
{
	CCB_SCSIIO *ccb = (CCB_SCSIIO *) ccbh;
	uchar *cdb;
	physical_entry pe[2];
	SymTarg *targ;
	uchar msg[8];

	targ = s->targ + ccb->cam_ch.cam_target_id;

	if(targ->flags & tf_ignore){
		ccbh->cam_status = CAM_SEL_TIMEOUT;
		return B_OK;
	}

	if(ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
		cdb = ccb->cam_cdb_io.cam_cdb_ptr;
	} else {
		cdb = ccb->cam_cdb_io.cam_cdb_bytes;
	}

	get_memory_map((void*) (ccb->cam_sim_priv), 1536, pe, 2);

	/* identify message */
	msg[0] = 0xC0 | (ccb->cam_ch.cam_target_lun & 0x07);

	/* fill out table */
	prep_io((SymPriv *) ccb->cam_sim_priv, (uint32) pe[0].address);

	/* insure only one transaction at a time for any given target */
	acquire_sem(targ->sem_targ);

	targ->priv = (SymPriv *) ccb->cam_sim_priv;;
	targ->priv_phys = (uint32 ) pe[0].address;

	targ->inbound = (ccb->cam_ch.cam_flags & CAM_DIR_IN) ? 1 : 0;

	if(ccb->cam_ch.cam_flags & CAM_SCATTER_VALID){
		exec_io(targ, cdb, ccb->cam_cdb_len, msg, 1,
				ccb->cam_data_ptr, ccb->cam_sglist_cnt, 1);
	} else {
		exec_io(targ, cdb, ccb->cam_cdb_len, msg, 1,
				ccb->cam_data_ptr, ccb->cam_dxfer_len, 0);
	}

/*	dprintf("symbios%d: state = 0x%02x, status = 0x%02x\n",
			s->num,targ->state,targ->priv->status[0]);*/

	/* decode status */
	switch(targ->status){
	case status_complete:
		if((ccb->cam_scsi_status=targ->priv->_status[0]) != 0) {
			ccbh->cam_status = CAM_REQ_CMP_ERR;

			/* nonzero status is an error ... 0x02 = check condition */
			if((ccb->cam_scsi_status == 0x02) &&
			   !(ccb->cam_ch.cam_flags & CAM_DIS_AUTOSENSE) &&
			   ccb->cam_sense_ptr && ccb->cam_sense_len){
				   uchar command[6];

				   command[0] = 0x03;		/* request_sense */
				   command[1] = ccb->cam_ch.cam_target_lun << 5;
				   command[2] = 0;
				   command[3] = 0;
				   command[4] = ccb->cam_sense_len;
				   command[5] = 0;

				   targ->inbound = 1;
				   exec_io(targ, command, 6, msg, 1,
						   ccb->cam_sense_ptr, ccb->cam_sense_len, 0);

				   if(targ->priv->_status[0]){
					   ccb->cam_ch.cam_status |= CAM_AUTOSENSE_FAIL;
				   } else {
					   ccb->cam_ch.cam_status |= CAM_AUTOSNS_VALID;
				   }
			}
		} else {
			ccbh->cam_status = CAM_REQ_CMP;

			if(cdb[0] == 0x12) {
				/* inquiry just succeeded ... is it non SG and with enough data to
				   snoop the support bits? */
				if(!(ccb->cam_ch.cam_flags & CAM_SCATTER_VALID) && (ccb->cam_dxfer_len>7)){
					negotiator(s, targ, ccb->cam_data_ptr, msg);
				}
			}
		}
		break;

	case status_timeout:
		ccbh->cam_status = CAM_SEL_TIMEOUT;
		break;

	default: // XXX
		ccbh->cam_status = CAM_SEL_TIMEOUT;
	}

	targ->status = status_inactive;
//	dprintf("symbios%d: releasing targ @ 0x%08x\n",s->num,targ);
	release_sem(targ->sem_targ);
	return B_OK;
}

/*
** sim_path_inquiry returns info on the target/lun.
*/
static long sim_path_inquiry(Symbios *s, CCB_HEADER *ccbh)
{
    CCB_PATHINQ	*ccb;
    ccb = (CCB_PATHINQ *) ccbh;
    ccb->cam_version_num = SIM_VERSION;
    ccb->cam_target_sprt = 0;
    ccb->cam_hba_eng_cnt = 0;
    memset (ccb->cam_vuhba_flags, 0, VUHBA);
    ccb->cam_sim_priv = SIM_PRIV;
    ccb->cam_async_flags = 0;
    ccb->cam_initiator_id = s->host_targ_id;
	ccb->cam_hba_inquiry = s->max_targ_id > 7 ? PI_WIDE_16 : 0 ;
    strncpy (ccb->cam_sim_vid, sim_vendor_name, SIM_ID);
    strncpy (ccb->cam_hba_vid, hba_vendor_name, HBA_ID);
    ccb->cam_osd_usage = 0;
    ccbh->cam_status = CAM_REQ_CMP;
	register_stats(s);
    return 0;
}


/*
** sim_extended_path_inquiry returns info on the target/lun.
*/
static long sim_extended_path_inquiry(Symbios *s, CCB_HEADER *ccbh)
{
    CCB_EXTENDED_PATHINQ *ccb;

    sim_path_inquiry(s, ccbh);
    ccb = (CCB_EXTENDED_PATHINQ *) ccbh;
    sprintf(ccb->cam_sim_version, "%d.0", SIM_VERSION);
    sprintf(ccb->cam_hba_version, "%d.0", HBA_VERSION);
    strncpy(ccb->cam_controller_family, "Symbios", FAM_ID);
    strncpy(ccb->cam_controller_type, s->name, TYPE_ID);
    return 0;
}

/*
** scsi_sim_action performes the scsi i/o command embedded in the
** passed ccb.
**
** The target/lun ids are assumed to be in range.
*/
static long sim_action(Symbios *s, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INPROG;
	switch(ccbh->cam_func_code){
	case XPT_SCSI_IO:
		return sim_execute_scsi_io(s,ccbh);
	case XPT_PATH_INQ:
		return sim_path_inquiry(s,ccbh);
	case XPT_EXTENDED_PATH_INQ:
		return sim_extended_path_inquiry(s, ccbh);
	default:
		ccbh->cam_status = CAM_REQ_INVALID;
		return -1;
	}
}

static void reloc_script(Symbios *s)
{
	int i;
	ulong *scr = s->script;

	memcpy(scr, SCRIPT, sizeof(SCRIPT));
	for(i=0;i<PATCHES;i++){
		scr[LABELPATCHES[i]] += s->sram_phys;
	}
	d_printf("symbios%ld: loaded %ld byte SCRIPT, relocated %ld labels\n",
			 s->num, sizeof(SCRIPT), PATCHES);

		/* disable scsi ints */
	outb(sym_scratcha, 0x42);
	outb(sym_scratcha+1, 0x00);
	outb(sym_scratchb, 0x04);
	outb(sym_sien0, 0);
	outb(sym_sien1, 0);
	outb(sym_dien, sym_dien_sir);

	/* clear ints */
	inb(sym_sist0);
	inb(sym_sist1);
	inb(sym_dstat);

	outb(sym_dmode, ( sym_dmode_diom | sym_dmode_siom, 0 ));	/* FIXME: ??? */
}

static uint32 sym_readclock(Symbios *s)
{
	uint32 ms,a,i;
	bigtime_t t0,t1;

	outw(sym_sien0 , 0);    /* mask all scsi interrupts        */
	outb(sym_dien , 0);     /* mask all dma interrupts         */
	inw(sym_sist0);         /* clear pending scsi interrupts   */
	outb(sym_scntl3, 4);    /* set pre-scaler to divide by 3   */

	for(a=0,i=0;i<5;i++){
		ms = 0;
		outb(sym_stime1, 0);    /* disable general purpose timer   */
		spin(10000);            /* let it all settle for 10ms      */
		inw(sym_sist0);         /* another one, just to be sure :) */

		t0 = system_time();
		outb(sym_stime1, 11);   /* delay of 128ms */
		while (!(inb(sym_sist1) & sym_sist1_gen)) snooze(250);
		t1 = system_time();
		ms = (t1-t0)/1000 + 10; /* we seem to be off by 10ms typically */

		a += ((1 << 11) * 4400) / ms;
	}

	outb(sym_stime1, 0);    /* disable general purpose timer   */
	return a / 5;
}

static uchar id_bits[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/*
** Allocate the actual memory for the cardinfo object
*/
static Symbios *create_cardinfo(int num, pci_info *pi, int flags)
{
	char name[32];
	Symbios *s;
	int i,scf;
	area_id aid;
	uint32 stest2,stest4;

	if((pi->u.h0.interrupt_line == 0) || (pi->u.h0.interrupt_line > 128)) {
		return NULL; /* invalid IRQ */
	}

	if(!(s = (Symbios *) malloc(sizeof(Symbios)))) return NULL;

	s->num = num;
	s->iobase = pi->u.h0.base_registers[0];
	s->irq = pi->u.h0.interrupt_line;
	s->hwlock = 0;
	s->startqueue = NULL;
	s->startqueuetail = NULL;
	s->active = NULL;

	sprintf(name,"sym%d:sram",num);
	if(flags & symf_sram){
		unsigned char *c;
		s->sram_phys = pi->u.h0.base_registers[2];
		if((aid=map_physical_memory(name, s->sram_phys, 4096,
									 B_ANY_KERNEL_ADDRESS, B_READ_AREA + B_WRITE_AREA,
									 (void **) &(s->script))) < 0){
			free(s);
			return NULL;
		}
		/* memory io test */
		c = (unsigned char *) s->script;
		for(i=0;i<4096;i++) c[i] = (255 - (i & 0xff));
		for(i=0;i<4096;i++) {
			if(c[i] != (255 - (i & 0xff))) {
				d_printf("symbios%d: scripts ram io error @ %d\n",num,i);
				goto err;
			}
		}
	} else {
		uchar *a;
		physical_entry entries[2];
		aid = create_area(name, (void **)&a, B_ANY_KERNEL_ADDRESS, 4096*5,
			B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
		if(aid == B_ERROR || aid == B_BAD_VALUE || aid == B_NO_MEMORY){
			free(s);
		    return NULL;
		}
		get_memory_map(a, 4096, entries, 2);
		s->sram_phys = (uint32) entries[0].address;
		s->script = (uint32 *) a;
	}

	d_printf("symbios%d: scripts ram @ 0x%08lx, mapped to 0x%08lx (%s)\n",
			 num, s->sram_phys, (uint32) s->script,
			 flags & symf_sram ? "onboard" : "offboard" );

	/* what are we set at now? */
	s->host_targ_id = inb(sym_scid) & 0x07;
	dprintf("symbios%ld: host id %ld\n",s->num,s->host_targ_id);

	s->host_targ_id = 7;  /* XXX figure this out somehow... */
	s->max_targ_id = (flags & symf_wide) ? 15 : 7;

	stest2 = inb(sym_stest2);
	stest4 = inb(sym_stest4);

	/* software reset */
	outb(sym_istat, sym_istat_srst);
	spin(10000);
	outb(sym_istat, 0);
	spin(10000);

	/* initiator mode, full arbitration */
	outb(sym_scntl0, sym_scntl0_arb0 | sym_scntl0_arb1);

	outb(sym_scntl1, 0);
	outb(sym_scntl2, 0);

	/* initiator id=7, respond to reselection */
	/* respond to reselect of id 7 */
	outb(sym_respid, id_bits[s->host_targ_id]);
	outb(sym_scid, sym_scid_rre | s->host_targ_id);

	outb(sym_dmode, 0);

	dprintf("symbios%ld: stest2 = 0x%02lx, stest4 = 0x%02lx\n",s->num,stest2,stest4);

	/* no differential, no loopback, no hiz, no always-wide, no filter, no lowlevel */
	outb(sym_stest2, 0); // save diff bit
	outb(sym_stest3, 0);

//	if(flags & symf_quadrupler){
//		outb(sym_stest4, sym_stest4_lvd);
//	}

	outb(sym_stest1, 0);    /* make sure clock doubler is OFF  */

	s->sclk = sym_readclock(s);
	dprintf("symbios%ld: clock is %ldKHz\n",s->num,s->sclk);

	if(flags & symf_doubler){
		/* if we have a doubler and we don't already have an 80MHz clock */
		if((s->sclk > 35000) && (s->sclk < 45000)){
			dprintf("symbios%ld: enabling clock doubler...\n",s->num);
			outb(sym_stest1, 0x08);  /* enable doubler */
			spin(200);                /* wait 20us      */
			outb(sym_stest3, 0xa0);  /* halt sclk, enable TolerANT*/
			outb(sym_scntl3, 0x05);  /* SCLK/4         */
			outb(sym_stest1, 0x0c);  /* engage doubler */
			outb(sym_stest3, 0x80);  /* reenable sclk, leave TolerANT on  */

			spin(3000);

			s->sclk = sym_readclock(s);
			dprintf("symbios%ld: clock is %ldKHz\n",s->num,s->sclk);
		}
	}
	if(flags & symf_quadrupler){
		if((s->sclk > 35000) && (s->sclk < 45000)){
			dprintf("symbios%ld: enabling clock quadrupler...\n",s->num);
			outb(sym_stest1, 0x08);  /* enable doubler */
			spin(200);                /* wait 20us      */
			outb(sym_stest3, 0xa0);  /* halt sclk, enable TolerANT*/
			outb(sym_scntl3, 0x05);  /* SCLK/4         */
			outb(sym_stest1, 0x0c);  /* engage doubler */
			outb(sym_stest3, 0x80);  /* reenable sclk, leave TolerANT on  */

			spin(3000);

			s->sclk = sym_readclock(s);
			dprintf("symbios%ld: clock is %ldKHz\n",s->num,s->sclk);
			s->sclk = 160000;
		}
	}
	outb(sym_stest3, 0x80);  /* leave TolerANT on  */

	scf = 0;
	/* set CCF / SCF according to specs */
	if(s->sclk < 25010) {
		dprintf("symbios%ld: unsupported clock frequency\n",s->num);
		goto err;  //		s->scntl3 = 0x01;
	} else if(s->sclk < 37510){
		dprintf("symbios%ld: unsupported clock frequency\n",s->num);
		goto err;  //		s->scntl3 = 0x02;
	} else if(s->sclk < 50010){
		/* 40MHz - divide by 1, 2 */
		scf = 0x10;
		s->scntl3 = 0x03;
	} else if(s->sclk < 75010){
		dprintf("symbios%ld: unsupported clock frequency\n",s->num);
		goto err; //		s->scntl3 = 0x04;
	} else if(s->sclk < 85000){
		/* 80 MHz - divide by 2, 4*/
		scf = 0x30;
		s->scntl3 = 0x05;
	} else {
		/* 160 MHz - divide by 4, 8 */
		scf = 0x50;
		s->scntl3 = 0x07;
	}


	s->maxoffset = (flags & symf_short) ? 8 : 15 ;
	s->syncsize = 0;

	if(scf == 0x50){
		/* calculate values for 160MHz clock */
		for(i=0;i<4;i++){
			s->syncinfo[s->syncsize].sxfer = i << 5;
			s->syncinfo[s->syncsize].scntl3 = s->scntl3 | 0x90; /* /2, Ultra2 */
			s->syncinfo[s->syncsize].period_ns = (625 * (i+4)) / 100;
			s->syncinfo[s->syncsize].period = 4 * (s->syncinfo[s->syncsize].period_ns / 4);
			s->syncsize++;
		}
	}

	if(scf >= 0x30){
		/* calculate values for 80MHz clock */
		for(i=0;i<4;i++){
			s->syncinfo[s->syncsize].sxfer = i << 5;
			if(scf == 0x30){
				s->syncinfo[s->syncsize].scntl3 = s->scntl3 | 0x90; /* /2, Ultra */
			} else {
				s->syncinfo[s->syncsize].scntl3 = s->scntl3 | 0xb0; /* /4, Ultra2 */
			}

			s->syncinfo[s->syncsize].period_ns = (125 * (i+4)) / 10;
			s->syncinfo[s->syncsize].period = 4 * (s->syncinfo[s->syncsize].period_ns / 4);
			s->syncsize++;
		}
	}

	/* calculate values for 40MHz clock */
	for(i=0;i<8;i++){
		s->syncinfo[s->syncsize].sxfer = i << 5;
		s->syncinfo[s->syncsize].scntl3 = s->scntl3 | scf;
		s->syncinfo[s->syncsize].period_ns = 25 * (i+4);
		s->syncinfo[s->syncsize].period = 4 * (s->syncinfo[s->syncsize].period_ns / 4);
		s->syncsize++;
	}

	for(i=0;i<s->syncsize;i++){
		dprintf("symbios%ld: syncinfo[%d] = { %02x, %02x, %d ns, %d ns }\n",
				s->num, i,
				s->syncinfo[i].sxfer, s->syncinfo[i].scntl3,
				s->syncinfo[i].period_ns, s->syncinfo[i].period);
	}

	for(i=0;i<16;i++){
		s->targ[i].id = i;
		s->targ[i].adapter = s;
		s->targ[i].wide = 0;
		s->targ[i].offset = 0;
		s->targ[i].status = status_inactive;

		if((i == s->host_targ_id) || (i > s->max_targ_id)){
			s->targ[i].flags = tf_ignore;
		} else {
			s->targ[i].flags = tf_ask_sync;
			if(flags & symf_wide) s->targ[i].flags |= tf_ask_wide;
//			s->targ[i].flags = 0;

			setparams(s->targ + i, 0, 0, 0);

			sprintf(name,"sym%ld:%02d:lock",s->num,i);
			s->targ[i].sem_targ = create_sem(1,name);

			sprintf(name,"sym%ld:%02d:done",s->num,i);
			s->targ[i].sem_done = create_sem(0,name);
		}
	}

	if(flags & symf_wide){
		s->idmask = 15;
		s->op_in = OP_WDATA_IN;
		s->op_out = OP_WDATA_OUT;
	} else {
		s->idmask = 7;
		s->op_in = OP_NDATA_IN;
		s->op_out = OP_NDATA_OUT;
	}

	reloc_script(s);
    return s;

err:
	free(s);
	delete_area(aid);
	return NULL;
}


/*
** Multiple Card Cruft
*/
#define MAXCARDS 4

static Symbios *cardinfo[MAXCARDS] = { NULL, NULL, NULL, NULL };

static long sim_init0(void)                { return init_symbios(cardinfo[0],0); }
static long sim_init1(void)                { return init_symbios(cardinfo[1],0); }
static long sim_init2(void)                { return init_symbios(cardinfo[2],0); }
static long sim_init3(void)                { return init_symbios(cardinfo[3],0); }
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
sim_install_symbios(void)
{
    int i, j, iobase, irq;
    int cardcount = 0;
    pci_info h;
    CAM_SIM_ENTRY entry;

    dprintf("symbios: sim_install()\n");

    for (i = 0; ; i++) {
		if ((*pci->get_nth_pci_info) (i, &h) != B_NO_ERROR) {
			/*if(!cardcount) d_printf("symbios: no controller found\n");*/
			break;
		}

//		d_printf("scan: %04x %04x %02x\n", h.device_id, h.vendor_id, h.revision);

#define PCI_VENDOR_SYMBIOS 0x1000

		if(h.vendor_id == PCI_VENDOR_SYMBIOS) {
			for(j=0;devinfo[j].id;j++){
				if((devinfo[j].id == h.device_id) && (h.revision >= devinfo[j].rev)){
					iobase = h.u.h0.base_registers[0];
					irq = h.u.h0.interrupt_line;
					d_printf("symbios%d: %s controller @ 0x%08x, irq %d\n",
							 cardcount, devinfo[j].name, iobase, irq);

					if(cardcount == MAXCARDS){
						d_printf("symbios: too many controllers!\n");
						return cardcount;
					}

					if((cardinfo[cardcount]=create_cardinfo(cardcount,&h,devinfo[j].flags)) != NULL){
						cardinfo[cardcount]->name = devinfo[j].name;
						entry.sim_init = sim_init_funcs[cardcount];
						entry.sim_action = sim_action_funcs[cardcount];
						cardinfo[cardcount]->registered = 0;
						register_stats(cardinfo[cardcount]);

						(*cam->xpt_bus_register)(&entry);
						cardcount++;
					} else {
						d_printf("symbios%d: cannot allocate cardinfo\n",cardcount);
					}
					break;
				}
			}
		}
    }

    return cardcount;
}

static status_t std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
#if DEBUG_SAFETY
		set_dprintf_enabled(true);
#endif
		load_driver_symbols("53c8xx");

		if (get_module(pci_name, (module_info **) &pci) != B_OK)
			return B_ERROR;

		if (get_module(cam_name, (module_info **) &cam) != B_OK) {
			put_module(pci_name);
			return B_ERROR;
		}

		if(sim_install_symbios()){
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

static
sim_module_info sim_symbios_module = {
	{ "busses/scsi/53c8xx/v1", 0, &std_ops }
};

_EXPORT
module_info  *modules[] =
{
	(module_info *) &sim_symbios_module,
	NULL
};

