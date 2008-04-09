/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Drivers.h>
#include <OS.h>
#include <bus_manager.h>
#include <malloc.h>
#include <module.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioccom.h>
#include <sys/ioctl.h>

#define __KERNEL__
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#define copy_from_user memcpy 
#define copy_to_user memcpy
#define CardServices gPcmciaCs->_CardServices
#define get_handle gPcmciaDs->get_handle

const char sockname[] = "bus/pcmcia/sock/%ld";
static char ** devices;
uint32 devices_count = 0;

int32 api_version = B_CUR_DRIVER_API_VERSION;

cs_client_module_info *gPcmciaCs;
ds_module_info *gPcmciaDs;

static status_t
ds_open(const char *name, uint32 flags, void **_cookie)
{
	int32 socket = -1;
	int32 i;
	*_cookie = NULL;
	
	for (i=0; i<devices_count; i++) {
		if (strcmp(name, devices[i]) == 0) {
			socket = i;
			break;
		}
	}
	
	if (socket < 0) {
		return B_BAD_VALUE;
	}
	
	if (get_handle(socket, (client_handle_t *)_cookie) != B_OK) {
		return ENODEV;
	}
	
	return B_OK;
}


static status_t
ds_close(void *cookie)
{
	return B_OK;
}


static status_t
ds_free(void *cookie)
{
	return B_OK;
}


static status_t
ds_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	return B_ERROR;
}


static status_t
ds_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	return B_ERROR;
}


/*====================================================================*/

static status_t 
ds_ioctl(void *cookie, uint32 cmd, void *arg, size_t len)
{
	/*socket_info_t *s = (socket_info_t *) cookie;
    u_int size = IOCPARM_LEN(cmd);
    status_t ret, err;
    ds_ioctl_arg_t buf;
    
    if (size > sizeof(ds_ioctl_arg_t)) return -EINVAL;
    
	err = ret = 0;
    
    if (cmd & IOC_IN) copy_from_user((char *)&buf, (char *)arg, size);
    
    switch (cmd) {
    case DS_ADJUST_RESOURCE_INFO:
	ret = CardServices(AdjustResourceInfo, s->handle, &buf.adjust);
	break;
    case DS_GET_CARD_SERVICES_INFO:
	ret = CardServices(GetCardServicesInfo, &buf.servinfo);
	break;
    case DS_GET_CONFIGURATION_INFO:
	ret = CardServices(GetConfigurationInfo, s->handle, &buf.config);
	break;
    case DS_GET_FIRST_TUPLE:
	ret = CardServices(GetFirstTuple, s->handle, &buf.tuple);
	break;
    case DS_GET_NEXT_TUPLE:
	ret = CardServices(GetNextTuple, s->handle, &buf.tuple);
	break;
    case DS_GET_TUPLE_DATA:
	buf.tuple.TupleData = buf.tuple_parse.data;
	buf.tuple.TupleDataMax = sizeof(buf.tuple_parse.data);
	ret = CardServices(GetTupleData, s->handle, &buf.tuple);
	break;
    case DS_PARSE_TUPLE:
	buf.tuple.TupleData = buf.tuple_parse.data;
	ret = CardServices(ParseTuple, s->handle, &buf.tuple,
			   &buf.tuple_parse.parse);
	break;
    case DS_RESET_CARD:
	ret = CardServices(ResetCard, s->handle, NULL);
	break;
    case DS_GET_STATUS:
	ret = CardServices(GetStatus, s->handle, &buf.status);
	break;
    case DS_VALIDATE_CIS:
	ret = CardServices(ValidateCIS, s->handle, &buf.cisinfo);
	break;
    case DS_SUSPEND_CARD:
	ret = CardServices(SuspendCard, s->handle, NULL);
	break;
    case DS_RESUME_CARD:
	ret = CardServices(ResumeCard, s->handle, NULL);
	break;
    case DS_EJECT_CARD:
	ret = CardServices(EjectCard, s->handle, NULL);
	break;
    case DS_INSERT_CARD:
	ret = CardServices(InsertCard, s->handle, NULL);
	break;
    case DS_ACCESS_CONFIGURATION_REGISTER:
	if ((buf.conf_reg.Action == CS_WRITE) && !capable(CAP_SYS_ADMIN))
	    return -EPERM;
	ret = CardServices(AccessConfigurationRegister, s->handle,
			   &buf.conf_reg);
	break;
    case DS_GET_FIRST_REGION:
        ret = CardServices(GetFirstRegion, s->handle, &buf.region);
	break;
    case DS_GET_NEXT_REGION:
	ret = CardServices(GetNextRegion, s->handle, &buf.region);
	break;
    case DS_GET_FIRST_WINDOW:
	buf.win_info.handle = (window_handle_t)s->handle;
	ret = CardServices(GetFirstWindow, &buf.win_info.handle,
			   &buf.win_info.window);
	break;
    case DS_GET_NEXT_WINDOW:
	ret = CardServices(GetNextWindow, &buf.win_info.handle,
			   &buf.win_info.window);
	break;
    case DS_GET_MEM_PAGE:
	ret = CardServices(GetMemPage, buf.win_info.handle,
			   &buf.win_info.map);
	break;
    case DS_REPLACE_CIS:
	ret = CardServices(ReplaceCIS, s->handle, &buf.cisdump);
	break;
	*/
	client_handle_t h = (client_handle_t) cookie;
    u_int size = IOCPARM_LEN(cmd);
    status_t ret, err;
    ds_ioctl_arg_t buf;
    
    if (size > sizeof(ds_ioctl_arg_t)) return -EINVAL;
    
	err = ret = 0;
	
    if (cmd & IOC_IN) copy_from_user((char *)&buf, (char *)arg, size);
    
    switch (cmd) {
    case DS_ADJUST_RESOURCE_INFO:
	ret = CardServices(AdjustResourceInfo, h, &buf.adjust);
	break;
    case DS_GET_CARD_SERVICES_INFO:
	ret = CardServices(GetCardServicesInfo, &buf.servinfo);
	break;
    case DS_GET_CONFIGURATION_INFO:
	ret = CardServices(GetConfigurationInfo, h, &buf.config);
	break;
    case DS_GET_FIRST_TUPLE:
	ret = CardServices(GetFirstTuple, h, &buf.tuple);
	break;
    case DS_GET_NEXT_TUPLE:
	ret = CardServices(GetNextTuple, h, &buf.tuple);
	break;
    case DS_GET_TUPLE_DATA:
	buf.tuple.TupleData = buf.tuple_parse.data;
	buf.tuple.TupleDataMax = sizeof(buf.tuple_parse.data);
	ret = CardServices(GetTupleData, h, &buf.tuple);
	break;
    case DS_PARSE_TUPLE:
	buf.tuple.TupleData = buf.tuple_parse.data;
	ret = CardServices(ParseTuple, h, &buf.tuple,
			   &buf.tuple_parse.parse);
	break;
    case DS_RESET_CARD:
	ret = CardServices(ResetCard, h, NULL);
	break;
    case DS_GET_STATUS:
	ret = CardServices(GetStatus, h, &buf.status);
	break;
    case DS_VALIDATE_CIS:
	ret = CardServices(ValidateCIS, h, &buf.cisinfo);
	break;
    case DS_SUSPEND_CARD:
	ret = CardServices(SuspendCard, h, NULL);
	break;
    case DS_RESUME_CARD:
	ret = CardServices(ResumeCard, h, NULL);
	break;
    case DS_EJECT_CARD:
	ret = CardServices(EjectCard, h, NULL);
	break;
    case DS_INSERT_CARD:
	ret = CardServices(InsertCard, h, NULL);
	break;
/*    case DS_ACCESS_CONFIGURATION_REGISTER:
	if ((buf.conf_reg.Action == CS_WRITE) && !capable(CAP_SYS_ADMIN))
	    return -EPERM;
	ret = CardServices(AccessConfigurationRegister, h,
			   &buf.conf_reg);
	break;
    case DS_GET_FIRST_REGION:
        ret = CardServices(GetFirstRegion, h, &buf.region);
	break;
    case DS_GET_NEXT_REGION:
	ret = CardServices(GetNextRegion, h, &buf.region);
	break;
    case DS_GET_FIRST_WINDOW:
	buf.win_info.handle = (window_handle_t)h;
	ret = CardServices(GetFirstWindow, &buf.win_info.handle,
			   &buf.win_info.window);
	break;
    case DS_GET_NEXT_WINDOW:
	ret = CardServices(GetNextWindow, &buf.win_info.handle,
			   &buf.win_info.window);
	break;
    case DS_GET_MEM_PAGE:
	ret = CardServices(GetMemPage, buf.win_info.handle,
			   &buf.win_info.map);
	break;*/
    case DS_REPLACE_CIS:
	ret = CardServices(ReplaceCIS, h, &buf.cisdump);
	break;
	
/*    case DS_BIND_REQUEST:
	if (!capable(CAP_SYS_ADMIN)) return -EPERM;
	err = bind_request(i, &buf.bind_info);
	break;
    case DS_GET_DEVICE_INFO:
	err = get_device_info(i, &buf.bind_info, 1);
	break;
    case DS_GET_NEXT_DEVICE:
	err = get_device_info(i, &buf.bind_info, 0);
	break;
    case DS_UNBIND_REQUEST:
	err = unbind_request(i, &buf.bind_info);
	break;
    case DS_BIND_MTD:
	if (!capable(CAP_SYS_ADMIN)) return -EPERM;
	err = bind_mtd(i, &buf.mtd_info);
	break;*/
    default:
    err = -EINVAL;
    }
    
    if ((err == 0) && (ret != CS_SUCCESS)) {
	switch (ret) {
	case CS_BAD_SOCKET: case CS_NO_CARD:
	    err = ENODEV; break;
	case CS_BAD_ARGS: case CS_BAD_ATTRIBUTE: case CS_BAD_IRQ:
	case CS_BAD_TUPLE:
		err = EINVAL; break;
	case CS_IN_USE:
	    err = EBUSY; break;
	case CS_OUT_OF_RESOURCE:
	    err = ENOSPC; break;
	case CS_NO_MORE_ITEMS:
	    err = ENODATA; break;
	case CS_UNSUPPORTED_FUNCTION:
	    err = ENOSYS; break;
	default:
	    err = EIO; break;
	}
    }
    
    if (cmd & IOC_OUT) copy_to_user((char *)arg, (char *)&buf, size);
     
    return err;
} /* ds_ioctl */


status_t
init_hardware()
{
	return B_OK;
}


const char **
publish_devices(void)
{
	return (const char **)devices;
}


static device_hooks hooks = {
	&ds_open,
	&ds_close,
	&ds_free,
	&ds_ioctl,
	&ds_read,
	&ds_write,
	NULL,
	NULL,
	NULL,
	NULL
};

device_hooks *
find_device(const char *name)
{
	return &hooks;
}


status_t
init_driver()
{
	status_t err;
	client_handle_t handle;
	uint32 i;
	
#if DEBUG && !defined(__HAIKU__)
	load_driver_symbols("ds");
#endif

	if ((err = get_module(CS_CLIENT_MODULE_NAME, (module_info **)&gPcmciaCs))!=B_OK)
		return err;
	if ((err = get_module(DS_MODULE_NAME, (module_info **)&gPcmciaDs))!=B_OK) {
		put_module(CS_CLIENT_MODULE_NAME);
		return err;
	}
	
	devices_count = 0;
	while (get_handle(devices_count, &handle)==B_OK) {
		devices_count++;
	}
	
	if (devices_count <= 0) {
		put_module(CS_CLIENT_MODULE_NAME);
		put_module(DS_MODULE_NAME);
		return ENODEV;
	}
	
	devices = malloc(sizeof(char *) * (devices_count+1));
	if (!devices) {
		put_module(CS_CLIENT_MODULE_NAME);
		put_module(DS_MODULE_NAME);
		return ENOMEM;
	}
	for (i=0; i<devices_count; i++) {
		devices[i] = strdup(sockname);
		sprintf(devices[i], sockname, i);
	}
	devices[devices_count] = NULL;
	
	return B_OK;
}


void
uninit_driver()
{
	int32 i = 0;
	for (i=0; i<devices_count; i++) {
		free (devices[i]);
	}
	free(devices);
	devices = NULL;

	put_module(DS_MODULE_NAME);
	put_module(CS_CLIENT_MODULE_NAME);
}

