/*
** Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <unistd.h>

#include <Application.h>
#include <StopWatch.h>

#include <Catalog.h>
#include <HashMapCatalog.h>
#include <Entry.h>
#include <Locale.h>
#include <Path.h>
#include <Roster.h>

const uint32 kNumStrings = 10000;

BString strs[kNumStrings];
BString ctxs[kNumStrings];

BString trls[kNumStrings];

const char *translated;

class CatalogSpeed {
	public:
		void TestCreation();
		void TestLookup();
		void TestIdCreation();
		void TestIdLookup();
};

#define B_TRANSLATION_CONTEXT "CatalogSpeed"

#define catSig "x-vnd.Be.locale.catalogSpeed"
#define catName catSig".catalog"


void
CatalogSpeed::TestCreation()
{
	for (uint32 i = 0; i < kNumStrings; i++) {
		strs[i] << "native-string#" << 1000000+i;
		ctxs[i] << B_TRANSLATION_CONTEXT;
		trls[i] << "translation#" << 4000000+i;
	}

	BStopWatch watch("catalogSpeed", true);

	status_t res;
	assert(be_locale != NULL);
	system("mkdir -p ./locale/catalogs/"catSig);

	// create an empty catalog of default type...
	BPrivate::EditableCatalog cat1("Default", catSig, "klingon");
	assert(cat1.InitCheck() == B_OK);

	// ...and populate the catalog with some data:
	for (uint32 i = 0; i < kNumStrings; i++) {
		cat1.SetString(strs[i].String(), trls[i].String(), ctxs[i].String());
	}
	watch.Suspend();
	printf("\tadded %ld strings in           %9Ld usecs\n",
		cat1.CountItems(), watch.ElapsedTime());

	watch.Reset();
	watch.Resume();
	res = cat1.WriteToFile("./locale/catalogs/"catSig"/klingon.catalog");
	assert(res == B_OK);
	watch.Suspend();
	printf("\t%ld strings written to disk in %9Ld usecs\n",
		cat1.CountItems(), watch.ElapsedTime());
}


void
CatalogSpeed::TestLookup()
{
	BStopWatch watch("catalogSpeed", true);

	BCatalog *cat = be_catalog = new BCatalog(catSig, "klingon");

	assert(cat != NULL);
	assert(cat->InitCheck() == B_OK);
	watch.Suspend();
	printf("\t%ld strings read from disk in  %9Ld usecs\n",
		cat->CountItems(), watch.ElapsedTime());

	watch.Reset();
	watch.Resume();
	for (uint32 i = 0; i < kNumStrings; i++) {
		translated = B_TRANSLATE(strs[i].String());
	}
	watch.Suspend();
	printf("\tlooked up %lu strings in       %9Ld usecs\n",
		kNumStrings, watch.ElapsedTime());

	delete cat;
}


void
CatalogSpeed::TestIdCreation()
{
	BStopWatch watch("catalogSpeed", true);
	watch.Suspend();

	status_t res;
	BString s("string");
	s << "\x01" << typeid(*this).name() << "\x01";
	//size_t hashVal = __stl_hash_string(s.String());
	assert(be_locale != NULL);
	system("mkdir -p ./locale/catalogs/"catSig);

	// create an empty catalog of default type...
	BPrivate::EditableCatalog cat1("Default", catSig, "klingon");
	assert(cat1.InitCheck() == B_OK);

	// ...and populate the catalog with some data:
	for (uint32 i = 0; i < kNumStrings; i++) {
		trls[i] = BString("id_translation#") << 6000000+i;
	}
	watch.Reset();
	watch.Resume();
	for (uint32 i = 0; i < kNumStrings; i++) {
		cat1.SetString(i, trls[i].String());
	}
	watch.Suspend();
	printf("\tadded %ld strings by id in     %9Ld usecs\n",
		cat1.CountItems(), watch.ElapsedTime());

	watch.Reset();
	watch.Resume();
	res = cat1.WriteToFile("./locale/catalogs/"catSig"/klingon.catalog");
	assert( res == B_OK);
	watch.Suspend();
	printf("\t%ld strings written to disk in %9Ld usecs\n",
		cat1.CountItems(), watch.ElapsedTime());
}


void
CatalogSpeed::TestIdLookup()
{
	BStopWatch watch("catalogSpeed", true);

	BCatalog *cat = be_catalog = new BCatalog(catSig, "klingon");

	assert(cat != NULL);
	assert(cat->InitCheck() == B_OK);
	watch.Suspend();
	printf("\t%ld strings read from disk in  %9Ld usecs\n",
		cat->CountItems(), watch.ElapsedTime());

	watch.Reset();
	watch.Resume();
	for (uint32 i = 0; i < kNumStrings; i++) {
		translated = B_TRANSLATE_ID(i);
	}
	watch.Suspend();
	printf("\tlooked up %lu strings in       %9Ld usecs\n",
		kNumStrings, watch.ElapsedTime());

	delete cat;
}


int
main(int argc, char **argv)
{
	BApplication* testApp
		= new BApplication("application/"catSig);

	// change to app-folder:
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BEntry appEntry(&appInfo.ref);
	BEntry appFolder;
	appEntry.GetParent(&appFolder);
	BPath appPath;
	appFolder.GetPath(&appPath);
	chdir(appPath.Path());

	CatalogSpeed catSpeed;
	printf("\t------------------------------------------------\n");
	printf("\tstring-based catalog usage:\n");
	printf("\t------------------------------------------------\n");
	catSpeed.TestCreation();
	catSpeed.TestLookup();
	printf("\t------------------------------------------------\n");
	printf("\tid-based catalog usage:\n");
	printf("\t------------------------------------------------\n");
	catSpeed.TestIdCreation();
	catSpeed.TestIdLookup();

	delete testApp;

	return 0;
}
