#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include "debug.h"
#include "config.h"
#include "../generic/OsSupportBeOS.h"

status_t
init_hardware(void)
{
	LOG_CREATE();
	if (B_OK == probe_device()) {
		PRINT(("ALL YOUR BASE ARE BELONG TO US\n"));
		return B_OK;
	} else {
		LOG(("hardware not found\n"));
		return B_ERROR;
	}
}

status_t
init_driver(void)
{
	status_t rv;
	bigtime_t start;
	bool s0cr, s1cr, s2cr;

	LOG_CREATE();
	
	LOG(("init_driver\n"));
	
	ASSERT(sizeof(ich_bd) == 8);
	
	rv = probe_device();
	if (rv != B_OK) {
		LOG(("No supported audio hardware found.\n"));
		return B_ERROR;
	}

	PRINT((VERSION "\n"));
	PRINT(("found %s\n", config->name));
	PRINT(("IRQ = %ld, NAMBAR = %#lX, NABMBAR = %#lX, MMBAR = %#lX, MBBAR = %#lX\n",config->irq,config->nambar,config->nabmbar,config->mmbar,config->mbbar));

}

void
uninit_driver(void)
{
	LOG(("uninit_driver()\n"));

	#if DEBUG
		if (chan_po) LOG(("chan_po frames_count = %Ld\n",chan_po->played_frames_count));
		if (chan_pi) LOG(("chan_pi frames_count = %Ld\n",chan_pi->played_frames_count));
		if (chan_mc) LOG(("chan_mc frames_count = %Ld\n",chan_mc->played_frames_count));
	#endif

	LOG(("uninit_driver() finished\n"));
}

static const char *gals_names[] = {
	"audio/multi/darla20",
	"audio/multi/gina20",
	"audio/multi/layla20",
	"audio/multi/darla24",
	NULL
}

const char **
publish_devices(void) {
	return gals_names;
}

device_hooks gals_hooks = {
	gals_open, 			/* -> open entry point */
	gals_close, 		/* -> close entry point */
	gals_free,			/* -> free cookie */
	gals_control, 		/* -> control entry point */
	gals_read,			/* -> read entry point */
	gals_write,			/* -> write entry point */
	NULL,				/* start select */
	NULL,				/* stop select */
	NULL,				/* scatter-gather read from the device */
	NULL				/* scatter-gather write to the device */
};


device_hooks *
find_device(const char * name) {
	return &gals_hooks;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;
