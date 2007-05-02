#ifndef _FBSD_COMPAT_SYS_SYSTM_H_
#define _FBSD_COMPAT_SYS_SYSTM_H_

#include <stdint.h>

#include <sys/callout.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_media.h>

#define DELAY(n)	snooze(n)

#endif
