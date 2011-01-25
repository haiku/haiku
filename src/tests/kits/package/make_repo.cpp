#include <stdio.h>
#include <stdlib.h>

#include <File.h>
#include <Message.h>
#include <String.h>

#include <package/RepositoryHeader.h>


using namespace BPackageKit;


int
main(int argc, const char** argv)
{
	if (argc < 5) {
		fprintf(stderr, "usage: %s <name> <url> <priority> <pkg-count>\n",
			argv[0]);
		return 1;
	}

	BRepositoryHeader repoHeader;
	repoHeader.SetName(argv[1]);
	repoHeader.SetOriginalBaseURL(argv[2]);
	repoHeader.SetPriority(atoi(argv[3]));

	BMessage repoHeaderArchive;
	status_t result = repoHeader.Archive(&repoHeaderArchive);
	if (result != B_OK) {
		fprintf(stderr, "couldn't archive repository-header\n");
		return 1;
	}

	BFile output(argv[1], B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((result = repoHeaderArchive.Flatten(&output)) != B_OK) {
		fprintf(stderr, "couldn't flatten repository-header archive\n");
		return 1;
	}

	int pkgCount = atoi(argv[4]);
	for (int i = 0; i < pkgCount; ++i) {
		BMessage pkg('PKGp');

		BString name = BString("pkg") << i + 1;
		pkg.AddString("name", name.String());

		BString majorVersion
			= BString() << 1 + i % 5 << "." << i % 10;
		BString minorVersion
			= BString() << i % 100;
		pkg.AddString("version",
			(BString() << majorVersion << "." << minorVersion).String());

		pkg.AddString("provides",
			(BString() << name << "-" << majorVersion).String());
		if (i % 2 == 1)
			pkg.AddString("provides", (BString("lib") << name).String());
		if (i % 3 != 1)
			pkg.AddString("provides", (BString("cmd:") << name).String());

		if (i > 1) {
			int requiresCount = rand() % 10 % i;
			for (int r = 0; r < requiresCount; ++r) {
				int reqIndex = rand() % i;
				pkg.AddString("requires",
					(BString("pkg") << reqIndex).String());
			}
		}

		result = pkg.Flatten(&output);
		if (result != B_OK) {
			fprintf(stderr, "couldn't flatten pkg message #%d\n", i + 1);
			return 1;
		}
	}

	return 0;
}
