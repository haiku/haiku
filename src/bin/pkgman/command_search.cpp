/*
 * Copyright 2013-2023, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <set>

#include <Locale.h>

#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <TextTable.h>

#include "Command.h"
#include "PackageManager.h"
#include "pkgman.h"


// TODO: internationalization!
// The table code doesn't support full-width characters yet.


using namespace BPackageKit;


static const char* const kShortUsage =
	"  %command% ( <search-string> | --all | -a )\n"
	"    Searches for packages matching <search-string>.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% ( <search-string> | --all | -a )\n"
	"Searches for packages matching <search-string>.\n"
	"\n"
	"Options:\n"
	"  -a, --all\n"
	"    List all packages. Specified instead of <search-string>.\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"  -D, --details\n"
	"    Print more details. Matches in each installation location and each\n"
	"    repository will be listed individually with their version.\n"
	"  -i, --installed-only\n"
	"    Only find installed packages.\n"
	"  -u, --uninstalled-only\n"
	"    Only find not installed packages.\n"
	"  -n, --not-required\n"
	"    List only the packages that are not required by any other package.\n"
	"    Note: Implies --all, and omits listing \"_source\" and \"_debuginfo\" packages.\n"
	"  -r, --requirements\n"
	"    Search packages with <search-string> as requirements.\n"
	"  -s <scope>, --search-scope=<scope>\n"
	"    Search for packages containing <search-string> only on the given scope.\n"
	"    <scope> must be either \"name\" or \"full\"."
	"\n"
	"Status flags in non-detailed listings:\n"
	"  S - installed in system with a matching version in a repository\n"
	"  s - installed in system without a matching version in a repository\n"
	"  H - installed in home with a matching version in a repository\n"
	"  h - installed in home without a matching version in a repository\n"
	"  v - multiple different versions available in repositories\n"
	"\n";


DEFINE_COMMAND(SearchCommand, "search", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_PACKAGES)


static int
get_terminal_width()
{
    int fd = fileno(stdout);
    struct winsize windowSize;
	if (isatty(fd) == 1 && ioctl(fd, TIOCGWINSZ, &windowSize) == 0)
		return windowSize.ws_col;

    return INT_MAX;
}


struct PackageComparator {
	PackageComparator(const BSolverRepository* systemRepository,
		const BSolverRepository* homeRepository)
		:
		fSystemRepository(systemRepository),
		fHomeRepository(homeRepository)
	{
		BLocale::Default()->GetCollator(&fCollator);
		fCollator.SetNumericSorting(true);
	}

	int operator()(const BSolverPackage* a, const BSolverPackage* b) const
	{
		int cmp = fCollator.Compare(a->Name().String(), b->Name().String());
		if (cmp != 0)
			return cmp;

		// Names are equal. Sort by installation location and then by repository
		// name.
		if (a->Repository() == b->Repository())
			return 0;

		if (a->Repository() == fSystemRepository)
			return -1;
		if (b->Repository() == fSystemRepository)
			return 1;
		if (a->Repository() == fHomeRepository)
			return -1;
		if (b->Repository() == fHomeRepository)
			return 1;

		return a->Repository()->Name().Compare(b->Repository()->Name());
	}

private:
	const BSolverRepository*	fSystemRepository;
	const BSolverRepository*	fHomeRepository;

	BCollator					fCollator;
};


static int
compare_packages(const BSolverPackage* a, const BSolverPackage* b,
	void* comparator)
{
	return (*(PackageComparator*)comparator)(a, b);
}


static status_t
filter_required_packages(const BPackageManager& packageManager,
	BObjectList<BSolverPackage>& packages)
{
	std::set<BSolverPackage*> packagesSet;
	BSolverPackageSpecifierList requirements;
	for (int32 i = 0; i < packages.CountItems(); i++) {
		BSolverPackage* package = packages.ItemAt(i);
		if (package->Name().EndsWith("_source") || package->Name().EndsWith("_debuginfo"))
			continue;

		packagesSet.insert(package);

		BObjectList<BPackageResolvableExpression, true> requiresList
			= package->Info().RequiresList();
		for (int32 j = 0; j < requiresList.CountItems(); j++)
			requirements.AppendSpecifier(requiresList.ItemAt(j)->ToString());
	}
	packages.MakeEmpty();

	BObjectList<BSolverPackage> requiredPackages;
	const BSolverPackageSpecifier* unmatched;
	status_t status = packageManager.Solver()->FindPackages(requirements,
		0, requiredPackages, &unmatched);
	if (status != B_OK) {
		fprintf(stderr, "Failed to filter required packages: %s\n", strerror(status));
		if (unmatched != NULL) {
			// Try to find the package this requirement comes from.
			std::set<BSolverPackage*>::const_iterator setIterator = packagesSet.begin();
			for (; setIterator != packagesSet.end(); setIterator++) {
				BSolverPackage* package = *setIterator;
				BObjectList<BPackageResolvableExpression, true> requiresList
					= package->Info().RequiresList();
				for (int32 j = 0; j < requiresList.CountItems(); j++) {
					if (requiresList.ItemAt(j)->ToString() != unmatched->SelectString())
						continue;

					fprintf(stderr, "\t(unmatched: %s, required by %s)\n",
						unmatched->SelectString().String(), package->Name().String());
					return status;
				}
			}

			// We couldn't find it, just print the requirement itself.
			fprintf(stderr, "\t(unmatched: %s)\n", unmatched->SelectString().String());
		}
		return status;
	}

	for (int32 i = 0; i < requiredPackages.CountItems(); i++)
		packagesSet.erase(requiredPackages.ItemAt(i));

	std::set<BSolverPackage*>::const_iterator setIterator = packagesSet.begin();
	for (; setIterator != packagesSet.end(); setIterator++)
		packages.AddItem(*setIterator);

	return B_OK;
}


int
SearchCommand::Execute(int argc, const char* const* argv)
{
	bool installedOnly = false;
	bool uninstalledOnly = false;
	bool nameOnly = false;
	bool fullSearch = false;
	bool listAll = false;
	bool listNotRequiredOnly = false;
	bool details = false;
	bool requirements = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'a' },
			{ "not-required", no_argument, 0, 'n' },
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "details", no_argument, 0, 'D' },
			{ "help", no_argument, 0, 'h' },
			{ "installed-only", no_argument, 0, 'i' },
			{ "uninstalled-only", no_argument, 0, 'u' },
			{ "requirements", no_argument, 0, 'r' },
			{ "search-scope", required_argument, NULL, 's' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "aDhinurs:", sLongOptions, NULL);
		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'a':
				listAll = true;
				break;

			case 'D':
				details = true;
				break;

			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'i':
				installedOnly = true;
				uninstalledOnly = false;
				break;

			case 'u':
				uninstalledOnly = true;
				installedOnly = false;
				break;

			case 'r':
				requirements = true;
				break;

			case 'n':
				listNotRequiredOnly = true;
				break;

			case 's':
				if (strcmp(optarg, "name") == 0)
					nameOnly = true;
				else if (strcmp(optarg, "full") == 0)
					fullSearch = true;
				else
					fprintf(stderr, "Warning: Invalid search scope (%s). Using default.\n",
						optarg);
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining argument is the search string. Ignored when --all or --not-required has been
	// specified.
	if ((!(listAll || listNotRequiredOnly) && argc != optind + 1)
		|| ((listAll || listNotRequiredOnly) && requirements))
		PrintUsageAndExit(true);

	const char* searchString = (listAll || listNotRequiredOnly) ? "" : argv[optind++];

	// create the solver
	PackageManager packageManager(B_PACKAGE_INSTALLATION_LOCATION_HOME);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());
	packageManager.Init(
		(!uninstalledOnly ? PackageManager::B_ADD_INSTALLED_REPOSITORIES : 0)
			| (!installedOnly ? PackageManager::B_ADD_REMOTE_REPOSITORIES : 0));

	uint32 flags = BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
		| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_PROVIDES;

	if (nameOnly)
		flags = BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME;

	if (fullSearch)
		flags = BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
			| BSolver::B_FIND_IN_PROVIDES;

	if (requirements)
		flags = BSolver::B_FIND_IN_REQUIRES;

	// search
	BObjectList<BSolverPackage> packages;
	status_t error = packageManager.Solver()->FindPackages(searchString,
		flags, packages);
	if (error != B_OK)
		DIE(error, "searching packages failed");

	if (packages.IsEmpty()) {
		printf("No matching packages found.\n");
		return 0;
	}

	if (listNotRequiredOnly) {
		error = filter_required_packages(packageManager, packages);
		if (error != B_OK)
			return error;
	}

	// sort packages by name and installation location/repository
	const BSolverRepository* systemRepository
		= static_cast<const BSolverRepository*>(
			packageManager.SystemRepository());
	const BSolverRepository* homeRepository
		= static_cast<const BSolverRepository*>(
			packageManager.HomeRepository());
	PackageComparator comparator(systemRepository, homeRepository);
	packages.SortItems(&compare_packages, &comparator);

	// print table
	TextTable table;

	if (details) {
		table.AddColumn("Repository");
		table.AddColumn("Name");
		table.AddColumn("Version");
		table.AddColumn("Arch");

		int32 packageCount = packages.CountItems();
		for (int32 i = 0; i < packageCount; i++) {
			BSolverPackage* package = packages.ItemAt(i);

			BString repository = "";
			if (package->Repository() == systemRepository)
				repository = "<system>";
			else if (package->Repository() == homeRepository)
				repository = "<home>";
			else
				repository = package->Repository()->Name();

			table.SetTextAt(i, 0, repository);
			table.SetTextAt(i, 1, package->Name());
			table.SetTextAt(i, 2, package->Version().ToString());
			table.SetTextAt(i, 3, package->Info().ArchitectureName());
		}
	} else {
		table.AddColumn("Status");
		table.AddColumn("Name");
		table.AddColumn("Description", B_ALIGN_LEFT, true);

		int32 packageCount = packages.CountItems();
		for (int32 i = 0; i < packageCount;) {
			// find the next group of equally named packages
			int32 groupStart = i;
			std::set<BPackageVersion> versions;
			BSolverPackage* systemPackage = NULL;
			BSolverPackage* homePackage = NULL;
			while (i < packageCount) {
				BSolverPackage* package = packages.ItemAt(i);
				if (i > groupStart
					&& package->Name() != packages.ItemAt(groupStart)->Name()) {
					break;
				}

				if (package->Repository() == systemRepository)
					systemPackage = package;
				else if (package->Repository() == homeRepository)
					homePackage = package;
				else
					versions.insert(package->Version());

				i++;
			}

			// add a table row for the group
			BString status;
			if (systemPackage != NULL) {
				status << (versions.find(systemPackage->Version())
					!= versions.end() ? 'S' : 's');
			}
			if (homePackage != NULL) {
				status << (versions.find(homePackage->Version())
					!= versions.end() ? 'H' : 'h');
			}
			if (versions.size() > 1)
				status << 'v';

			int32 rowIndex = table.CountRows();
			BSolverPackage* package = packages.ItemAt(groupStart);
			table.SetTextAt(rowIndex, 0, status);
			table.SetTextAt(rowIndex, 1, package->Name());
			table.SetTextAt(rowIndex, 2, package->Info().Summary());
		}
	}

	table.Print(get_terminal_width());

	return 0;
}
