/*
** Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <String.h>

#include <DefaultCatalog.h>
#include <HashMapCatalog.h>
#include <PlainTextCatalog.h>

using std::vector;

void
usage()
{
	fprintf(stderr,
		"usage: linkcatkeys [-v] [-t(a|f|r)] [-o <outfile>] [-l <catalogLang>]\n"
		"                   -s <catalogSig> <catalogFiles>\n"
		"options:\n"
		"  -l <catalogLang>\tlanguage of the target-catalog (default is English)\n"
		"  -o <outfile>\t\texplicitly specifies the name of the output-file\n"
		"  -s <catalogSig>\tsignature of the target-catalog\n"
		"  -t(a|f|r)\t\tspecifies target of resulting catalog (-tf is default)\n"
		"           \t\ta => write catalog as an attribute (to output-file)\n"
		"           \t\tf => write catalog into the output-file\n"
		"           \t\tr => write catalog as a resource (to output-file)\n"
		"  -v\t\t\tbe verbose, show summary\n");
	exit(-1);
}


int
main(int argc, char **argv)
{
	bool showSummary = false;
	bool showWarnings = false;
	vector<const char *> inputFiles;
	BString outputFile("default.catalog");
	enum TargetType {
		TARGET_ATTRIBUTE,
		TARGET_FILE,
		TARGET_RESOURCE
	};
	TargetType outputTarget = TARGET_FILE;
	const char *catalogSig = NULL;
	BString catalogLang("English");
	status_t res;
	while ((++argv)[0]) {
		if (argv[0][0] == '-' && argv[0][1] != '-') {
			char *arg = argv[0] + 1;
			char c;
			while ((c = *arg++) != '\0') {
				if (c == 's')
					catalogSig = (++argv)[0];
				else if (c == 'l')
					catalogLang = (++argv)[0];
				else if (c == 'v')
					showSummary = true;
				else if (c == 'w')
					showWarnings = true;
				else if (c == 'o') {
					outputFile = (++argv)[0];
					break;
				}
				else if (c == 't') {
					switch(*arg) {
						case 'a': outputTarget = TARGET_ATTRIBUTE; break;
						case 'f': outputTarget = TARGET_FILE; break;
						case 'r': outputTarget = TARGET_RESOURCE; break;
						default: usage();
					}
				}
			}
		} else if (!strcmp(argv[0], "--help")) {
			usage();
		} else {
			inputFiles.push_back(argv[0]);
		}
	}
	if (inputFiles.empty() || !catalogSig || !outputFile.Length())
		usage();

	DefaultCatalog targetCatImpl(outputFile.String(), catalogSig,
		catalogLang.String());
	if ((res = targetCatImpl.InitCheck()) != B_OK) {
		fprintf(stderr, "couldn't construct target-catalog %s - error: %s\n",
			outputFile.String(), strerror(res));
		exit(-1);
	}

	uint32 count = inputFiles.size();
	for (uint32 i = 0; i < count; ++i) {
		PlainTextCatalog inputCatalog(inputFiles[i], catalogSig,
			catalogLang.String());
		if ((res = inputCatalog.ReadFromFile()) != B_OK) {
			fprintf(stderr, "couldn't load source-catalog %s - error: %s\n",
				inputFiles[i], strerror(res));
			exit(-1);
		}

		// now walk over all entries in input-catalog and add them to
		// target catalog, unless they already exist there.
		BHashMapCatalog::CatWalker walker(&inputCatalog);
		while (!walker.AtEnd()) {
			const CatKey &key(walker.GetKey());

			if (!targetCatImpl.GetString(key))
				targetCatImpl.SetRawString(key, walker.GetValue());
			walker.Next();
		}
	}

	switch(outputTarget) {
		case TARGET_ATTRIBUTE: {
			BEntry entry(outputFile.String());
			entry_ref eref;
			entry.GetRef(&eref);
			res = targetCatImpl.WriteToAttribute(eref);
			if (res != B_OK) {
				fprintf(stderr,
					"couldn't write target-attribute to %s - error: %s\n",
					outputFile.String(), strerror(res));
				exit(-1);
			}
			break;
		}
		case TARGET_RESOURCE: {
			BEntry entry(outputFile.String());
			entry_ref eref;
			entry.GetRef(&eref);
			res = targetCatImpl.WriteToResource(eref);
			if (res != B_OK) {
				fprintf(stderr,
					"couldn't write target-resource to %s - error: %s\n",
					outputFile.String(), strerror(res));
				exit(-1);
			}
		}
		default: {
			res = targetCatImpl.WriteToFile(outputFile.String());
			if (res != B_OK) {
				fprintf(stderr,
					"couldn't write target-catalog to %s - error: %s\n",
					outputFile.String(), strerror(res));
				exit(-1);
			}
		}
	}
	if (showSummary) {
		int32 count = targetCatImpl.CountItems();
		if (count) {
			fprintf(stderr, "%d key%s found and written to %s\n",
				count, (count==1 ? "": "s"), outputFile.String());
		} else
			fprintf(stderr, "no keys found\n");
	}

	return res;
}
