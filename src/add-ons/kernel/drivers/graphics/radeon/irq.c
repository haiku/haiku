/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	Interrupt handling. Currently, none of this is used
	as I haven't got the HW spec.
*/

#include "radeon_driver.h"
#include <stdio.h>

#include <mmio.h>
#include <rbbm_regs.h>


// disable all interrupts
static void Radeon_DisableIRQ( device_info *di )
{
	OUTREG( di->regs, RADEON_GEN_INT_CNTL, 0 );
}


static uint32 thread_interrupt_work( vuint8 *regs, device_info *di, uint32 int_status ) 
{
	shared_info *si = di->si;
	uint32 handled = B_HANDLED_INTERRUPT;

	if( (int_status & RADEON_CRTC_VBLANK_STAT) != 0 &&
		si->ports[0].vblank >= 0 ) 
	{
		int32 blocked;

		++di->vbi_count[0];
		
		if( (get_sem_count( si->ports[0].vblank, &blocked ) == B_OK) && (blocked < 0) ) {
			//SHOW_FLOW( 3, "Signalled VBI 0 (%ldx)", -blocked );
			release_sem_etc( si->ports[0].vblank, -blocked, B_DO_NOT_RESCHEDULE );
			handled = B_INVOKE_SCHEDULER;
		}
	}
	
	if( (int_status & RADEON_CRTC2_VBLANK_STAT) != 0 &&
		si->ports[1].vblank >= 0 ) 
	{
		int32 blocked;
	
		++di->vbi_count[1];	
		
		if( (get_sem_count( si->ports[1].vblank, &blocked ) == B_OK) && (blocked < 0) ) {
			//SHOW_FLOW( 3, "Signalled VBI 1 (%ldx)", -blocked );
			release_sem_etc( si->ports[1].vblank, -blocked, B_DO_NOT_RESCHEDULE );
			handled = B_INVOKE_SCHEDULER;
		}
	}
	
	return handled;
}


static int32
Radeon_Interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	vuint8 *regs = di->regs;
	int32 int_status;

	int_status = INREG( regs, RADEON_GEN_INT_STATUS );

	if( int_status != 0 ) {
		++di->interrupt_count;
		
		handled = thread_interrupt_work( regs, di, int_status );
		OUTREG( regs, RADEON_GEN_INT_STATUS, int_status );
	}

	return handled;				
}

static int32 timer_interrupt_func( timer *te ) 
{
	bigtime_t now = system_time();
	/* get the pointer to the device we're handling this time */
	device_info *di = ((timer_info *)te)->di;
	shared_info *si = di->si;
	vuint8 *regs = di->regs;
	uint32 vbl_status = 0 /* read vertical blank status */;
	int32 result = B_HANDLED_INTERRUPT;

	/* are we suppoesed to handle interrupts still? */
	if( !di->shutdown_virtual_irq ) {
		/* reschedule with same period by default */
		bigtime_t when = si->refresh_period;
		timer *to;

		/* if interrupts are "enabled", do our thing */
		if( si->enable_virtual_irq ) {
			/* insert code to sync to interrupts here */
			if (!vbl_status) {
				when -= si->blank_period - 4;
			} 
			/* do the things we do when we notice a vertical retrace */
			result = thread_interrupt_work( regs, di, 
				RADEON_CRTC_VBLANK_STAT | 
				(di->has_crtc2 ? RADEON_CRTC_VBLANK_STAT : 0 ));
		}

		/* pick the "other" timer */
		to = (timer *)&(di->ti_a);
		if (to == te) to = (timer *)&(di->ti_b);
		/* our guess as to when we should be back */
		((timer_info *)to)->when_target = now + when;
		/* reschedule the interrupt */
		add_timer(to, timer_interrupt_func, ((timer_info *)to)->when_target, B_ONE_SHOT_ABSOLUTE_TIMER);
		/* remember the currently active timer */
		di->current_timer = (timer_info *)to;
	}

	return result;
}

status_t Radeon_SetupIRQ( device_info *di, char *buffer )
{
	shared_info *si = di->si;
	status_t result;
	thread_id thid;
    thread_info thinfo;
	
	sprintf( buffer, "%04X_%04X_%02X%02X%02X VBI 0",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );		
	si->ports[0].vblank = create_sem( 0, buffer );
	if( si->ports[0].vblank < 0 ) {
		result = si->ports[0].vblank;
		goto err1;
	}

	sprintf( buffer, "%04X_%04X_%02X%02X%02X VBI 1",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );		
	si->ports[1].vblank = create_sem( 0, buffer );
	if( si->ports[1].vblank < 0 ) {
		result = si->ports[1].vblank;
		goto err2;
	}
	
    /* change the owner of the semaphores to the opener's team */
    /* this is required because apps can't aquire kernel semaphores */
    thid = find_thread(NULL);
    get_thread_info(thid, &thinfo);
    set_sem_owner(si->ports[0].vblank, thinfo.team);
    set_sem_owner(si->ports[1].vblank, thinfo.team);

    /* disable and clear any pending interrupts */
    Radeon_DisableIRQ( di );

    /* if we're faking interrupts */
    if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
		SHOW_INFO0( 3, "We like to fake IRQ" );
        /* fake some kind of interrupt with a timer */
        di->shutdown_virtual_irq = false;
        si->refresh_period = 16666; /* fake 60Hz to start */
        si->blank_period = si->refresh_period / 20;

        di->ti_a.di = di;    /* refer to ourself */
        di->ti_b.di = di;
        di->current_timer = &(di->ti_a);

        /* program the first timer interrupt, and it will handle the rest */
        result = add_timer((timer *)(di->current_timer), timer_interrupt_func, 
        	si->refresh_period, B_ONE_SHOT_RELATIVE_TIMER);
        if( result != B_OK )
        	goto err3;
    } else {
        /* otherwise install our interrupt handler */
        result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, 
        	Radeon_Interrupt, (void *)di, 0);
        if( result != B_OK )
        	goto err3;

		SHOW_INFO( 3, "installed IRQ @ %d", di->pcii.u.h0.interrupt_line );
    }
    
    return B_OK;
    
err3:
	delete_sem( si->ports[1].vblank );
err2:
	delete_sem( si->ports[0].vblank );
err1:
	return result;
}


void Radeon_CleanupIRQ( device_info *di )
{
	shared_info *si = di->si;
	
    Radeon_DisableIRQ( di );
    
    /* if we were faking the interrupts */
    if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
        /* stop our interrupt faking thread */
        di->shutdown_virtual_irq = true;
        /* cancel the timer */
        /* we don't know which one is current, so cancel them both and ignore any error */
        cancel_timer((timer *)&(di->ti_a));
        cancel_timer((timer *)&(di->ti_b));
    } else {
        /* remove interrupt handler */
        remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, Radeon_Interrupt, di);
    }
    
    delete_sem( si->ports[1].vblank );
	delete_sem( si->ports[0].vblank );

	si->ports[1].vblank = si->ports[0].vblank = 0;
}
