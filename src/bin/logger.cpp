/*
 * Copyright 2006-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>


#define DEFAULT_PRIORITY LOG_DEBUG
#define DEFAULT_FACILITY LOG_USER


extern const char *__progname;
static const char *sProgramName = __progname;


struct mapping {
	const char*	name;
	int			value;
};

static mapping sPriorities[] = {
	{ "emerg",		LOG_EMERG },
	{ "panic",		LOG_PANIC },
	{ "alert",		LOG_ALERT },
	{ "critical",	LOG_CRIT },
	{ "crit",		LOG_CRIT },
	{ "error",		LOG_ERR },
	{ "err",		LOG_ERR },
	{ "warning", 	LOG_WARNING },
	{ "warn",		LOG_WARNING },
	{ "notice",		LOG_NOTICE },
	{ "info",		LOG_INFO },
	{ "debug",		LOG_DEBUG },
	{ NULL,			DEFAULT_PRIORITY },
};

static mapping sFacilities[] = {
	{ "kernel",		LOG_KERN },
	{ "kern",		LOG_KERN },
	{ "user",		LOG_USER },
	{ "mail",		LOG_MAIL },
	{ "daemon",		LOG_DAEMON },
	{ "auth",		LOG_AUTH },
	{ "security",	LOG_AUTH },
	{ "syslog",		LOG_SYSLOG },
	{ "lpr",		LOG_LPR },
	{ "news",		LOG_NEWS },
	{ "uucp",		LOG_UUCP },
	{ "cron",		LOG_CRON },
	{ "authpriv",	LOG_AUTHPRIV },
	{ NULL,			DEFAULT_FACILITY },
};


static int
lookup_mapping(struct mapping* mappings, const char *string)
{
	int32 i;
	for (i = 0; mappings[i].name != NULL; i++) {
		if (!strcasecmp(string, mappings[i].name))
			break;
	}

	return mappings[i].value;
}


static int
get_facility(const char *option)
{
	char facility[256];
	strlcpy(facility, option, sizeof(facility));

	char *end = strchr(facility, '.');
	if (end == NULL) {
		// there is no facility specified
		return DEFAULT_FACILITY;
	}

	end[0] = '\0';

	return lookup_mapping(sFacilities, facility);
}


static int
get_priority(const char *option)
{
	char *priority = strchr(option, '.');
	if (priority == NULL) {
		// there is no facility specified
		return DEFAULT_PRIORITY;
	}

	return lookup_mapping(sPriorities, ++priority);
}


static void
usage(void)
{
	fprintf(stderr, "usage: %s [-i] [-t <tag>] [-p <[facility.]priority>] "
			"<message>\n\n"
		"Sends a message to the system logging facility.\n"
		"If <message> is omitted, the message is read from stdin.\n\n"
		"   -i\tAdds the team ID to the log.\n"
		"   -t\tSpecifies the tag under which this message is posted.\n"
		"   -p\tSets the facility and priority this is logged under.\n"
		"    \t<facility> can be one of:\n"
		"\t\tkern, user, mail, daemon, auth, syslog, lpr, news,\n"
		"\t\tuucp, cron, authpriv\n"
		"    \t<priority> can be one of:\n"
		"\t\tdebug, info, notice, warning, warn, error, err,\n"
		"\t\tcritical, crit,alert, panic, emerg.\n", sProgramName);

	exit(-1);
}


int
main(int argc, char **argv)
{
	struct passwd* passwd = getpwuid(geteuid());
	const char* tag = NULL;
	int facility = DEFAULT_FACILITY;
	int priority = DEFAULT_PRIORITY;
	int options = 0;

	// read arguments

	int option;
	while ((option = getopt(argc, argv, "it:p:")) != -1) {
		switch (option) {
		    case 'i':	// log team ID
				options |= LOG_PID;
				break;

		    case 't':	// tag
				tag = optarg;
				break;

		    case 'p':	// facility/priority
				facility = get_facility(optarg);
				priority = get_priority(optarg);
				break;

		    default:
				usage();
				break;
		}
	}

	if (tag == NULL && passwd != NULL)
		tag = passwd->pw_name;

	argc -= optind;
	argv = &argv[optind];

	openlog(tag, options, facility);

	if (argc > 0) {
		// take message from arguments

		char* buffer = NULL;
		int32 length = 0;

		for (int32 i = 0; i < argc; i++) {
			int32 newLength = length + strlen(argv[i]) + 1;

			buffer = (char *)realloc(buffer, newLength + 1);
			if (buffer == NULL) {
				fprintf(stderr, "%s: out of memory\n", sProgramName);
				return -1;
			}

			strcpy(buffer + length, argv[i]);
			length = newLength;

			buffer[length - 1] = ' ';
		}

		if (length > 1 && buffer[length - 2] != '\n') {
			buffer[length - 1] = '\n';
			buffer[length] = '\0';
		} else
			buffer[length - 1] = '\0';

		syslog(priority, "%s", buffer);
		free(buffer);
	} else {
		// read messages from stdin

		char buffer[65536];
		while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
			syslog(priority, "%s", buffer);
		}
	}

	closelog();
	return 0;
}

