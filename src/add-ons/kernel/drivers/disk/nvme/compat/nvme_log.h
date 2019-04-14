/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_LOG_H__
#define __NVME_LOG_H__

#include <stdint.h>
#include <stdarg.h>


void nvme_log(enum nvme_log_level level, const char *format, ...)
	__attribute__((format(printf, 2, 3)));
void nvme_vlog(enum nvme_log_level level, const char *format, va_list ap)
	__attribute__((format(printf,2,0)));


#define nvme_emerg(format, args...)		\
	nvme_log(NVME_LOG_EMERG,		\
		"libnvme (FATAL): " format,	\
		## args)

#define nvme_alert(format, args...)		\
	nvme_log(NVME_LOG_ALERT,		\
		"libnvme (ALERT): " format,	\
		## args)

#define nvme_crit(format, args...)		\
	nvme_log(NVME_LOG_CRIT,			\
		"libnvme (CRITICAL): " format,	\
		## args)

#define nvme_err(format, args...)		\
	nvme_log(NVME_LOG_ERR,			\
		"libnvme (ERROR): " format,	\
		## args)

#define nvme_warning(format, args...)		\
	nvme_log(NVME_LOG_WARNING,		\
		"libnvme (WARNING): " format,	\
		## args)

#ifdef TRACE_NVME
#define nvme_notice(format, args...)	\
	nvme_log(NVME_LOG_NOTICE,	\
		"libnvme: " format,	\
		## args)

#define nvme_info(format, args...)	\
	nvme_log(NVME_LOG_INFO,		\
		"libnvme: " format,	\
		## args)

#define nvme_debug(format, args...)	\
	nvme_log(NVME_LOG_DEBUG,	\
		"libnvme: " format,	\
		## args)
#else
#define nvme_notice(format, args...)
#define nvme_info(format, args...)
#define nvme_debug(format, args...)
#endif


#endif /* __NVME_LOG_H__ */
