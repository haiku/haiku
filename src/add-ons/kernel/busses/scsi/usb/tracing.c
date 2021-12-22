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
/** tracing support implementation */

#include "usb_scsi.h"
#include "tracing.h"

#include <stdio.h>
#include <unistd.h> /* posix file i/o - create, write, close */
#include <malloc.h>
#include <string.h>
#include <driver_settings.h>

/** private log path name */
static const char *private_log_path = "/var/log/"MODULE_NAME".log";

static const char *log_port_name	= MODULE_NAME"-logging";

#ifdef BUILD_LOG_DAEMON

int main(int argc, char**argv)
{
	if(B_NAME_NOT_FOUND == find_port(log_port_name)){
		bool b_screen = (argc > 1);
		port_id pid = create_port(1000, log_port_name);
		while(true){
			int32 code;
			char buffer[1024 + 1];
			size_t sz = read_port_etc(pid, &code, buffer, 1024, B_TIMEOUT, 1000 * 1000);
			if(sz != B_TIMED_OUT){
				if(b_screen){
					buffer[sz] = 0;
					printf(buffer);
				} else {
					FILE *f = fopen(private_log_path, "a");
					fwrite(buffer, sz, 1, f);
					fclose(f);
				}
			}
		}
	}
	return 0;
}

#else /* BUILD_LOG_DAEMON */

/** is activity logging on or off? */
#if DEBUG
bool b_log = true;
#else
bool b_log = false;
#endif
/** logging in private file */
bool b_log_file		= false;
/** appending logs to previous log sessions */
bool b_log_append	= false;
/** log SCSI commands */
bool b_log_scsi_cmd = false;
/** log USB bulk callback function activity */
bool b_log_bulk_cb	= false;

bool b_log_protocol = false;
//bool b_log_CSW = false;
//bool b_log_CDB = false;
bool b_log_data_processing = false;
bool b_log_sense_data	   = false;
/** log time at wich logging event occured */
static bool b_log_time	   = false;
/** log thread id from wich logging performed */
static bool b_log_thid	   = false;
/** log thread name from wich logging performed */
static bool b_log_thname   = false;
/** semaphore id used to synchronizing logging requests */
//static sem_id loglock;
/** log result of READ_CAPACITY command */
bool b_log_capacity = false;


/**
	\fn: load_log_settings
	\param sh: driver_settings handle
	called from main driver settings loading procedure to load log-related
	parameters
*/
void
load_log_settings(void *sh)
{
	if(sh){
#if !DEBUG
		b_log			= get_driver_boolean_parameter(sh, "debug_output", b_log, true);
#endif
		b_log_file		= get_driver_boolean_parameter(sh, "debug_output_in_file",
																														 b_log_file, true);
		b_log_append	= ! get_driver_boolean_parameter(sh, "debug_output_file_rewrite",
																														!b_log_append, true);
		b_log_time		= get_driver_boolean_parameter(sh, "debug_trace_time",
																														 b_log_time, false);
		b_log_thid		= get_driver_boolean_parameter(sh, "debug_trace_threadid",
																														 b_log_thid, false);
		b_log_thname	= get_driver_boolean_parameter(sh, "debug_trace_thread_name",
																														 b_log_thname, false);
		b_log_scsi_cmd	= get_driver_boolean_parameter(sh, "debug_trace_commands",
																														 b_log_scsi_cmd, false);
		b_log_bulk_cb	= get_driver_boolean_parameter(sh, "debug_trace_bulk_callback",
																														 b_log_bulk_cb, false);
		b_log_protocol	= get_driver_boolean_parameter(sh, "debug_trace_protocol",
																														 b_log_protocol, false);
		b_log_data_processing =
						get_driver_boolean_parameter(sh, "debug_trace_data_io",
										b_log_data_processing, false);
		b_log_sense_data =
						get_driver_boolean_parameter(sh, "debug_trace_sense_data",
										b_log_sense_data, false);
		b_log_capacity	= get_driver_boolean_parameter(sh, "debug_trace_capacity",
										b_log_capacity, false);
		if(!b_log_file){ /* some information is already in system log entries */
			b_log_thid	 = false;
			b_log_thname = false;
		}
	}
}
/**
	\fn: create_log
	called to [re]create private log file
*/
void
create_log(void)
{
	int flags = O_WRONLY | O_CREAT | ((!b_log_append) ? O_TRUNC : 0);
	if(b_log_file){
		close(open(private_log_path, flags, 0666));
	}
}

/**
	\fn: usb_scsi_trace
	\param b_force: if true - output message ignoring current logging state
	\param fmt: format string used in message formatting
	\param ...: variable argument used in message formatting
	formats and traces messages to current debug output target.
*/
void
usb_scsi_trace(bool b_force, const char *fmt, ...)
{
	if(b_force || b_log){
		va_list arg_list;
		static char *prefix = MODULE_NAME":";
		static char buf[1024];
		char *buf_ptr = buf;
		port_id pid = find_port(log_port_name);
		bool b_no_port = (pid == B_NAME_NOT_FOUND);
		if(!b_log_file || b_no_port){ /* it's not a good idea to add prefix in private file */
			strcpy(buf, prefix);
			buf_ptr += strlen(prefix);
		}
		if(b_log_time){ /* add time of logging this string */
			bigtime_t time = system_time();
			uint32 msec = time / 1000;
			uint32 sec	= msec / 1000;
			sprintf(buf_ptr, "%02ld.%02ld.%03ld:", sec / 60, sec % 60, msec % 1000);
			buf_ptr += strlen(buf_ptr);
		}
		if(b_log_thid){ /* add id of the calling thread */
			thread_id tid = find_thread(0);
			thread_info tinfo = {0};
			if(b_log_thname){ /* add name of the calling thread */
				get_thread_info(tid, &tinfo);
			}
			sprintf(buf_ptr, "['%s':%lx]:", tinfo.name, tid);
			buf_ptr += strlen(buf_ptr);
		}
		va_start(arg_list, fmt);
		vsprintf(buf_ptr, fmt, arg_list);
		va_end(arg_list);
		if(b_log_file && !b_no_port){ /* write in private log file */
			if(B_OK != write_port_etc(pid, 0, buf, strlen(buf), B_TIMEOUT, 0))
				dprintf("%s", buf);
		} else /* write in system log*/
			dprintf("%s", buf);
	}
}
/**
	\fn: trace_CCB_HEADER
	\param ccbh: struct to be traced
	traces CCB_HEADER struct to current debug output target
*/
void
usb_scsi_trace_CCB_HEADER(const CCB_HEADER *ccbh)
{
	TRACE("CCB_HEADER:%08x\n"
		"	phys_addr:%ld\n"
		"	cam_ccb_len:%d\n"
		"	cam_func_code:%02x\n"
		"	cam_status:%02x\n"
		"	cam_hrsvd0:%02x\n"
		"	cam_path_id:%02x\n"
		"	cam_target_id:%02x\n"
		"	cam_target_lun:%02x\n"
		"	cam_flags:%08x\n",
			ccbh,
			ccbh->phys_addr,
			ccbh->cam_ccb_len,
			ccbh->cam_func_code,
			ccbh->cam_status,
			ccbh->cam_hrsvd0,
			ccbh->cam_path_id,
			ccbh->cam_target_id,
			ccbh->cam_target_lun,
			ccbh->cam_flags);
}
/**
	\fn: trace_CCB_SCSIIO
	\param ccbio: struct to be traced
	traces CCB_SCSIIO struct to current debug output target
*/
void
usb_scsi_trace_CCB_SCSIIO(const CCB_SCSIIO *ccbio)
{
	TRACE("CCB_SCSIIO:%08x\n", ccbio);
	usb_scsi_trace_CCB_HEADER(&ccbio->cam_ch);
	TRACE("	cam_pdrv_ptr:%08x\n"
		"	cam_next_ccb:%08x\n"
		"	cam_req_map:%08x\n"
		"	(*cam_cbfcnp):%08x\n"
		"	cam_data_ptr:%08x\n"
		"	cam_dxfer_len:%ld\n"
		"	cam_sense_ptr:%08x\n"
		"	cam_sense_len:%d\n"
		"	cam_cdb_len:%d\n"
		"	cam_sglist_cnt:%d\n"
		"	cam_sort:%d\n"
		"	cam_scsi_status:%02x\n"
		"	cam_sense_resid:%d\n"
		"	cam_resid:%d\n"
		"	cam_timeout:%d\n"
		"	cam_msg_ptr:%08x\n"
		"	cam_msgb_len:%d\n"
		"	cam_vu_flags:%04x\n"
		"	cam_tag_action:%d\n",
	ccbio->cam_pdrv_ptr,
	ccbio->cam_next_ccb,
	ccbio->cam_req_map,
	ccbio->cam_cbfcnp,
	ccbio->cam_data_ptr,
	ccbio->cam_dxfer_len,
	ccbio->cam_sense_ptr,
	ccbio->cam_sense_len,
	ccbio->cam_cdb_len,
	ccbio->cam_sglist_cnt,
	ccbio->cam_sort,
	ccbio->cam_scsi_status,
	ccbio->cam_sense_resid,
	ccbio->cam_resid,
	ccbio->cam_timeout,
	ccbio->cam_msg_ptr,
	ccbio->cam_msgb_len,
	ccbio->cam_vu_flags,
	ccbio->cam_tag_action);

	usb_scsi_trace_bytes("CDB_UN:\n", ccbio->cam_cdb_io.cam_cdb_bytes, IOCDBLEN);
}
/**
	\fn: usb_scsi_command_trace
	\param b_hlight: highlight command and prefix it with spec. charachter
	\param cmd: array of bytes to be traced. typically pointer SCSI command buffer
	\param cmdlen: size of buffer in cmd parameter
	traces SCSI commands into debug output target.can highlight and prefix the
	text with special charachter and color for two different types
	of commands.
*/
void
usb_scsi_trace_command(bool b_hlight, const uint8 *cmd, size_t cmdlen)
{
	size_t len = min(cmdlen, 12); /* command length watchdog */
	char hl_mask[] = "\33[%sCMD:\33[0m";
	char prefix[sizeof(hl_mask) + 6];
	if(b_log_file){ /* compose CMD prefix */
		sprintf(prefix, "%sCMD:", b_hlight ? "=>":"");
	} else {
		sprintf(prefix, hl_mask, b_hlight ? "33m=>":"32m");
	}
	usb_scsi_trace_bytes(prefix, cmd, len); /* append command bytes to log */
}
/**
	\fn:usb_scsi_bytes_trace
	\param bytes:array of bytes to be traced.
	\param bytes_len: size of buffer in bytes parameter
	traces buffer bytes one by one into debug output target.
*/
void
usb_scsi_trace_bytes(const char *prefix, const uint8 *bytes, size_t bytes_len)
{
	size_t len = min(bytes_len, 0x100); /* length watchdog */
	char truncTail[] = "<TRUNC>\n";
	char *pbuf = (char *)malloc(len * 3 + sizeof(truncTail) + strlen(prefix) + 1);
	if(pbuf){
		size_t i = 0;
		char *p = pbuf;
		strcpy(p, prefix);
		p += strlen(prefix);
		for(;i < len; i++){
			sprintf(p, " %02x", bytes[i]);
			p += strlen(p);
		}
		/* user should be informed about truncation too*/
		strcpy(p, (len < bytes_len) ? truncTail : "\n");
		TRACE(pbuf);
		free(pbuf);
	} else {
		TRACE_ALWAYS("usb_scsi_trace_bytes:error allocate "
																			"memory for tracing %d bytes\n", len);
	}
}

void usb_scsi_trace_sgb(const char *prefix, sg_buffer *sgb)
{
	char sgbHead[] = "SGB:";
	size_t i = 0;
	size_t len = strlen(prefix) + strlen(sgbHead) + sgb->count * 9;
	char *sgbPrefix = (char*)malloc(len);
	if(0 != sgbPrefix){
		sg_buffer sgb_own;
		size_t sgb_len = 0;
		char *p = sgbPrefix;
		strcpy(p, prefix);
		p += strlen(p);
		strcpy(p, sgbHead);
		p += strlen(p);
		i = 0;
		for(; i < sgb->count; i++){
			sprintf(p, "%lX,", sgb->piov[i].iov_len);
			sgb_len += sgb->piov[i].iov_len;
			p += strlen(p);
		}
		if(B_OK == realloc_sg_buffer(&sgb_own, sgb_len)){
			sg_memcpy(&sgb_own, 0, sgb, 0, sgb_len);
			/* assume non-fragmented memory	*/
			usb_scsi_trace_bytes(sgbPrefix, sgb_own.piov->iov_base, sgb_own.piov->iov_len);
			free_sg_buffer(&sgb_own);
		} else {
			TRACE_ALWAYS("usb_scsi_trace_sgb:error allocate sgb for %d bytes\n", sgb_len);
		}
	} else {
		TRACE_ALWAYS("usb_scsi_trace_sgb:error allocate memory for %d bytes\n", len);
	}
}

void usb_scsi_trace_SG(iovec *sg, int count)
{
	char sg_mask[] = "SG:{%s}\n";
	char truncTail[] = "<TRUNC>";
	size_t trunc_count = min(count, 0x20); /* length watchdog */
	size_t len = sizeof(sg_mask) + sizeof(truncTail) + trunc_count * 16;
	char *pbuf = (char *)malloc(len + 1);
	if(pbuf){
		int i = 0;
		char *p = pbuf;
		for(; i < trunc_count; i++){
			sprintf(p, (!i) ? "%d" : ", %d", sg[i].iov_len);
			p += strlen(p);
		}
		/* user should be informed about truncation too*/
		if(trunc_count < count)
			strcpy(p, truncTail);

		TRACE(sg_mask, pbuf);
		free(pbuf);
	} else {
		TRACE_ALWAYS("usb_scsi_trace_SG:error allocate "
																		"memory for tracing %d SG entries\n", trunc_count);
	}
}

#endif /* BUILD_LOG_DAEMON */

