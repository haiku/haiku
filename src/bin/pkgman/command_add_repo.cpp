/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Errors.h>
#include <SupportDefs.h>

#include <package/AddRepositoryRequest.h>
#include <package/Context.h>
#include <package/JobQueue.h>

#include "pkgman.h"


// TODO: internationalization!


using namespace Haiku::Package;


static const char* kCommandUsage =
	"Usage: %s add-repo <repo-URL> [<repo-URL> ...]\n"
	"Adds one or more repositories by downloading them from the given URL(s).\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


struct Listener : public JobStateListener {
	virtual	void JobStarted(Job* job)
	{
		printf("%s ...\n", job->Title().String());
	}
	virtual	void JobSucceeded(Job* job)
	{
	}
	virtual	void JobFailed(Job* job)
	{
		DIE(job->Result(), "failed!");
	}
	virtual	void JobAborted(Job* job)
	{
		DIE(job->Result(), "aborted");
	}
};


int
command_add_repo(int argc, const char* const* argv)
{
	bool asUserRepository = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "user", no_argument, 0, 'u' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hu", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_command_usage_and_exit(false);
				break;

			case 'u':
				asUserRepository = true;
				break;

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are repo URLs, i. e. at least one more argument.
	if (argc < optind + 1)
		print_command_usage_and_exit(true);

	const char* const* repoURLs = argv + optind;
	int urlCount = argc - optind;

	Context context;
	status_t result;
	Listener listener;
	context.SetDefaultJobStateListener(&listener);
	for (int i = 0; i < urlCount; ++i) {
		AddRepositoryRequest request(context, repoURLs[i], asUserRepository);
		JobQueue jobQueue;
		result = request.CreateJobsToRun(jobQueue);
		if (result != B_OK)
			DIE(result, "unable to create necessary jobs");

		while (Job* job = jobQueue.Pop()) {
			result = job->Run();
			delete job;
			if (result == B_INTERRUPTED)
				break;
		}
	}

	return 0;
}
