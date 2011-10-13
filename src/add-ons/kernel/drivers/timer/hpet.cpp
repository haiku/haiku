/*
 * Copyright 2009-2010, Stefano Ceccherini (stefano.ceccherini@gmail.com)
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "hpet.h"
#include "hpet_interface.h"
#include "int.h"
#include "msi.h"

#include <Drivers.h>
#include <KernelExport.h>
#include <ACPI.h>
#include <PCI.h>

#include <stdlib.h>
#include <string.h>


#define TRACE_HPET
#ifdef TRACE_HPET
	#define TRACE(x) dprintf x
#else
	#define TRACE(x) ;
#endif

#define TEST_HPET


static struct hpet_regs *sHPETRegs;
static uint64 sHPETPeriod;

static area_id sHPETArea;


struct hpet_timer_cookie {
	int number;
	int32 irq;
	sem_id sem;
};

////////////////////////////////////////////////////////////////////////////////

static status_t hpet_open(const char*, uint32, void**);
static status_t hpet_close(void*);
static status_t hpet_free(void*);
static status_t hpet_control(void*, uint32, void*, size_t);
static ssize_t hpet_read(void*, off_t, void*, size_t*);
static ssize_t hpet_write(void*, off_t, const void*, size_t*);

////////////////////////////////////////////////////////////////////////////////

static const char* hpet_name[] = {
    "misc/hpet",
    NULL
};


device_hooks hpet_hooks = {
	hpet_open,
	hpet_close,
	hpet_free,
	hpet_control,
	hpet_read,
	hpet_write,
};

int32 api_version = B_CUR_DRIVER_API_VERSION;

static acpi_module_info* sAcpi;
static vint32 sOpenCount;


static inline bigtime_t
hpet_convert_timeout(const bigtime_t &relativeTimeout)
{
	bigtime_t counter = sHPETRegs->u0.counter64;
	bigtime_t converted = (1000000000ULL / sHPETPeriod) * relativeTimeout;

	dprintf("counter: %lld, relativeTimeout: %lld, converted: %lld\n",
		counter, relativeTimeout, converted);

	return converted + counter;
}


#define MIN_TIMEOUT 3000

static status_t
hpet_set_hardware_timer(bigtime_t relativeTimeout, volatile hpet_timer *timer)
{
	// TODO:
	if (relativeTimeout < MIN_TIMEOUT)
		relativeTimeout = MIN_TIMEOUT;

	bigtime_t timerValue = hpet_convert_timeout(relativeTimeout);

	//dprintf("comparator: %lld, new value: %lld\n", timer->u0.comparator64, timerValue);

	timer->u0.comparator64 = timerValue;

	// enable timer interrupt
	timer->config |= HPET_CONF_TIMER_INT_ENABLE;

	return B_OK;
}


static status_t
hpet_clear_hardware_timer(volatile hpet_timer *timer)
{
	// Disable timer interrupt
	timer->config &= ~HPET_CONF_TIMER_INT_ENABLE;
	return B_OK;
}


static int32
hpet_timer_interrupt(void *arg)
{
	//dprintf("HPET timer_interrupt!!!!\n");
	hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)arg;

	// clear interrupt status
	int32 intStatus = 1 << hpetCookie->number;
	if (sHPETRegs->interrupt_status & intStatus) {
		sHPETRegs->interrupt_status |= intStatus;
		hpet_clear_hardware_timer(&sHPETRegs->timer[hpetCookie->number]);
		
		release_sem(hpetCookie->sem);
		return B_HANDLED_INTERRUPT;
	}

	return B_UNHANDLED_INTERRUPT;
}


static status_t
hpet_set_enabled(bool enabled)
{
	if (enabled)
		sHPETRegs->config |= HPET_CONF_MASK_ENABLED;
	else
		sHPETRegs->config &= ~HPET_CONF_MASK_ENABLED;
	return B_OK;
}


static status_t
hpet_set_legacy(bool enabled)
{
	if (!HPET_IS_LEGACY_CAPABLE(sHPETRegs)) {
		dprintf("hpet_init: HPET doesn't support legacy mode. Skipping.\n");
		return B_NOT_SUPPORTED;
	}

	if (enabled)
		sHPETRegs->config |= HPET_CONF_MASK_LEGACY;
	else
		sHPETRegs->config &= ~HPET_CONF_MASK_LEGACY;

	return B_OK;
}


#ifdef TRACE_HPET
static void
hpet_dump_timer(volatile struct hpet_timer *timer)
{
	dprintf("HPET Timer %ld:\n", (timer - sHPETRegs->timer));
	dprintf("CAP/CONFIG register: 0x%llx\n", timer->config);
	dprintf("Capabilities:\n");
	dprintf("\troutable IRQs: ");
	uint32 interrupts = (uint32)HPET_GET_CAP_TIMER_ROUTE(timer);
	for (int i = 0; i < 32; i++) {
		if (interrupts & (1 << i))
			dprintf("%d ", i);
	}

	dprintf("\n\tsupports FSB delivery: %s\n",
			timer->config & HPET_CAP_TIMER_FSB_INT_DEL ? "Yes" : "No");


	dprintf("\n");
	dprintf("Configuration:\n");
	dprintf("\tFSB Enabled: %s\n",
		timer->config & HPET_CONF_TIMER_FSB_ENABLE ? "Yes" : "No");
	dprintf("\tInterrupt Enabled: %s\n",
		timer->config & HPET_CONF_TIMER_INT_ENABLE ? "Yes" : "No");
	dprintf("\tTimer type: %s\n",
		timer->config & HPET_CONF_TIMER_TYPE ? "Periodic" : "OneShot");
	dprintf("\tInterrupt Type: %s\n",
		timer->config & HPET_CONF_TIMER_INT_TYPE ? "Level" : "Edge");

	dprintf("\tconfigured IRQ: %lld\n",
		HPET_GET_CONF_TIMER_INT_ROUTE(timer));

	if (timer->config & HPET_CONF_TIMER_FSB_ENABLE) {
		dprintf("\tfsb_route[0]: 0x%llx\n", timer->fsb_route[0]);
		dprintf("\tfsb_route[1]: 0x%llx\n", timer->fsb_route[1]);
	}
}
#endif


static void
hpet_init_timer(volatile struct hpet_timer *timer)
{
	uint32 interrupts = (uint32)HPET_GET_CAP_TIMER_ROUTE(timer);

	// TODO: Check if the interrupt is already used, and try another
	uint32 interrupt = 0;	
	for (int i = 0; i < 32; i++) {
		if (interrupts & (1 << i)) {
			interrupt = i;		
			break;
		}
	}

	timer->config = 0;

	timer->config |= (interrupt << HPET_CONF_TIMER_INT_ROUTE_SHIFT)
		& HPET_CONF_TIMER_INT_ROUTE_MASK;

	// Non-periodic mode
	timer->config &= ~HPET_CONF_TIMER_TYPE;

	// level triggered
	timer->config |= HPET_CONF_TIMER_INT_TYPE;

	// Disable FSB/MSI, enable 64 bit mode
	timer->config &= ~HPET_CONF_TIMER_FSB_ENABLE;
	timer->config &= ~HPET_CONF_TIMER_32MODE;


#ifdef TRACE_HPET
	hpet_dump_timer(timer);
#endif
}


static status_t
hpet_configure_interrupt(volatile hpet_timer* timer, int32 *irq)
{
	status_t status = B_OK;
	*irq = HPET_GET_CONF_TIMER_INT_ROUTE(timer);
	// TODO: Configure interrupt using msi or regular irqs
	return status;
}


static status_t
hpet_test()
{
	uint64 initialValue = sHPETRegs->u0.counter64;
	spin(10);
	uint64 finalValue = sHPETRegs->u0.counter64;

	if (initialValue == finalValue) {
		dprintf("hpet_test: counter does not increment\n");
		return B_ERROR;
	}

	return B_OK;
}


static status_t
hpet_init()
{
	if (sHPETRegs == NULL)
		return B_NO_INIT;

	sHPETPeriod = HPET_GET_PERIOD(sHPETRegs);

	TRACE(("hpet_init: HPET is at %p.\n\tVendor ID: %llx, rev: %llx, period: %lld\n",
		sHPETRegs, HPET_GET_VENDOR_ID(sHPETRegs), HPET_GET_REVID(sHPETRegs),
		sHPETPeriod));

	status_t status = hpet_set_enabled(false);
	if (status != B_OK)
		return status;

	status = hpet_set_legacy(false);
	if (status != B_OK)
		return status;

	uint32 numTimers = HPET_GET_NUM_TIMERS(sHPETRegs) + 1;

	TRACE(("hpet_init: HPET supports %lu timers, and is %s bits wide.\n",
		numTimers, HPET_IS_64BIT(sHPETRegs) ? "64" : "32"));

	TRACE(("hpet_init: configuration: 0x%llx, timer_interrupts: 0x%llx\n",
		sHPETRegs->config, sHPETRegs->interrupt_status));

	if (numTimers < 3) {
		dprintf("hpet_init: HPET does not have at least 3 timers. Skipping.\n");
		return B_ERROR;
	}

/*
#ifdef TRACE_HPET
	for (uint32 c = 0; c < numTimers; c++)
		hpet_dump_timer(&sHPETRegs->timer[c]);
#endif
*/
	sHPETRegs->interrupt_status = 0;

	status = hpet_set_enabled(true);
	if (status != B_OK)
		return status;

#ifdef TEST_HPET
	status = hpet_test();
	if (status != B_OK)
		return status;
#endif

	return status;
}


////////////////////////////////////////////////////////////////////////////////


status_t
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	sOpenCount = 0;

	status_t status = get_module(B_ACPI_MODULE_NAME, (module_info**)&sAcpi);
	if (status < B_OK)
		return status;

	acpi_hpet *hpetTable;
	status = sAcpi->get_table(ACPI_HPET_SIGNATURE, 0,
		(void**)&hpetTable);
	
	if (status != B_OK) {
		put_module(B_ACPI_MODULE_NAME);
		return status;
	}

	sHPETArea = map_physical_memory("HPET registries",
					hpetTable->hpet_address.address, B_PAGE_SIZE, 0,
					0, (void**)&sHPETRegs);

	if (sHPETArea < 0) {
		put_module(B_ACPI_MODULE_NAME);	
		return sHPETArea;
	}

	return hpet_init();
}


void
uninit_driver(void)
{
	hpet_set_enabled(false);

	if (sHPETArea > 0)
		delete_area(sHPETArea);

	put_module(B_ACPI_MODULE_NAME);
}


const char**
publish_devices(void)
{
	return hpet_name;
}


device_hooks*
find_device(const char* name)
{
	return &hpet_hooks;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark -


status_t
hpet_open(const char* name, uint32 flags, void** cookie)
{
	*cookie = NULL;

	if (sHPETRegs == NULL)
		return B_NO_INIT;

	if (atomic_add(&sOpenCount, 1) != 0) {
		atomic_add(&sOpenCount, -1);
		return B_BUSY;
	}

	hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)malloc(sizeof(hpet_timer_cookie));
	int timerNumber = 2;
	hpetCookie->number = timerNumber;
	hpetCookie->sem = create_sem(0, "hpet_timer 2 sem");
	set_sem_owner(hpetCookie->sem, B_SYSTEM_TEAM);

	hpet_set_enabled(false);

	hpet_init_timer(&sHPETRegs->timer[timerNumber]);
	
	status_t status = hpet_configure_interrupt(&sHPETRegs->timer[timerNumber], &hpetCookie->irq);
	if (status == B_OK)
		status = install_io_interrupt_handler(hpetCookie->irq, &hpet_timer_interrupt, hpetCookie, 0);
	if (status != B_OK)
		dprintf("hpet_open(): cannot install interrupt handler: %s\n", strerror(status));
	else
		dprintf("hpet_open(): HPET timer uses irq %ld\n", hpetCookie->irq);

	hpet_set_enabled(true);

	*cookie = hpetCookie;

	return status;
}


status_t
hpet_close(void* cookie)
{	
	if (sHPETRegs == NULL)
		return B_NO_INIT;

	atomic_add(&sOpenCount, -1);

	hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)cookie;

	dprintf("hpet_close (%d)\n", hpetCookie->number);
	hpet_clear_hardware_timer(&sHPETRegs->timer[hpetCookie->number]);
	remove_io_interrupt_handler(hpetCookie->irq, &hpet_timer_interrupt, hpetCookie);

	return B_OK;
}


status_t
hpet_free(void* cookie)
{	
	if (sHPETRegs == NULL)
		return B_NO_INIT;
	
	hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)cookie;	

	delete_sem(hpetCookie->sem);

	free(cookie);

	return B_OK;
}


status_t
hpet_control(void* cookie, uint32 op, void* arg, size_t length)
{
	hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)cookie;
	status_t status = B_BAD_VALUE;

	switch (op) {
		case HPET_WAIT_TIMER:
		{
			bigtime_t value = *(bigtime_t*)arg;
			dprintf("hpet: wait timer (%d) for %lld...\n", hpetCookie->number, value);
			hpet_set_hardware_timer(value, &sHPETRegs->timer[hpetCookie->number]);
			status = acquire_sem_etc(hpetCookie->sem, 1, B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
			break;
		}
		default:
			break;		
			
	}

	return status;
}


ssize_t
hpet_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	//hpet_timer_cookie* hpetCookie = (hpet_timer_cookie*)cookie;
	*(uint64*)buffer = sHPETRegs->u0.counter64;

	return sizeof(uint64);
}


ssize_t
hpet_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_NOT_ALLOWED;
}

