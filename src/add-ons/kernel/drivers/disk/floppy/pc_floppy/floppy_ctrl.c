/*
 * Copyright 2003-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include <Drivers.h>
#include "floppy.h"


/* VERY verbose... */
//#define DEBUG_LOWLEVEL

#if defined(DEBUG) && DEBUG > 0
#	define PRINT_SR0(x) print_sr0(x)
#else
#	define PRINT_SR0(x) ;
#endif


#if defined(DEBUG) && DEBUG > 0
static void
print_sr0(uint8 sr0)
{
	const char *result = "ok";
	switch (sr0 & FDC_SR0_IC) {
		case 0x80:
			result = "invalid";
			break;
		case 0x40:
			result = "abterm";
			break;
		case 0xc0:
			result = "drvchngd";
			break;
	}

	TRACE("sr0: ds %d, hs %d, ec %d, se %d, %s\n", (sr0 & FDC_SR0_DS), !!(sr0 & FDC_SR0_HS), !!(sr0 & FDC_SR0_EC), !!(sr0 & FDC_SR0_SE), result);
}
#endif


void
write_reg(floppy_t *flp, floppy_reg_selector selector, uint8 data)
{
#ifdef DEBUG_LOWLEVEL
	TRACE("write to 0x%lx, data 0x%x\n", flp->iobase + selector, data);
#endif
	flp->isa->write_io_8(flp->iobase + selector, data);
}

uint8 read_reg(floppy_t *flp, floppy_reg_selector selector)
{
	uint8 data;
	data = flp->isa->read_io_8(flp->iobase + selector);
#ifdef DEBUG_LOWLEVEL
	TRACE("read from 0x%lx = 0x%x\n", flp->iobase + selector, data);
#endif
	return data;
}


void
reset_controller(floppy_t *master)
{
	uint8 command[4]; /* for SPECIFY & CONFIGURE */
	uint8 result[1]; /* for SPECIFY */
	
	TRACE("reset_controller()\n");
	LOCK(master->ben);
	
	//master->pending_cmd = CMD_RESET;
	master->pending_cmd = 0;
	write_reg(master, DIGITAL_OUT, read_reg(master, DIGITAL_OUT) & ~0x04); /* reset */
	//wait_result(master);
	spin(20);
	write_reg(master, DATA_RATE_SELECT, FDC_500KBPS); // 500 kbps
	master->data_rate = FDC_500KBPS;
	write_reg(master, DIGITAL_OUT, /*(master->dma < 0)?0x04:*//*0x0c*/0x04); /* motors[abcd] off, DMA on, deassert reset, drive select(0) */
	
	wait_result(master); /* wait for reset interrupt */
	
	command[0] = CMD_CONFIG;	// configure command
	command[1] = 0;		// N/A
	command[2] = 0x57;//0x70;	// Implied Seek, FIFO, Poll Disable, THRESH = default
	command[3] = 0;		// PRETRK
	
	send_command(master, command, sizeof(command));
	
	command[0] = CMD_SPECIFY;
	command[1] = (((16 - (3)) << 4) | ((240 / 16))); /* step rate 3ms, head unload time 240ms */
	command[2] = 0x02 | 0x01; /* head load time 2ms, NO DMA */
	
	send_command(master, command, 3); /* send SPECIFY */
	read_result(master, result, 1);
	
	UNLOCK(master->ben);
}


/* returns a bitmap of the present drives */
int
count_floppies(floppy_t *master)
{
	int i, err;
	int flops = 0;
	
	master->master = master;

	//floppy_dump_reg(master);
	
	/* reset controller */
	reset_controller(master);
	
	
	for (i = 0; i < MAX_DRIVES_PER_CTRL; i++) {
		master->drive_num = i; /* fake */
		TRACE("DETECTING DRIVE %d...\n", i);
		turn_on_motor(master);
		drive_select(master);
		err = recalibrate_drive(master);
		if (err == 0)
			flops |= 1 << i;
		TRACE("DRIVE %d %s\n", i, err?"NOT here":"is here");
		drive_deselect(master);
		//snooze(50000);
		//turn_off_motor(master);
	}
	
//	floppy_dump_reg(master);
	
	master->drive_num = 0;
	return flops;
}


/* selects the drive until deselect(), takes the benaphore */
void
drive_select(floppy_t *flp)
{
	cpu_status st;
	uint8 reg;
	TRACE("drive_select(%d)\n", flp->drive_num);
	LOCK(flp->master->ben);
	
	/* must be atomic to not change motor states! */
	st = disable_interrupts();
	acquire_spinlock(&(flp->master->slock));
	
	/* make sure the speed for this drive is correct */
	if (flp->geometry && (flp->master->data_rate != flp->geometry->data_rate)) {
		write_reg(flp, DATA_RATE_SELECT, flp->geometry->data_rate); // 500 kbps
		flp->master->data_rate = flp->geometry->data_rate;
	}

	/* make sure the drive is selected and DMA is on */
	reg = read_reg(flp, DIGITAL_OUT);
	if((reg & 0x0b) != ((flp->drive_num & 0x03) | 0x08))
		write_reg(flp, DIGITAL_OUT, (reg & 0xfc) | (flp->drive_num & 0x03) | /*(flp->master->dma < 0)?0x04:*/0x0c); /* DMA on, reset off */
	
	release_spinlock(&(flp->master->slock));
	restore_interrupts(st);

	flp->master->current = flp->drive_num;
}


void
drive_deselect(floppy_t *flp)
{
	UNLOCK(flp->master->ben);
	TRACE("drive_deselect(%d)\n", flp->drive_num);
}


void
turn_on_motor(floppy_t *flp)
{
	cpu_status st;
	uint8 reg;
	bool do_snooze = false;
	
	TRACE("turn_on_motor(%d)\n", flp->drive_num);
	flp->motor_timeout = MOTOR_TIMEOUT;
	
	/* must be atomic to not deselect a drive! */
	st = disable_interrupts();
	acquire_spinlock(&(flp->master->slock));
	
	if(((reg = read_reg(flp, DIGITAL_OUT)) & (0x10 << flp->drive_num)) == 0) {
		/* it's off now, turn it on and wait */
		//reg = 0x0c;
		//reg &= 0xfc; /* mask out drive num */
		write_reg(flp, DIGITAL_OUT, (0x10 << flp->drive_num) | reg);
		do_snooze = true;
	}
	
	release_spinlock(&(flp->master->slock));
	restore_interrupts(st);
	
	if (do_snooze)
		snooze(MOTOR_SPINUP_DELAY);
}


void
turn_off_motor(floppy_t *flp)
{
	cpu_status st;
	TRACE("turn_off_motor(%d)\n", flp->drive_num);
	
	/* must be atomic to not deselect a drive! */
	st = disable_interrupts();
	acquire_spinlock(&(flp->master->slock));
	
	write_reg(flp, DIGITAL_OUT, read_reg(flp, DIGITAL_OUT) & ~(0x10 << flp->drive_num));
	//write_reg(flp, DIGITAL_OUT, 0x4);
	
	flp->motor_timeout = 0;
	
	release_spinlock(&(flp->master->slock));
	restore_interrupts(st);
}


void
wait_for_rqm(floppy_t *flp)
{
	while ((read_reg(flp, MAIN_STATUS) & 0x80) == 0);
}


int
send_command(floppy_t *flp, const uint8 *data, int len)
{
	int i, status;
	
	switch (data[0] & CMD_SWITCH_MASK) {
	case CMD_SENSEI:
	case CMD_SENSED:
	case CMD_CONFIG:
	case CMD_DUMPREG:
		/* those don't generate an interrupt
		 * (SENSEI is sent by the intrrupt handler itself!)
		 */
		break;
	default:
		flp->pending_cmd = data[0];
	}
	for(i = 0; i < len; i++) {
		status = wait_til_ready(flp);
		if ((status < 0) || (status & 0x40))
			break;
		write_reg(flp, DATA, data[i]);
	}
#ifdef DEBUG_LOWLEVEL
	TRACE("sent %d B\n", i);
#endif
	return i;
}


int
wait_result(floppy_t *flp) {
	status_t err;

	{ int32 c; get_sem_count(flp->completion, &c); TRACE("SEM< %ld\n", c); }
	
	if ((err = acquire_sem_etc(flp->completion, 1, B_TIMEOUT, FLOPPY_CMD_TIMEOUT)) < B_OK) {
		if (err == B_TIMED_OUT) {
			cpu_status st;
			TRACE("timed out! faking intr !!\n");
			st = disable_interrupts();
			flo_intr(flp);
			restore_interrupts(st);
			acquire_sem_etc(flp->completion, 1, B_TIMEOUT, FLOPPY_CMD_TIMEOUT);
		}
		return -1;
	}
	{ int32 c; get_sem_count(flp->completion, &c); TRACE("SEM> %ld\n", c); }
	return 0;
}


int
read_result(floppy_t *flp, uint8 *data, int len)
{
	int i, status;
	
	//if (flp->master->need_reset)
	//	return -1;
	for(i = 0; i < len; i++) {
		status = wait_til_ready(flp);
		if ((status < 0) || ((status & 0x40) == 0))
			break;
		data[i] = read_reg(flp, DATA);
	}
	//flp->master->need_reset = 1;
#ifdef DEBUG_LOWLEVEL
	TRACE("read %d B\n", i);
#endif
	return i;
}


int
wait_til_ready(floppy_t *flp)
{
	int i, status;
	
	for (i = 0; i < 1000; i++)
		if ((status = read_reg(flp, MAIN_STATUS)) & 0x80)
			return status;
	TRACE("timeout waiting for %d !\n", flp->drive_num);
	return -1;
}

#if 0
static int
has_drive_changed(floppy_t *flp)
{
	return !!(read_reg(flp, DIGITAL_IN) & 0x80);
}
#endif

status_t
query_media(floppy_t *flp, bool forceupdate)
{
	status_t err = B_OK;
	uint8 command[4];
	uint8 result[7];
	const floppy_geometry *geom = NULL;

	TRACE("query_media(%d, %s)\n", flp->drive_num, forceupdate?"true":"false");

	turn_on_motor(flp);
	drive_select(flp);

	if (read_reg(flp, DIGITAL_IN) & 0x80) {/* media changed */
		flp->status = FLOP_MEDIA_CHANGED;
		TRACE("media changed\n");
	}
	if (err || (flp->status < FLOP_MEDIA_UNREADABLE) || forceupdate) {
		geom = supported_geometries;
		TRACE("querying format [err %08lx, st %d, fu %s]\n", err, flp->status, forceupdate?"t":"f");

		/* zero the current geometry */
		flp->geometry = 0;
		flp->bgeom.bytes_per_sector = 255;
		flp->bgeom.sectors_per_track = 0;
		flp->bgeom.cylinder_count = 0;
		flp->bgeom.head_count = 0;
		flp->bgeom.read_only = true;
		
		command[0] = 0x04;      // sense drive command
		command[1] = 0x00 | (flp->drive_num & 0x03);         // 
		
		send_command(flp, command, 2);
		
		read_result(flp, result, 1);
		TRACE("sense_drive(%d): wp %d, trk0 %d, hd %d, drivesel %d\n", flp->drive_num, !!(result[0]&0x40), !!(result[0]&0x10), !!(result[0]&0x04), (result[0]&0x03));
		flp->bgeom.read_only = !!(result[0]&0x40);
		
		for (; geom->numsectors; geom++) {
			TRACE("trying geometry %s\n", geom->name);
			
			/* apply geometry parameters */
			if (flp->master->data_rate != geom->data_rate) {
				write_reg(flp, DATA_RATE_SELECT, geom->data_rate); // 500 kbps
				flp->master->data_rate = geom->data_rate;
			}
			
			/* seek track 0, head 0 */
			command[0] = 0x0f; // seek command
			command[1] = (flp->drive_num & 0x03) | 0x00; // drive | head 0
			command[2] = 0x00; // track 0
			
			TRACE("SEEK at track 0 head 0\n");
			send_command(flp, command, 3);
			if (wait_result(flp) < 0)
				continue;
			
			/* if it went there then there si something, even if we can't read it */
			flp->status = FLOP_MEDIA_UNREADABLE;
			
			//command[0] = 8; // sense interrupt command
			//send_command(flp, command, 1);
			
			//read_result(flp, result, 2); // read the result
			PRINT_SR0(flp->result[0]);
			TRACE("track is %d\n", flp->result[1]);
			if (flp->result[0] & FDC_SR0_IC)
				continue;
			
			command[0] = 0x0a | ((geom->flags&FL_MFM)?0x40:0); // read track id
			command[1] = (flp->drive_num & 0x03) | 0x00; // drive | head 0
			
			TRACE("READ_TRACK_ID\n");
			send_command(flp, command, 2);
			if (wait_result(flp) < 0)
				continue;
			
			//read_result(flp, result, 7);
			PRINT_SR0(flp->result[0]);
			TRACE("sr1: %02x\n", flp->result[1]);
			TRACE("sr2: %02x\n", flp->result[2]);
			TRACE("read id: track %d, head %d, sec %d, bps %d\n", flp->result[3], flp->result[4], flp->result[5], flp->result[6]);
			if (flp->result[0] & FDC_SR0_IC)
				continue;
			
			/* seek track 2, head 1 */
			command[0] = 0x0f; // seek command
			command[1] = (flp->drive_num & 0x03) | 0x04; // drive | head 1
			command[2] = 0x02; // track 2
			
			TRACE("SEEK at track 2 head 1\n");
			send_command(flp, command, 3);
			if (wait_result(flp) < 0)
				continue;
			
			//command[0] = 8; // sense interrupt command
			//send_command(flp, command, 1);
			
			//read_result(flp, result, 2); // read the result
			PRINT_SR0(flp->result[0]);
			TRACE("track is %d\n", flp->result[1]);
			if (flp->result[0] & FDC_SR0_IC)
				continue;
			
			command[0] = 0x0a | ((geom->flags&FL_MFM)?0x40:0); // read track id
			command[1] = (flp->drive_num & 0x03) | 0x00; // drive | head 0
			
			TRACE("READ_TRACK_ID\n");
			send_command(flp, command, 2);
			if (wait_result(flp) < 0)
				continue;
			
			//read_result(flp, result, 7);
			
			//read_result(flp, result, 7);
			PRINT_SR0(flp->result[0]);
			TRACE("sr1: %02x\n", flp->result[1]);
			TRACE("sr2: %02x\n", flp->result[2]);
			TRACE("read id: track %d, head %d, sec %d, bps %d\n", flp->result[3], flp->result[4], flp->result[5], flp->result[6]);
			if (flp->result[0] & FDC_SR0_IC)
				continue;

			break;
		}
		if (geom->numsectors) {
			dprintf(FLO "drive %d: media type is %s\n", flp->drive_num, geom->name);
			flp->status = FLOP_MEDIA_FORMAT_FOUND;
			err = B_OK;
			flp->geometry = geom;
			flp->bgeom.bytes_per_sector = geom->g.bytes_per_sector;
			flp->bgeom.sectors_per_track = geom->g.sectors_per_track;
			flp->bgeom.cylinder_count = geom->g.cylinder_count;
			flp->bgeom.head_count = geom->g.head_count;
			//flp->bgeom.read_only = true;
		} else {
			flp->status = FLOP_NO_MEDIA;
		}
	}

	switch (flp->status) {
	case FLOP_NO_MEDIA:
		err = B_DEV_NO_MEDIA;
		break;
	case FLOP_MEDIA_CHANGED:
		err = B_DEV_MEDIA_CHANGED;
		break;
	case FLOP_MEDIA_UNREADABLE:
		err = B_DEV_UNREADABLE;
		break;
	case FLOP_MEDIA_FORMAT_FOUND:
	case FLOP_WORKING:
	default:
		err = B_OK;
	}

	drive_deselect(flp);
	return err;
}


int
recalibrate_drive(floppy_t *flp)
{
	int retry;
	
	TRACE("recalibrate_drive(%d)\n", flp->drive_num);
	
	turn_on_motor(flp);
	
	for(retry = 0; retry < 1; retry++) {
		uint8 command[2] = { 7, 0 }; // recalibrate command

		command[1] = flp->drive_num & 0x03;
		// send the recalibrate command
		send_command(flp, command, sizeof(command));
		
		if (wait_result(flp) < 0)
			return 3;
		
		//command[0] = 8; // sense interrupt command
		//send_command(flp, command, 1);
		
		// read the result
		//read_result(flp, result, sizeof(result));
		if (flp->result[0] & 0xd0) {
			TRACE("recalibration failed\n");
			return 2;
		} if (flp->result[1] != 0) {
			TRACE("drive is at cylinder %d, didn't make it to 0\n", flp->result[1]);
			if (retry > 3)
				return 1;
		} else {
			// successful
			break;
		}
	}

	TRACE("recalibration done\n");
	return 0;
}


int32
flo_intr(void *arg)
{
	int i;
	int len;
	int32 err = B_UNHANDLED_INTERRUPT;
	floppy_t *flp = (floppy_t *)arg;
	floppy_t *master = flp->master;

	if (master ==NULL)
		return B_UNHANDLED_INTERRUPT;
	acquire_spinlock(&master->slock);

	while (flp && (flp->drive_num != master->current) && !flp->pending_cmd)
		flp = flp->next;
	if (flp) {
		uint8 msr;
		int got = 0;
		msr = read_reg(flp, MAIN_STATUS);
		//TRACE("got irq for %d! MSR=%02x\n", flp->drive_num, msr);
		if ((((flp->pending_cmd & CMD_SWITCH_MASK) == CMD_READD) ||
			((flp->pending_cmd & CMD_SWITCH_MASK) == CMD_READTRK)) && ((msr & 0x60) == 0x60)) {
											/* non DMA xfer(data) & need read */
/*			uint8 command[1];
			command[0] = CMD_SENSEI;
			send_command(flp, command, 1);
			len = read_result(flp, flp->result, 2);
			PRINT_SR0(flp->result[0]);*/
			
			//TRACE("READi\n");
//			while ((msr & 0x60) == 0x60 /*&& (got < 256)*/) {
			while ((msr & 0xF0) == 0xF0 && (master->buffer_index < CYL_BUFFER_SIZE)) {
				master->buffer[master->buffer_index++] = read_reg(flp, DATA);
				master->avail++;
				msr = read_reg(flp, MAIN_STATUS);
				got++;
//				spin(10);
/*				if (got < 30)
					TRACE("%02x", msr);
				else
					spin(15);*/
			//	if (got > 255)
			//	break;
			}
//			TRACE("intr: READ %d\n", got);
		} else if (((flp->pending_cmd & CMD_SWITCH_MASK) == CMD_WRITED) && ((msr & 0x60) == 0x20)) {
											/* non DMA xfer(data) & need write */
			TRACE("WRITEi\n");
			
		} else {
			len = 8;
//			if (flp->pending_cmd != CMD_RESET)
			len = read_result(flp, flp->result, len);
//			else
//				len = 0;
			if (len < 0) {
				TRACE("buggy interrupt from %d !\n", flp->drive_num);
			} else if (len < 1) { /* must pool the interrupt reason */
				uint8 command[1] = { CMD_SENSEI };
				for (i = 0; i < 4; i++) { /* might have to issue 4 SENSEI */
					TRACE("intr: %dth SENSEI\n", i+1);
					if (send_command(flp, command, 1) < 1)
						break;
					len = read_result(flp, flp->result, 2);
					if (len > 0)
						PRINT_SR0(flp->result[0]);
					if (len != 2)
						break;
					if ((flp->result[0] & (0x80|FDC_SR0_DS)) == master->current)
						break;
				}
			} else
				TRACE("RES(%d) %02x %02x %02x %02x\n", len, flp->result[0], flp->result[1], flp->result[2], flp->result[3]);
			/* only do that for !READ && !WRITE */
			flp->pending_cmd = 0;
			release_sem_etc(flp->completion, 1, B_DO_NOT_RESCHEDULE);
		}
		err = B_HANDLED_INTERRUPT;
	}

	release_spinlock(&master->slock);

	if (err == B_UNHANDLED_INTERRUPT)
		TRACE("unhandled irq!\n");
	return err;
}


void
floppy_dump_reg(floppy_t *flp) {
	uint8 command[1] = { 0x0e }; // dumpreg command
	//uint8 result[10];
	uint8 *result = flp->result;
	
	send_command(flp, command, sizeof(command));
	
	//wait_result(flp);
	read_result(flp, result, 8);
	
	dprintf(FLO "dumpreg: tracks - d1=%d d2=%d d3=%d d4=%d\n", 
		result[0], result[1], result[2], result[3]);
	dprintf(FLO "dumpreg: step_rate_time=%d motor_off_time=%d motor_on_time=%d dma=%d\n", 
		result[4] >> 4, result[4] & 0x0f, result[5] >> 1, result[5] & 0x01);
	dprintf(FLO "sec_per_trk/end_of_trk=0x%.2x lock=%d, byte[7]=0x%.2x\n", 
		result[6], result[7] >> 7,result[7]);
	dprintf(FLO "gap=%d wg=%d eis=%d fifo=%d poll=%d thresh=%d pretrk=%d\n", 
		(result[7] & 0x02) >> 1, result[7] & 0x01, (result[8] & 0x40) >> 6, 
		(result[8] & 0x20) >> 5, (result[8] & 0x10) >> 4, result[8] & 0x0f, result[9]);
}


static void
fill_command_from_lba(floppy_t *flp, floppy_command *cmd, int lba)
{
	cmd->cylinder = lba / (flp->bgeom.sectors_per_track * flp->bgeom.head_count);
	cmd->head = (lba / flp->bgeom.sectors_per_track) % flp->bgeom.head_count;
	cmd->sector = lba % flp->bgeom.sectors_per_track + 1;
	cmd->drive = (flp->drive_num & 0x3) | (cmd->head << 2) | 0x80; /* Implied Seek */
}


/* does NOT check for valid track number */
ssize_t
pio_read_sectors(floppy_t *flp, /*void *buf,*/ int lba, int num_sectors)
{
	ssize_t transfered = 0;
	floppy_command cmd;
#if 0
	uint8 cmd2[8];
	uint8 result[4];
#endif
	//floppy_result res;
	
	if (flp->status < FLOP_MEDIA_FORMAT_FOUND)
		return B_DEV_UNREADABLE;
	turn_on_motor(flp);
	drive_select(flp);
	if (flp->geometry == NULL || !flp->bgeom.bytes_per_sector || !flp->bgeom.sectors_per_track) {
		drive_deselect(flp);
		return B_DEV_NO_MEDIA;
	}
	
	flp->track = -1;
	
	num_sectors = MIN(num_sectors, (signed)(flp->bgeom.sectors_per_track - (lba % flp->bgeom.sectors_per_track)));
	cmd.id = CMD_READD | /*0xc0*/0x40; // multi-track, read MFM, one head
	cmd.sector_size = 2;	/* 512 bytes */
	cmd.track_length = num_sectors;//flp->bgeom.sectors_per_track;
	cmd.gap3_length = flp->geometry->gap;	//27; /* 3.5" floppy */
	cmd.data_length = 0xff;	/* don't care */
	fill_command_from_lba(flp, &cmd, lba);
	dprintf(FLO "pio_read_sector(%d, %d={%d,%d,%d}, %d)\n", flp->drive_num, lba, cmd.cylinder, cmd.head, cmd.sector, num_sectors);
	if (num_sectors < 1) {
		drive_deselect(flp);
		return EINVAL;
	}
	//num_sectors = 1;

#if 0
	/* first issue sense interrupt, to find the current track */
	TRACE("track check: SENSEI\n");
	cmd2[0] = CMD_SENSEI;
	send_command(flp, cmd2, 1);
	read_result(flp, result, 2);
	PRINT_SR0(result[0]);
	/* if the current track is not the one we want, seek */
	if (result[1] != cmd.cylinder) {
		/* seek */
		cmd2[0] = CMD_SEEK; // seek command
		cmd2[1] = (flp->drive_num & 0x3) | (cmd.head << 2);
		cmd2[2] = cmd.cylinder; // track 0
		
		TRACE("SEEK at track %d head %d\n", cmd.cylinder, cmd.head);
		send_command(flp, cmd2, 3);
		if (wait_result(flp) < 0)
			return ENXIO;
	}

#endif
	
	flp->master->avail = 0;
	flp->master->buffer_index = (lba % (flp->bgeom.sectors_per_track/* * flp->bgeom.head_count*/)) * flp->bgeom.bytes_per_sector;
	//flp->pending_cmd = CMD_READD;
	
	send_command(flp, (uint8 *)&cmd, sizeof(cmd));

	if (wait_result(flp) < 0) {
		drive_deselect(flp);
		return ENXIO;
	}
	PRINT_SR0(flp->result[0]);
	TRACE("sr1: %02x\n", flp->result[1]);
	TRACE("sr2: %02x\n", flp->result[2]);
	TRACE("@ track %d, head %d, sec %d, bps %d\n", flp->result[3], flp->result[4], flp->result[5], flp->result[6]);
	switch (flp->result[0] & FDC_SR0_IC) {
	case 0x80:
		transfered = EINVAL;
		break;
	case 0x40:
		//if (flp->result[1] != 0x80) /* End Of Track is not really an error, actually it means it worked :) */
		//	transfered = ENXIO;
		break;
	case 0xc0:
		transfered = B_DEV_MEDIA_CHANGED;
		break;
	}
	if (transfered) {
		drive_deselect(flp);
		return ENXIO;
	}
	/* normal termination */
	transfered = flp->avail;
/*	if (transfered > 0)
		memcpy(buf, flp->buffer, flp->avail);*/
	
	drive_deselect(flp);
	return num_sectors;//transfered;
}


ssize_t
pio_write_sectors(floppy_t *flp, /*const void *buf,*/ int lba, int num_sectors)
{
	return -1;
}


/* does NOT check for valid track number */
ssize_t
pio_read_track(floppy_t *flp, /*void *buf,*/ int lba)
{
	ssize_t transfered = 0;
	floppy_command cmd;
#if 0
	uint8 cmd2[8];
	uint8 result[4];
#endif
	int tries = 0;
	
	if (flp->status < FLOP_MEDIA_FORMAT_FOUND)
		return B_DEV_UNREADABLE;
	turn_on_motor(flp);
	drive_select(flp);
	if (flp->geometry == NULL || !flp->bgeom.bytes_per_sector || !flp->bgeom.sectors_per_track) {
		drive_deselect(flp);
		return B_DEV_NO_MEDIA;
	}
	
	flp->track = -1;
	
retry_track:
	
	//num_sectors = MIN(num_sectors, (signed)(flp->bgeom.sectors_per_track - (lba % flp->bgeom.sectors_per_track)));
	cmd.id = CMD_READTRK | 0x40; // read MFM
	cmd.sector_size = 2;	/* 512 bytes */
	cmd.track_length = flp->bgeom.sectors_per_track;
	cmd.gap3_length = flp->geometry->gap;	//27; /* 3.5" floppy */
	cmd.data_length = 0xff;	/* don't care */
	fill_command_from_lba(flp, &cmd, lba);
	cmd.sector = 1;
	dprintf(FLO "pio_read_track(%d, %d={%d,%d,%d}) try %d\n", flp->drive_num, lba, cmd.cylinder, cmd.head, cmd.sector, tries);
	
#if 0
	/* first issue sense interrupt, to find the current track */
	TRACE("track check: SENSEI\n");
	cmd2[0] = CMD_SENSEI;
	send_command(flp, cmd2, 1);
	read_result(flp, result, 2);
	PRINT_SR0(result[0]);
	/* if the current track is not the one we want, seek */
	if (result[1] != cmd.cylinder) {
		/* seek */
		cmd2[0] = CMD_SEEK; // seek command
		cmd2[1] = (flp->drive_num & 0x3) | (cmd.head << 2);
		cmd2[2] = cmd.cylinder; // track 0
		
		TRACE("SEEK at track %d head %d\n", cmd.cylinder, cmd.head);
		send_command(flp, cmd2, 3);
		if (wait_result(flp) < 0)
			return ENXIO;
	}
#endif

	flp->master->avail = 0;
	flp->master->buffer_index = 0;//(lba % (flp->bgeom.sectors_per_track * flp->bgeom.head_count)) * flp->bgeom.bytes_per_sector;
	
	send_command(flp, (uint8 *)&cmd, sizeof(cmd));

	if (wait_result(flp) < 0) {
		drive_deselect(flp);
		return ENXIO;
	}

	PRINT_SR0(flp->result[0]);
	TRACE("sr1: %02x\n", flp->result[1]);
	TRACE("sr2: %02x\n", flp->result[2]);
	TRACE("@ track %d, head %d, sec %d, bps %d\n", flp->result[3], flp->result[4], flp->result[5], flp->result[6]);

	switch (flp->result[0] & FDC_SR0_IC) {
	case 0x80:
		transfered = EINVAL;
		break;
	case 0x40:
		TRACE(FLO "sr1: %02x\n", flp->result[1]);
		if (flp->result[1] != 0x80) {/* End Of Track is not really an error, actually it means it worked :) */
			if (/*(flp->result[1] == 0x10) && */tries < 3) { /* overrun */
				tries++;
				goto retry_track;
			} else
				transfered = EIO;
		}
		break;
	case 0xc0:
		transfered = B_DEV_MEDIA_CHANGED;
		break;
	}
	if (transfered) {
		drive_deselect(flp);
		return ENXIO;
	}
	/* normal termination */
	transfered = flp->avail;
/*	if (transfered > 0)
		memcpy(buf, flp->buffer, flp->avail);*/
	flp->master->track = cmd.cylinder*flp->bgeom.head_count + cmd.head;
	
	drive_deselect(flp);
	return 1;//transfered;
}


ssize_t
read_sectors(floppy_t *flp, int lba, int num_sectors)
{
	ssize_t transfered = 0;
	//return pio_read_sectors(flp, lba, num_sectors);
	/* if (nothing cached || not the same track cached) */
	dprintf(FLO "read_sector(%d, %d, %d)\n", flp->drive_num, lba, num_sectors);
	num_sectors = MIN(num_sectors, (signed)(flp->bgeom.sectors_per_track - (lba % flp->bgeom.sectors_per_track)));
	if ((flp->master->track < 0) || 
		(flp->master->track != (signed)(lba / (flp->bgeom.sectors_per_track)))) {
		if ((lba / (flp->bgeom.sectors_per_track)) >= (flp->bgeom.head_count * flp->bgeom.cylinder_count))
			return ENXIO;
		transfered = pio_read_track(flp, lba);
		if (transfered < 0)
			return transfered;
		/* XXX: TODO: in case of IO error, retry by single sector */
	}
	return num_sectors;
}

