/*
** Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <cstdio>
#include <cstdlib>

#include <File.h>
#include <String.h>

#include <DefaultCatalog.h>
#include <EditableCatalog.h>


using BPrivate::CatKey;
using BPrivate::DefaultCatalog;
using BPrivate::EditableCatalog;


void
usage()
{
	fprintf(stderr, "usage: dumpcatalog <catalogFiles>\n");
	exit(-1);
}


int
main(int argc, char **argv)
{
	const char *inputFile = NULL;
	status_t res;
	if (!argv[1] || !strcmp(argv[1], "--help")) {
		usage();
	} else {
		inputFile = argv[1];
	}
	if (!inputFile || !strlen(inputFile))
		usage();

	EditableCatalog inputCatalog("Default", "dummy", "dummy");
	if ((res = inputCatalog.InitCheck()) != B_OK) {
		fprintf(stderr, "couldn't construct catalog %s - error: %s\n",
			inputFile, strerror(res));
		exit(-1);
	}
	if ((res = inputCatalog.ReadFromFile(inputFile)) != B_OK) {
		fprintf(stderr, "couldn't load input-catalog %s - error: %s\n",
			inputFile, strerror(res));
		exit(-1);
	}
	DefaultCatalog* inputCatImpl
		= dynamic_cast<DefaultCatalog*>(inputCatalog.CatalogData());
	if (!inputCatImpl) {
		fprintf(stderr, "couldn't access impl of input-catalog %s\n",
			inputFile);
		exit(-1);
	}
	// now walk over all entries in input-catalog and dump them to
	// stdout
	DefaultCatalog::CatWalker walker(inputCatImpl);
	BString str, ctx, cmt;
	while (!walker.AtEnd()) {
		const CatKey &key(walker.GetKey());
		key.GetStringParts(&str, &ctx, &cmt);
		printf("Hash:\t\t%" B_PRIu32 "\nKey:\t\t<%s:%s:%s>\nTranslation:\t%s\n"
			"-----\n", key.fHashVal, str.String(), ctx.String(), cmt.String(),
			walker.GetValue());
		walker.Next();
	}
	int32 count = inputCatalog.CountItems();
	if (count) {
		fprintf(stderr, "%" B_PRId32 " entr%s dumped\n", count,
			(count==1 ? "y": "ies"));
	} else
		fprintf(stderr, "no entries found\n");
	return res;
}
