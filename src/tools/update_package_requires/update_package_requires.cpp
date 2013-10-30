/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <map>

#include <package/PackageInfo.h>
#include <package/PackageResolvable.h>
#include <package/PackageResolvableExpression.h>
#include <package/RepositoryCache.h>


using namespace BPackageKit;


typedef std::list<BPackageResolvable*> ProvidesList;


static const char* sProgramName = "update_package_requires";


#define DIE(result, msg...)											\
do {																\
	fprintf(stderr, "*** " msg);									\
	fprintf(stderr, " : %s\n", strerror(result));					\
	exit(5);														\
} while(0)


static void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout,
		"Usage: %s <package info> <repository>\n"
		"Updates the versions in the \"requires\" list of the given package\n"
		"info file using the available package information from the given\n"
		"repository cache file <repository>.\n",
		sProgramName);
	exit(error ? 1 : 0);
}


static void
update_requires_expression(BPackageResolvableExpression& expression,
	const ProvidesList& providesList)
{
	// find the best-matching provides
	BPackageResolvable* bestProvides = NULL;
	for (ProvidesList::const_iterator it = providesList.begin();
		it != providesList.end(); ++it) {
		BPackageResolvable* provides = *it;
		if (!expression.Matches(*provides))
			continue;

		if (bestProvides == NULL || bestProvides->Version().InitCheck() != B_OK
			|| (provides->Version().InitCheck() == B_OK
				&& provides->Version().Compare(bestProvides->Version()) > 0)) {
			bestProvides = provides;
		}
	}

	if (bestProvides == NULL || bestProvides->Version().InitCheck() != B_OK)
		return;

	// Update the expression. Enforce the minimum found version, if the requires
	// has no version requirement or also a minimum. Otherwise enforce the exact
	// version found.
	BPackageResolvableOperator newOperator = B_PACKAGE_RESOLVABLE_OP_EQUAL;
	switch (expression.Operator()) {
		case B_PACKAGE_RESOLVABLE_OP_LESS:
		case B_PACKAGE_RESOLVABLE_OP_LESS_EQUAL:
		case B_PACKAGE_RESOLVABLE_OP_EQUAL:
		case B_PACKAGE_RESOLVABLE_OP_NOT_EQUAL:
			break;
		case B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL:
		case B_PACKAGE_RESOLVABLE_OP_GREATER:
		case B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT:
			newOperator = B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL;
			break;
	}

	expression.SetTo(expression.Name(), newOperator, bestProvides->Version());
}


int
main(int argc, const char* const* argv)
{
	if (argc != 3)
		print_usage_and_exit(true);

	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		print_usage_and_exit(false);

	const char* const packageInfoPath = argv[1];
	const char* const repositoryCachePath = argv[2];

	// read the repository cache
	BRepositoryCache repositoryCache;
	status_t error = repositoryCache.SetTo(repositoryCachePath);
	if (error != B_OK) {
		DIE(error, "failed to read repository cache file \"%s\"",
			repositoryCachePath);
	}

	// create a map for all provides (name -> resolvable list)
	typedef std::map<BString, ProvidesList> ProvidesMap;

	ProvidesMap providesMap;

	for (BRepositoryCache::Iterator it = repositoryCache.GetIterator();
		const BPackageInfo* info = it.Next();) {
		const BObjectList<BPackageResolvable>& provides = info->ProvidesList();
		int32 count = provides.CountItems();
		for (int32 i = 0; i < count; i++) {
			BPackageResolvable* resolvable = provides.ItemAt(i);
			ProvidesList& providesList = providesMap[resolvable->Name()];
			providesList.push_back(resolvable);
		}
	}

	// load the package info
	BPackageInfo packageInfo;
	error = packageInfo.ReadFromConfigFile(packageInfoPath);
	if (error != B_OK)
		DIE(error, "failed to read package info file \"%s\"", packageInfoPath);

	// clone the package info's requires list
	typedef std::list<BPackageResolvableExpression> RequiresList;
	RequiresList requiresList;
	int32 requiresCount = packageInfo.RequiresList().CountItems();
	for (int32 i = 0; i < requiresCount; i++)
		requiresList.push_back(*packageInfo.RequiresList().ItemAt(i));

	// rebuild the requires list with updated versions
	packageInfo.ClearRequiresList();
	for (RequiresList::iterator it = requiresList.begin();
		it != requiresList.end(); ++it) {
		BPackageResolvableExpression expression = *it;
		ProvidesMap::iterator foundIt = providesMap.find(expression.Name());
		if (foundIt != providesMap.end())
			update_requires_expression(expression, foundIt->second);

		error = packageInfo.AddRequires(expression);
		if (error != B_OK)
			DIE(error, "failed to add requires item to package info");
	}

	// write updated package info
	BString configString;
	error = packageInfo.GetConfigString(configString);
	if (error != B_OK)
		DIE(error, "failed to get updated package info string");

	FILE* file = fopen(packageInfoPath, "w");
	if (file == NULL) {
		DIE(errno, "failed to open package info file \"%s\" for writing",
			packageInfoPath);
	}

	if (fwrite(configString.String(), configString.Length(), 1, file) != 1) {
		DIE(errno, "failed to write updated package info file \"%s\"",
			packageInfoPath);
	}

	fclose(file);

	return 0;
}
