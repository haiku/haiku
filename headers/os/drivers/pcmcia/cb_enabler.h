/*
 * cb_enabler.h
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
 */

#ifndef _CB_ENABLER_H
#define _CB_ENABLER_H

#define __KERNEL__

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#undef __KERNEL__

#include <PCI.h>

#define CB_ENABLER_MODULE_NAME	"bus_managers/cb_enabler/v0"

typedef struct cb_device_descriptor cb_device_descriptor;
typedef struct cb_notify_hooks cb_notify_hooks;
typedef struct cb_enabler_module_info cb_enabler_module_info;

struct cb_device_descriptor
{
	uint16 vendor_id;  /* 0xffff is don't-care */
	uint16 device_id;  /* 0xffff is don't-care */
	uint8 class_base;  /* 0xff is don't-care */
	uint8 class_sub;   /* 0xff is don't-care */
	uint8 class_api;   /* 0xff is don't-care */
};

struct cb_notify_hooks
{
	status_t (*device_added)(pci_info *pci, void **cookie);
	void (*device_removed)(void *cookie);
};

struct cb_enabler_module_info {
    bus_manager_info	binfo;
	
	void (*register_driver)(const char *name, 
							const cb_device_descriptor *descriptors, 
							size_t count);
	void (*install_notify)(const char *name, 
						   const cb_notify_hooks *hooks);
	void (*uninstall_notify)(const char *name, 
							  const cb_notify_hooks *hooks);
};

#endif
