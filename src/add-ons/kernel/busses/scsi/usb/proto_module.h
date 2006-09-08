/**
 *
 * TODO: description
 * 
 * This file is a part of USB SCSI CAM for Haiku OS.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 * 	
 * 	
 */
#ifndef _PROTO_MODULE_H_ 
	#define _PROTO_MODULE_H_

#ifndef _MODULE_H
	#include <module.h>
#endif /*_MODULE_H*/

/*#ifndef _SG_BUFFER_H_
	#include "sg_buffer.h"
#endif / *_SG_BUFFER_H_*/

enum {
	/* B_OK */											 /* JFYI:command is OK */	
	B_CMD_FAILED = B_ERRORS_END + 1, /* command failed */
	B_CMD_WIRE_FAILED,							 /* device problems */
	B_CMD_UNKNOWN,									 /* command state unknown */
};

typedef enum{
	eDirNone = 0,
	eDirIn,
	eDirOut,
} EDirection;

struct _usb_device_info; /* forward, we can be included from device_info.h */

typedef void (*ud_transfer_callback)(struct _usb_device_info *udi,
									 CCB_SCSIIO *ccbio,
 									 int32 residue,
									 status_t status);

typedef struct {
	module_info	module;
	
	status_t		 (*init)(struct _usb_device_info *udi);					
	status_t		(*reset)(struct _usb_device_info *udi);					
	void		 (*transfer)(struct _usb_device_info *udi,
							 uint8 *cmd, uint8	cmdlen,
							 //sg_buffer *sgb,
							 iovec *sg_data,
							 int32	sg_count,
							 int32 transfer_len,
							 EDirection dir,
							 CCB_SCSIIO *ccbio,
							 ud_transfer_callback cb);					
} protocol_module_info;


typedef struct {
	module_info	module;

	status_t (*transform)(struct _usb_device_info *udi,
							uint8	*cmd, uint8	 len,
							uint8 **rcmd, uint8	*rlen);					
} transform_module_info;

#define MODULE_PREFIX		"generic/usb_scsi_extensions/" 
#define PROTOCOL_SUFFIX		"/protocol/v1"
#define TRANSFORM_SUFFIX 	"/transform/v1"
#define PROTOCOL_MODULE_MASK	MODULE_PREFIX"%s"PROTOCOL_SUFFIX
#define TRANSFORM_MODULE_MASK	MODULE_PREFIX"%s"TRANSFORM_SUFFIX

/**
	\define:_TRACE_ALWAYS
	trace always - used mainly for error messages 
*/
#define PTRACE_ALWAYS(__udi, __x...) \
		{ /*if(__udi->b_trace)*/ __udi->trace(true, __x); }
/**
	\define:TRACE
	trace only if logging is activated 
*/
#define PTRACE(__udi, __x...) \
		{ if(__udi->b_trace) __udi->trace(false, __x); }

#endif /* _PROTO_MODULE_H_ */

