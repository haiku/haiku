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
#ifndef _DEVICE_INFO_H_ 
	#define _DEVICE_INFO_H_

#ifndef _SCSI_COMMANDS_H_
	#include "scsi_commands.h"
#endif /*_SCSI_COMMANDS_H_*/ 

#ifndef _PROTO_MODULE_H_ 
	#include "proto_module.h"
#endif /* _PROTO_MODULE_H_ */ 

typedef struct _usb_device_info{
	uint8 dev_num;				/**/
	const usb_device device;	/**/
	uint16 interface;			/**/
	uint8	max_lun;			/**/
	
	uint32 properties;
	
	usb_pipe pipe_in;			/**/
	usb_pipe pipe_out;			/**/
	usb_pipe pipe_intr;			/**/
	
	sem_id lock_sem;			/**/
	sem_id trans_sem;			/**/
	uint32 tag;					/**/
	status_t status;			/**/
	
	bigtime_t trans_timeout;
	usb_module_info *usb_m;
	
	void *data;					/**/
	int actual_len;				/**/
	
	protocol_module_info *protocol_m;
	char *protocol_m_path;
	
	transform_module_info *transform_m;
	char *transform_m_path;
	
	bool b_trace;
	void (*trace)(bool b_force, const char *fmt, ...);
	void (*trace_bytes)(const char *prefix, const uint8 *bytes, size_t bytes_len);

	uint8 scsi_command_buf[IOCDBLEN];
	/* auto sense buffer. Some commands doesn't have it. emulate */
	scsi_sense_data autosense_data;
	/* iovec					 autosense_sg; DO NOT RESTORE IT !!!*/
	uint8 not_ready_luns;
} usb_device_info;

#endif /* _DEVICE_INFO_H_ */

