/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "syslog_output.h"

#include <FindDirectory.h>
#include <Path.h>

#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


static const size_t kMaxLogSize = 524288;	// 512kB

static const char *kFacilities[] = {
	"KERN", "USER", "MAIL", "DAEMON",
	"AUTH", "SYSLOGD", "LPR", "NEWS",
	"UUCP", "CRON", "AUTHPRIV",
	"", "", "", "", "",
	"LOCAL0", "LOCAL1", "LOCAL2", "LOCAL3",
	"LOCAL4", "LOCAL5", "LOCAL6", "LOCAL7",
	NULL
};
static const int32 kNumFacilities = 24;

int sLog = -1;
char sLastMessage[1024];
thread_id sLastThread;
int32 sRepeatCount;


/*!
	Creates the log file if not yet existing, or renames the old
	log file, if it's too big already.
*/
static status_t
prepare_output()
{
	bool needNew = true;
	bool tooLarge = false;

	if (sLog >= 0) {
		// check file size
		struct stat stat;
		if (fstat(sLog, &stat) == 0) {
			if (stat.st_size < kMaxLogSize)
				needNew = false;
			else
				tooLarge = true;
		}
	}

	if (needNew) {
		// close old file; it'll be (re)moved soon
		if (sLog >= 0)
			close(sLog);

		// get path (and create it if necessary)
		BPath base;
		find_directory(B_COMMON_LOG_DIRECTORY, &base, true);

		BPath syslog(base);
		syslog.Append("syslog");

		// move old file if it already exists
		if (tooLarge) {
			BPath oldlog(base);
			oldlog.Append("syslog.old");

			remove(oldlog.Path());
			rename(syslog.Path(), oldlog.Path());

			// ToDo: just remove old file if space on device is tight?
		}

		bool haveSyslog = sLog >= 0;

		// open file
		sLog = open(syslog.Path(), O_APPEND | O_CREAT | O_WRONLY, 644);
		if (!haveSyslog && sLog >=0) {
			// first time open, check file size again 
			prepare_output();
		}
	}

	return sLog >= 0 ? B_OK : B_ERROR;
}


static status_t
write_to_log(const char *buffer, int32 length)
{
	if (sRepeatCount > 0) {
		char repeat[64];
		ssize_t size = snprintf(repeat, sizeof(repeat),
			"Last message repeated %ld time%s\n", sRepeatCount,
			sRepeatCount > 1 ? "s" : "");
		sRepeatCount = 0;
		if (write(sLog, repeat, strlen(repeat)) < size)
			return B_ERROR;
	}

	if (write(sLog, buffer, length) < length)
		return B_ERROR;

	return B_OK;
}


static void
syslog_output(syslog_message &message)
{
	char header[128];
	int32 headerLength;

#if 0
	// parse & nicely print the time stamp from the message
	struct tm when;
	localtime_r(&message.when, &when);
	int32 pos = strftime(buffer, sizeof(buffer), "%b %d, %H:%M:%S ", &when);
#else
	int32 pos = 0;
#endif

	// add facility
	int facility = SYSLOG_FACILITY_INDEX(message.priority);
	if (facility >= kNumFacilities)
		facility = SYSLOG_FACILITY_INDEX(LOG_USER);
	pos += snprintf(header + pos, sizeof(header) - pos, "%s", kFacilities[facility]);

	// add ident/thread ID
	if (message.ident[0] == '\0') {
		// ToDo: find out team name?
	} else
		pos += snprintf(header + pos, sizeof(header) - pos, " '%s'", message.ident);

	if (message.options & LOG_PID)
		pos += snprintf(header + pos, sizeof(header) - pos, "[%ld]", message.from);

	headerLength = pos + strlcpy(header + pos, ": ", sizeof(header) - pos);
	if (headerLength >= (int32)sizeof(header))
		headerLength = sizeof(header) - 1;

	// add header to every line of the message and write it to the syslog

	char buffer[SYSLOG_MESSAGE_BUFFER_SIZE];
	pos = 0;

	while (true) {
		strcpy(buffer, header);
		int32 length;

		const char *newLine = strchr(message.message + pos, '\n');
		if (newLine != NULL) {
			length = newLine - message.message + 1 - pos;
			strlcpy(buffer + headerLength, message.message + pos, length + 1);
			pos += length;
		} else {
			length = strlcpy(buffer + headerLength, message.message + pos,
				sizeof(buffer) - headerLength);
		}

		length += headerLength;

		if (length >= (int32)sizeof(buffer))
			length = sizeof(buffer) - 1;

		if (message.from == sLastThread
			&& !strncmp(buffer + headerLength, sLastMessage, sizeof(sLastMessage))) {
			// we got this message already
			sRepeatCount++;
		} else {
			// dump message line

			if (prepare_output() < B_OK
				|| write_to_log(buffer, length) < B_OK) {
				// cannot write to syslog!
				fputs(buffer, stderr);
			}

			// save this message to suppress repeated messages
			strlcpy(sLastMessage, buffer + headerLength, sizeof(sLastMessage));
			sLastThread = message.from;
		}

		if (newLine == NULL || !newLine[1]) {
			// wrote last line of output
			break;
		}
	}
}


void
init_syslog_output(SyslogDaemon *daemon)
{
	daemon->AddHandler(syslog_output);
}

