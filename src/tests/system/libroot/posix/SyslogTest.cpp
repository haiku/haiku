/*
 * Copyright 2003-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>


int
main(int argc, char **argv)
{
	openlog_team("SyslogTest", LOG_PID, LOG_USER);

	log_team(LOG_ERR, "this is %.", "a test");

	int mask = setlogmask_team(LOG_MASK(LOG_CRIT));
	printf("team mask == %d\n", mask);

	log_team(LOG_WARNING, "this is a warning (hidden)");
	log_team(LOG_CRIT, "this is a critical condition (visible)");

	setlogmask_team(mask);
	syslog(LOG_WARNING, "thread warning (visible)");
	syslog(LOG_CRIT, "thread critical condition (visible)");
	syslog(LOG_CRIT, "thread critical condition (visible)");

	setlogmask(LOG_MASK(LOG_WARNING));
	log_team(LOG_WARNING | LOG_MAIL, "2. this is a warning from the MAIL facility (visible)");
	log_team(LOG_CRIT, "2. this is a critical condition (visible)");
	log_team(LOG_CRIT, "2. this is a critical condition (visible)");
	log_team(LOG_CRIT, "2. this is a critical condition (visible)");
		// test repeat message suppressing as well

	openlog(NULL, LOG_PERROR, LOG_USER);
	syslog(LOG_WARNING, "thread/perror warning (visible in stderr as well)");
	syslog(LOG_CRIT, "thread/perror critical condition (hidden)");

	openlog(NULL, LOG_CONS | LOG_PID, LOG_DAEMON);
	syslog(LOG_WARNING, "thread/cons warning (visible in stderr only when there is no syslog_daemon)");

	openlog("", 0, LOG_DAEMON);
	syslog(LOG_WARNING, "thread warning without ident (visible)");

	setlogmask(LOG_EMERG);
	closelog();
		// this should inherit the team log context on next logging entry

	syslog(LOG_ALERT, "now what are we doing here? (visible)");
	return 0;
}
