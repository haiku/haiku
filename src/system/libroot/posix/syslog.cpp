/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <syslog_daemon.h>
#include <TLS.h>

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


struct syslog_context {
	char	ident[B_OS_NAME_LENGTH];
	int16	mask;
	int16	facility;
	int32	options;
};

static syslog_context sTeamContext = {
	"",
	-1,
	LOG_USER,
	LOG_CONS
};
static int32 sThreadContextSlot = -1;


static syslog_context *
allocate_context()
{
	syslog_context *context = (syslog_context *)malloc(sizeof(syslog_context));
	if (context == NULL)
		return NULL;

	// inherit the attributes of the team context
	memcpy(context, &sTeamContext, sizeof(syslog_context));
	return context;
}


/*! This function returns the current syslog context structure.
	If there is none for the current thread, it will create one
	that inherits the context attributes from the team and put it
	into TLS.
	If it could not allocate a thread context, it will return the
	team context; this function is guaranteed to return a valid
	syslog context.
*/
static syslog_context *
get_context()
{
	if (sThreadContextSlot == B_NO_MEMORY)
		return &sTeamContext;

	if (sThreadContextSlot < 0) {
		static int32 lock = 0;
		if (atomic_add(&lock, 1) == 0) {
			int32 slot = tls_allocate();

			if (slot < 0) {
				sThreadContextSlot = B_NO_MEMORY;
				return &sTeamContext;
			}

			sThreadContextSlot = slot;
		} else {
			while (sThreadContextSlot == -1)
				snooze(10000);
		}
	}

	syslog_context *context = (syslog_context *)tls_get(sThreadContextSlot);
	if (context == NULL) {
		// try to allocate the context again; it might have
		// been deleted, or there was not enough memory last
		// time
		*tls_address(sThreadContextSlot) = context = allocate_context();
	}
	if (context != NULL)
		return context;

	return &sTeamContext;
}


//! Prints simplified syslog message to stderr.
static void
message_to_console(syslog_context *context, const char *text, va_list args)
{
	if (context->ident[0])
		fprintf(stderr, "'%s' ", context->ident);
	if (context->options & LOG_PID)
		fprintf(stderr, "[%" B_PRId32 "] ", find_thread(NULL));

	vfprintf(stderr, text, args);
	fputc('\n', stderr);
}


/*!	Creates the message from the given context and sends it to the syslog
	daemon, if the priority mask matches.
	If the message couldn't be delivered, and LOG_CONS was set, it will
	redirect the message to stderr.
*/
static void
send_syslog_message(syslog_context *context, int priority, const char *text,
	va_list args)
{
	int options = context->options;

	// do we have to do anything?
	if ((context->mask & LOG_MASK(SYSLOG_PRIORITY(priority))) == 0)
		return;

	port_id port = find_port(SYSLOG_PORT_NAME);
	if ((options & LOG_PERROR) != 0
		|| ((options & LOG_CONS) != 0 && port < B_OK)) {
		// if asked for, print out the (simplified) message on stderr
		message_to_console(context, text, args);
	}
	if (port < B_OK) {
		// apparently, there is no syslog daemon running;
		return;
	}

	// adopt facility from openlog() if not yet set
	if (SYSLOG_FACILITY(priority) == 0)
		priority |= context->facility;

	char buffer[2048];
	syslog_message &message = *(syslog_message *)&buffer[0];

	message.from = find_thread(NULL);
	message.when = real_time_clock();
	message.options = options;
	message.priority = priority;
	strcpy(message.ident, context->ident);

	int length = vsnprintf(message.message, sizeof(buffer)
		- sizeof(syslog_message), text, args);
	if (message.message + length - buffer < (int32)sizeof(buffer)) {
		if (length == 0 || message.message[length - 1] != '\n')
			message.message[length++] = '\n';
	} else
		buffer[length - 1] = '\n';

	status_t status;
	do {
		// make sure the message gets send (if there is a valid port)
		status = write_port(port, SYSLOG_MESSAGE, &message,
			sizeof(syslog_message) + length);
	} while (status == B_INTERRUPTED);

	if (status < B_OK && (options & LOG_CONS) != 0
		&& (options & LOG_PERROR) == 0) {
		// LOG_CONS redirects all output to the console in case contacting
		// the syslog daemon failed
		message_to_console(context, text, args);
	}
}


//	#pragma mark - POSIX API


void
closelog(void)
{
	closelog_thread();
}


void
openlog(const char *ident, int options, int facility)
{
	openlog_thread(ident, options, facility);
}


int
setlogmask(int priorityMask)
{
	return setlogmask_thread(priorityMask);
}


void
syslog(int priority, const char *message, ...)
{
	va_list args;

	va_start(args, message);
	send_syslog_message(get_context(), priority, message, args);
	va_end(args);
}


//	#pragma mark - Be extensions
// ToDo: it would probably be better to export these symbols as weak symbols only


void
closelog_team(void)
{
	// nothing to do here...
}


void
openlog_team(const char *ident, int options, int facility)
{
	if (ident != NULL)
		strlcpy(sTeamContext.ident, ident, sizeof(sTeamContext.ident));

	sTeamContext.options = options;
	sTeamContext.facility = SYSLOG_FACILITY(facility);
}


int
setlogmask_team(int priorityMask)
{
	int oldMask = sTeamContext.mask;

	if (priorityMask != 0)
		sTeamContext.mask = priorityMask;

	return oldMask;
}


void
log_team(int priority, const char *message, ...)
{
	va_list args;

	va_start(args, message);
	send_syslog_message(&sTeamContext, priority, message, args);
	va_end(args);
}


void
closelog_thread(void)
{
	if (sThreadContextSlot < 0)
		return;

	free(tls_get(sThreadContextSlot));
	*tls_address(sThreadContextSlot) = NULL;
}


void
openlog_thread(const char *ident, int options, int facility)
{
	syslog_context *context = get_context();

	if (ident)
		strcpy(context->ident, ident);

	context->options = options;
	context->facility = SYSLOG_FACILITY(facility);
}


int
setlogmask_thread(int priorityMask)
{
	syslog_context *context = get_context();
	int oldMask = context->mask;

	if (priorityMask != 0)
		context->mask = priorityMask;

	return oldMask;
}


void
log_thread(int priority, const char *message, ...)
{
	va_list args;

	va_start(args, message);
	send_syslog_message(get_context(), priority, message, args);
	va_end(args);
}


//	#pragma mark - BSD extensions


void
vsyslog(int priority, const char *message, va_list args)
{
	send_syslog_message(get_context(), priority, message, args);
}

