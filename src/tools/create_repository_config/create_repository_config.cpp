/*
 * Copyright 2013-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <package/RepositoryConfig.h>
#include <package/RepositoryInfo.h>


#define DIE(error, ...)								\
	do {											\
		fprintf(stderr, "Error: " __VA_ARGS__);		\
		fprintf(stderr, ": %s\n", strerror(error));	\
		exit(1);									\
	} while (false)

#define DIE_ON_ERROR(error, ...)		\
	do {								\
		status_t _error = error;		\
		if (error != B_OK)				\
			DIE(_error, __VA_ARGS__);	\
	} while (false)


static const char* sProgramName = "create_repository_config";


void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout,
		"Usage: %s <URL> <repository info> <repository config>\n"
		"Creates a repository config file from a given repository info and\n"
		"the base URL (the directory in which the 'repo', 'repo.info',\n"
		"and 'repo.sha256' files can be found).\n",
		sProgramName);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc != 4) {
		if (argc == 2
			&& (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
				print_usage_and_exit(false);
		}
		print_usage_and_exit(true);
	}

	int argi = 1;
	const char* url = argv[argi++];
	const char* infoPath = argv[argi++];
	const char* configPath = argv[argi++];

	// read the info
	BPackageKit::BRepositoryInfo repoInfo;
	DIE_ON_ERROR(repoInfo.SetTo(infoPath),
		"failed to read repository info file \"%s\"", infoPath);

	// init and write the config
	BPackageKit::BRepositoryConfig repoConfig;
	repoConfig.SetName(repoInfo.Name());
	repoConfig.SetBaseURL(url);
	repoConfig.SetPriority(repoInfo.Priority());
	DIE_ON_ERROR(repoConfig.Store(configPath),
		"failed to write repository config file \"%s\"", configPath);

	return 0;
}
