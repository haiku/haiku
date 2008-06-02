/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Copyright 2002-2004, Thomas Kurschel
 *
 * Distributed under the terms of the MIT License.
 */


#include "mmio.h"
#include "radeon_driver.h"
#include "rbbm_regs.h"

#include <stdio.h>


/**	Disable all interrupts */

static void
Radeon_DisableIRQ(device_info *di)
{
	OUTREG(di->regs, RADEON_GEN_INT_CNTL, 0);
}


/**	Interrupt worker routine
 *	handles standard interrupts, i.e. VBI and DMA
 */

static uint32
Radeon_ThreadInterruptWork(vuint8 *regs, device_info *di, uint32 int_status)
{
	shared_info *si = di->si;
	uint32 handled = B_HANDLED_INTERRUPT;

	if ((int_status & RADEON_CRTC_VBLANK_STAT) != 0
		&& si->crtc[0].vblank >= 0) {
		int32 blocked;

		++di->vbi_count[0];

		if (get_sem_count(si->crtc[0].vblank, &blocked ) == B_OK && blocked < 0) {
			release_sem_etc(si->crtc[0].vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}

	if ((int_status & RADEON_CRTC2_VBLANK_STAT) != 0
		&& si->crtc[1].vblank >= 0) {
		int32 blocked;

		++di->vbi_count[1];
		
		if (get_sem_count(si->crtc[1].vblank, &blocked) == B_OK && blocked < 0) {
			release_sem_etc(si->crtc[1].vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}

	if ((int_status & RADEON_VIDDMA_STAT) != 0) {
		release_sem_etc(di->dma_sem, 1, B_DO_NOT_RESCHEDULE);
		handled = B_INVOKE_SCHEDULER;
	}

	return handled;
}


/** Capture interrupt handler */

static int32
Radeon_HandleCaptureInterrupt(vuint8 *regs, device_info *di, uint32 cap_status)
{
	int32 blocked;
	uint32 handled = B_HANDLED_INTERRUPT;

	cpu_status prev_irq_state = disable_interrupts();
	acquire_spinlock(&di->cap_spinlock);

	++di->cap_counter;
	di->cap_int_status = cap_status;
	di->cap_timestamp = system_time();

	release_spinlock(&di->cap_spinlock);
	restore_interrupts(prev_irq_state);

	// don't release if semaphore count is positive, i.e. notifications are piling up
	if (get_sem_count(di->cap_sem, &blocked) == B_OK && blocked <= 0) {
		release_sem_etc(di->cap_sem, 1, B_DO_NOT_RESCHEDULE);
		handled = B_INVOKE_SCHEDULER;
	}

	// acknowledge IRQ			
	OUTREG(regs, RADEON_CAP_INT_STATUS, cap_status);

	return handled;
}


/**	Main interrupt handler */

static int32
Radeon_Interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	vuint8 *regs = di->regs;
	int32 full_int_status, int_status;

	// read possible IRQ reasons, ignoring any masked IRQs
	full_int_status = INREG(regs, RADEON_GEN_INT_STATUS);
	int_status = full_int_status & INREG(regs, RADEON_GEN_INT_CNTL);

	if (int_status != 0) {
		++di->interrupt_count;

		handled = Radeon_ThreadInterruptWork(regs, di, int_status);

		// acknowledge IRQ
		OUTREG(regs, RADEON_GEN_INT_STATUS, int_status);
	}

	// capture interrupt have no mask in GEN_INT_CNTL register;
	// probably, ATI wanted to make capture interrupt control independant of main control
	if ((full_int_status & RADEON_CAP0_INT_ACTIVE) != 0) {
		int32 cap_status;

		// same as before: only regard enabled IRQ reasons		
		cap_status = INREG(regs, RADEON_CAP_INT_STATUS);
		cap_status &= INREG(regs, RADEON_CAP_INT_CNTL);

		if (cap_status != 0) {
			int32 cap_handled;

			cap_handled = Radeon_HandleCaptureInterrupt(regs, di, cap_status);

			if (cap_handled == B_INVOKE_SCHEDULER || handled == B_INVOKE_SCHEDULER)
				handled = B_INVOKE_SCHEDULER;
			else if (cap_handled == B_HANDLED_INTERRUPT)
				handled = B_HANDLED_INTERRUPT;
		}
	}

	return handled;			
}


static int32
timer_interrupt_func(timer *te) 
{
	bigtime_t now = system_time();
	/* get the pointer to the device we're handling this time */
	device_info *di = ((timer_info *)te)->di;
	shared_info *si = di->si;
	vuint8 *regs = di->regs;
	uint32 vbl_status = 0 /* read vertical blank status */;
	int32 result = B_HANDLED_INTERRUPT;

	/* are we suppoesed to handle interrupts still? */
	if (!di->shutdown_virtual_irq) {
		/* reschedule with same period by default */
		bigtime_t when = si->refresh_period;
		timer *to;

		/* if interrupts are "enabled", do our thing */
		if (si->enable_virtual_irq) {
			/* insert code to sync to interrupts here */
			if (!vbl_status) {
				when -= si->blank_period - 4;
			} 
			/* do the things we do when we notice a vertical retrace */
			result = Radeon_ThreadInterruptWork(regs, di, 
				RADEON_CRTC_VBLANK_STAT
				| (di->num_crtc > 1 ? RADEON_CRTC2_VBLANK_STAT : 0));
		}

		/* pick the "other" timer */
		to = (timer *)&di->ti_a;
		if (to == te)
			to = (timer *)&di->ti_b;

		/* our guess as to when we should be back */
		((timer_info *)to)->when_target = now + when;

		/* reschedule the interrupt */
		add_timer(to, timer_interrupt_func, ((timer_info *)to)->when_target,
			B_ONE_SHOT_ABSOLUTE_TIMER);

		/* remember the currently active timer */
		di->current_timer = (timer_info *)to;
	}

	return result;
}


/** Setup IRQ handlers.
 *	Includes an VBI emulator via a timer (according to sample code), 
 *	though this makes sense for one CRTC only.
 */

status_t 
Radeon_SetupIRQ(device_info *di, char *buffer)
{
	shared_info *si = di->si;
	status_t result;
	thread_id thid;
    thread_info thinfo;

	sprintf(buffer, "%04X_%04X_%02X%02X%02X VBI 1",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	si->crtc[0].vblank = create_sem(0, buffer);
	if (si->crtc[0].vblank < 0) {
		result = si->crtc[0].vblank;
		goto err1;
	}

	si->crtc[1].vblank = 0;

	if (di->num_crtc > 1) {
		sprintf(buffer, "%04X_%04X_%02X%02X%02X VBI 2",
			di->pcii.vendor_id, di->pcii.device_id,
			di->pcii.bus, di->pcii.device, di->pcii.function);
		si->crtc[1].vblank = create_sem(0, buffer);
		if (si->crtc[1].vblank < 0) {
			result = si->crtc[1].vblank;
			goto err2;
		}
	}

	sprintf(buffer, "%04X_%04X_%02X%02X%02X Cap I",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	di->cap_sem = create_sem(0, buffer);
	if (di->cap_sem < 0) {
		result = di->cap_sem;
		goto err3;
	}

	B_INITIALIZE_SPINLOCK(&di->cap_spinlock);

	sprintf(buffer, "%04X_%04X_%02X%02X%02X DMA I",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	di->dma_sem = create_sem(0, buffer);
	if (di->dma_sem < 0) {
		result = di->dma_sem;
		goto err4;
	}

	/* change the owner of the semaphores to the opener's team */
	/* this is required because apps can't aquire kernel semaphores */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->crtc[0].vblank, thinfo.team);
	if (di->num_crtc > 1)
		set_sem_owner(si->crtc[1].vblank, thinfo.team);
	//set_sem_owner(di->cap_sem, thinfo.team);

	/* disable all interrupts */
	Radeon_DisableIRQ(di);

	/* if we're faking interrupts */
	if (di->pcii.u.h0.interrupt_pin == 0x00 || di->pcii.u.h0.interrupt_line == 0xff) {
		SHOW_INFO0( 3, "We like to fake IRQ" );
		/* fake some kind of interrupt with a timer */
		di->shutdown_virtual_irq = false;
		si->refresh_period = 16666; /* fake 60Hz to start */
		si->blank_period = si->refresh_period / 20;

		di->ti_a.di = di;    /* refer to ourself */
		di->ti_b.di = di;
		di->current_timer = &di->ti_a;

		/* program the first timer interrupt, and it will handle the rest */
		result = add_timer((timer *)(di->current_timer), timer_interrupt_func, 
			si->refresh_period, B_ONE_SHOT_RELATIVE_TIMER);
		if (result != B_OK)
			goto err5;
	} else {
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, 
			Radeon_Interrupt, (void *)di, 0);
		if (result != B_OK)
			goto err5;

		SHOW_INFO(3, "installed IRQ @ %d", di->pcii.u.h0.interrupt_line);
	}

	return B_OK;

err5:
	delete_sem(di->dma_sem);
err4:
	delete_sem(di->cap_sem);
err3:
	if (di->num_crtc > 1)
		delete_sem(si->crtc[1].vblank);
err2:
	delete_sem(si->crtc[0].vblank);
err1:
	return result;
}


/**	Clean-up interrupt handling */

void 
Radeon_CleanupIRQ(device_info *di)
{
	shared_info *si = di->si;

	Radeon_DisableIRQ(di);

	/* if we were faking the interrupts */
	if (di->pcii.u.h0.interrupt_pin == 0x00 || di->pcii.u.h0.interrupt_line == 0xff) {
		/* stop our interrupt faking thread */
		di->shutdown_virtual_irq = true;
		/* cancel the timer */
		/* we don't know which one is current, so cancel them both and ignore any error */
		cancel_timer((timer *)&di->ti_a);
		cancel_timer((timer *)&di->ti_b);
	} else {
		/* remove interrupt handler */
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, Radeon_Interrupt, di);
	}

	delete_sem(si->crtc[0].vblank);

	if (di->num_crtc > 1)
		delete_sem(si->crtc[1].vblank);

	delete_sem(di->cap_sem);
	delete_sem(di->dma_sem);

	di->cap_sem = si->crtc[1].vblank = si->crtc[0].vblank = 0;
}
