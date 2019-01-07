/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "sysinit.h"

#include <sys/cdefs.h>
#include <sys/kernel.h>


//#define TRACE_SYSINIT
#ifdef TRACE_SYSINIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


/* linker sets */
SET_DECLARE(__freebsd_sysinit, struct sysinit);
SET_DECLARE(__freebsd_sysuninit, struct sysinit);

/* make sure there's something in both linker sets, so we can link */
SYSINIT(__dummy, 0, 0, NULL, NULL);
SYSUNINIT(__dummy, 0, 0, NULL, NULL);

/* orders */
static const enum sysinit_elem_order orders[6] = {
	SI_ORDER_FIRST, SI_ORDER_SECOND, SI_ORDER_THIRD, SI_ORDER_FOURTH,
	SI_ORDER_MIDDLE, SI_ORDER_ANY,
};


void
init_sysinit()
{
	struct sysinit* const* initee;
	int32 i;

	for (i = 0; i < 6; i++) {
		SET_FOREACH(initee, __freebsd_sysinit) {
			if ((*initee)->order != orders[i] || (*initee)->func == NULL)
				continue;
			TRACE("sysinit: %d, %d, %s\n", orders[i], (*initee)->order,
				(*initee)->name);
			(*initee)->func((*initee)->arg);
		}
	}
}


void
uninit_sysinit()
{
	struct sysinit* const* initee;
	int32 i;

	for (i = 5; i >= 0; i--) {
		SET_FOREACH(initee, __freebsd_sysuninit) {
			if ((*initee)->order != orders[i] || (*initee)->func == NULL)
				continue;
			TRACE("sysinit: de-initializing %s %p\n", (*initee)->name, (*initee)->func);
			(*initee)->func((*initee)->arg);
		}
	}
}
