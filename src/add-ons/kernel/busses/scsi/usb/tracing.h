/**
 *
 * TODO: description
 *
 * This file is a part of USB SCSI CAM for Haiku.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 *
 *
 */
/** tracing support definitions */

#ifndef _USB_SCSI_TRACING_H_
	#define _USB_SCSI_TRACING_H_

#ifndef _SG_BUFFER_H_
	#include "sg_buffer.h"
#endif /*_SG_BUFFER_H_*/

void load_log_settings(void *sh);
void create_log(void);
void usb_scsi_trace(bool b_force, const char *fmt, ...);

/* non-switchable tracng functions */
void usb_scsi_trace_bytes(const char *prefix, const uint8 *bytes, size_t bytes_len);
void usb_scsi_trace_sgb(const char *prefix, sg_buffer *sgb);
void usb_scsi_trace_CCB_HEADER(const CCB_HEADER *ccb);
void usb_scsi_trace_CCB_SCSIIO(const CCB_SCSIIO *ccbio);

/* ---------------------- Generic tracing --------------------------------------- */
/**
	\define:TRACE_ALWAYS
	trace always - used mainly for error messages
*/
#define TRACE_ALWAYS(x...) \
		usb_scsi_trace(true, x)
/**
	\define:TRACE
	trace only if logging is activated
*/
#define TRACE(x...) \
		usb_scsi_trace(false, x)

/* ---------------------- SCSI commands tracing -------------------------------- */
extern bool b_log_scsi_cmd;
void usb_scsi_trace_command(bool b_hlight, const uint8 *cmd, size_t cmdlen);
/**
	\define:TRACE_SCSI_COMMAND
	trace SCSI command
*/
#define TRACE_SCSI_COMMAND(cmd, cmdlen)\
		{ if(b_log_scsi_cmd) usb_scsi_trace_command(false, cmd, cmdlen); }
/**
	\define:TRACE_SCSI_COMMAND_HLIGHT
	trace SCSI command and prefixes it with special sign. Used for tracing
	commands internally converted by this module.
*/
#define TRACE_SCSI_COMMAND_HLIGHT(cmd, cmdlen)\
		{ if(b_log_scsi_cmd) usb_scsi_trace_command(true, cmd, cmdlen); }

/* ---------------------- USB callback tracing -------------------------------- */
/**
	\define:TRACE_BULK_CALLBACK
	trace the bulk_callback procedures data and status flow.
*/
extern bool b_log_bulk_cb;
#define TRACE_BULK_CALLBACK(stat, len)\
		{ if(b_log_bulk_cb) TRACE("B_CB:status:%08x;length:%d\n", stat, len); }

/* --------------- USB mass storage commands tracing --------------------------- */
extern bool b_log_protocol;
/* --------------------------- data i/o tracing -------------------------------- */
extern bool b_log_data_processing;
void usb_scsi_trace_SG(iovec *sg, int count);
/**
	\define:TRACE_DATA_IO
	trace the information about amount processed data and status of process.
*/
#define TRACE_DATA_IO(x...)\
		{ if(b_log_data_processing) usb_scsi_trace(false, x); }
#define TRACE_DATA_IO_SG(sg, cnt)\
		{ if(b_log_data_processing) usb_scsi_trace_SG(sg, cnt); }

extern bool b_log_sense_data;
/**
	\define:TRACE_SENSE_DATA
	trace the information REQUEST SENSE data.
*/
#define TRACE_SENSE_DATA(data, len)\
		{ if(b_log_sense_data) usb_scsi_trace_bytes("SENSE:", data, len); }
#define TRACE_MODE_SENSE_DATA(prefix, data, len)\
		{ if(b_log_sense_data) usb_scsi_trace_bytes(prefix, data, len); }
#define TRACE_MODE_SENSE_SGB(prefix, data)\
		{ if(b_log_sense_data) usb_scsi_trace_sgb(prefix, data); }

extern bool b_log_capacity;
#define TRACE_CAPACITY(prefix, data)\
		{ if(b_log_capacity) usb_scsi_trace_sgb(prefix, data); }

#endif /*_USB_SCSI_TRACING_H_*/

