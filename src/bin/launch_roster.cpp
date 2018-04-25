/*
 * Copyright 2015-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*!	The launch_daemon's companion command line tool. */


#include <LaunchRoster.h>
#include <StringList.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static struct option const kLongOptions[] = {
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

static struct option const kLogLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{"raw", no_argument, 0, 'r'},
	{"user", no_argument, 0, 'u'},
	{"system", no_argument, 0, 's'},
	{"event", required_argument, 0, 'e'},
	{"limit", required_argument, 0, 'l'},
	{NULL}
};

extern const char *__progname;
static const char *kProgramName = __progname;


static void
list_jobs(bool verbose)
{
	BLaunchRoster roster;
	BStringList jobs;
	status_t status = roster.GetJobs(NULL, jobs);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not get job listing: %s\n", kProgramName,
			strerror(status));
		exit(EXIT_FAILURE);
	}

	for (int32 i = 0; i < jobs.CountStrings(); i++)
		puts(jobs.StringAt(i).String());
}


static void
list_targets(bool verbose)
{
	BLaunchRoster roster;
	BStringList targets;
	status_t status = roster.GetTargets(targets);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not get target listing: %s\n", kProgramName,
			strerror(status));
		exit(EXIT_FAILURE);
	}

	for (int32 i = 0; i < targets.CountStrings(); i++)
		puts(targets.StringAt(i).String());
}


static void
print_log(const BMessage& log)
{
	time_t now = time(NULL);
	bigtime_t runtime = system_time();

	for (int32 index = 0;; index++) {
		BMessage item;
		if (log.FindMessage("item", index, &item) != B_OK)
			break;

		uint64 when;
		const char* message;
		if (item.FindUInt64("when", &when) != B_OK
			|| item.FindString("message", &message) != B_OK)
			break;

		time_t at = now - (runtime - when) / 1000000l;
		struct tm tm;
		localtime_r(&at, &tm);
		char label[256];
		strftime(label, sizeof(label), "%F %X", &tm);
		printf("%s %s\n", label, message);
	}
}


static void
log_usage(int status)
{
	fprintf(stderr, "Usage: %s log [-rusel] [<job-name>]\n"
		"Where the following options are allowed:\n"
		"  -u --user       List only user log entries\n"
		"  -s --system     List only system log entries\n"
		"  -e --event      Filter by event name (partial names accepted)\n"
		"  -l --limit <n>  Limit output to <n> events\n"
		"<job-name>, if given, filters the jobs by name.\n",
		kProgramName);

	exit(status);
}


static void
get_log(int argCount, char** args)
{
	bool raw = false;
	bool userOnly = false;
	bool systemOnly = false;
	int32 limit = 0;
	const char* event = NULL;
	const char* job = NULL;

	optind = 0;
	int c;
	while ((c = getopt_long(argCount, args, "hruse:l:", kLogLongOptions, NULL))
			!= -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				log_usage(0);
				break;
			case 'r':
				raw = true;
				break;
			case 'u':
				userOnly = true;
				break;
			case 's':
				systemOnly = true;
				break;
			case 'e':
				event = optarg;
				break;
			case 'l':
				limit = strtol(optarg, NULL, 0);
				break;
		}
	}

	if (argCount - optind >= 1)
		job = args[optind];

	BLaunchRoster roster;
	BMessage filter;
	if (userOnly)
		filter.AddBool("userOnly", true);
	if (systemOnly)
		filter.AddBool("systemOnly", true);
	if (event != NULL)
		filter.AddString("event", event);
	if (job != NULL)
		filter.AddString("job", job);
	if (limit != 0)
		filter.AddInt32("limit", limit);

	BMessage info;
	status_t status = roster.GetLog(filter, info);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not get log: %s\n", kProgramName,
			strerror(status));
		exit(EXIT_FAILURE);
	}

	if (raw) {
		info.PrintToStream();
		return;
	}

	print_log(info);

	BMessage user;
	if (info.FindMessage("user", &user) == B_OK) {
		if (user.HasMessage("item"))
			puts("User log:");
		print_log(user);
	}
}


static void
get_info(const char* name)
{
	BLaunchRoster roster;
	BMessage info;
	status_t targetStatus = roster.GetTargetInfo(name, info);
	if (targetStatus == B_OK) {
		printf("Target: %s\n", name);
		info.PrintToStream();
	}

	info.MakeEmpty();
	status_t jobStatus = roster.GetJobInfo(name, info);
	if (jobStatus == B_OK) {
		printf("Job: %s\n", name);
		info.PrintToStream();
	}

	if (jobStatus != B_OK && targetStatus != B_OK) {
		fprintf(stderr, "%s: Could not get target or job info for \"%s\": "
			"%s\n", kProgramName, name, strerror(jobStatus));
		exit(EXIT_FAILURE);
	}
}


static void
start_job(const char* name)
{
	BLaunchRoster roster;
	status_t status = roster.Start(name);
	if (status == B_NAME_NOT_FOUND)
		status = roster.Target(name);

	if (status != B_OK) {
		fprintf(stderr, "%s: Starting job \"%s\" failed: %s\n", kProgramName,
			name, strerror(status));
		exit(EXIT_FAILURE);
	}
}


static void
stop_job(const char* name)
{
	BLaunchRoster roster;
	status_t status = roster.Stop(name);
	if (status == B_NAME_NOT_FOUND)
		status = roster.StopTarget(name);

	if (status != B_OK) {
		fprintf(stderr, "%s: Stopping job \"%s\" failed: %s\n", kProgramName,
			name, strerror(status));
		exit(EXIT_FAILURE);
	}
}


static void
restart_job(const char* name)
{
	stop_job(name);
	start_job(name);
}


static void
enable_job(const char* name, bool enable)
{
	BLaunchRoster roster;
	status_t status = roster.SetEnabled(name, enable);
	if (status != B_OK) {
		fprintf(stderr, "%s: %s job \"%s\" failed: %s\n", kProgramName,
			enable ? "Enabling" : "Disabling", name, strerror(status));
		exit(EXIT_FAILURE);
	}
}


static void
usage(int status)
{
	fprintf(stderr, "Usage: %s <command>\n"
		"Where <command> is one of:\n"
		"  list - Lists all jobs (the default command)\n"
		"  list-targets - Lists all targets\n"
		"The following <command>s have a <name> argument:\n"
		"  start - Starts a job/target\n"
		"  stop - Stops a running job/target\n"
		"  restart - Restarts a running job/target\n"
		"  info - Shows info for a job/target\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char** argv)
{
	const char* command = "list";
	bool verbose = false;

	int c;
	while ((c = getopt_long(argc, argv, "+hv", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				usage(0);
				break;
			case 'v':
				verbose = true;
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind >= 1)
		command = argv[optind];

	if (strcmp(command, "list") == 0) {
		list_jobs(verbose);
	} else if (strcmp(command, "list-targets") == 0) {
		list_targets(verbose);
	} else if (strcmp(command, "log") == 0) {
		get_log(argc - optind, &argv[optind]);
	} else if (argc == optind + 1) {
		// For convenience (the "info" command can be omitted)
		get_info(command);
	} else {
		// All commands that need a name following

		const char* name = argv[argc - 1];

		if (strcmp(command, "info") == 0) {
			get_info(name);
		} else if (strcmp(command, "start") == 0) {
			start_job(name);
		} else if (strcmp(command, "stop") == 0) {
			stop_job(name);
		} else if (strcmp(command, "restart") == 0) {
			restart_job(name);
		} else if (strcmp(command, "enable") == 0) {
			enable_job(name, true);
		} else if (strcmp(command, "disable") == 0) {
			enable_job(name, false);
		} else {
			fprintf(stderr, "%s: Unknown command \"%s\".\n", kProgramName,
				command);
		}
	}
	return 0;
}
