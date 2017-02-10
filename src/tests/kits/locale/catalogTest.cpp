/*
** Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <Catalog.h>
#include <DefaultCatalog.h>
#include <Entry.h>
#include <Locale.h>
#include <Path.h>
#include <Roster.h>

class CatalogTest {
	public:
		void Run();
		void Check();
};

#define B_TRANSLATION_CONTEXT "CatalogTest"


#define catSig "x-vnd.Be.locale.catalogTest"
#define catName catSig".catalog"

void
CatalogTest::Run()
{
	printf("app...");
	status_t res;
	BString s;
	s << "string" << "\x01" << B_TRANSLATION_CONTEXT << "\x01";
	size_t hashVal = CatKey::HashFun(s.String());
	assert(be_locale != NULL);
	system("mkdir -p ./locale/catalogs/"catSig);

	// create an empty catalog of default type...
	BPrivate::EditableCatalog cata("Default", catSig, "German");
	assert(cata.InitCheck() == B_OK);

	// ...and populate the catalog with some data:
	res = cata.SetString("string", "Schnur", B_TRANSLATION_CONTEXT);
	assert(res == B_OK);
	res = cata.SetString(hashVal, "Schnur_id");
		// add a second entry for the same hash-value, but with different
		// translation
	assert(res == B_OK);
	res = cata.SetString("string", "String", "programming");
	assert(res == B_OK);
	res = cata.SetString("string", "Textpuffer", "programming",
		"Deutsches Fachbuch");
	assert(res == B_OK);
	res = cata.SetString("string", "Leine", B_TRANSLATION_CONTEXT,
		"Deutsches Fachbuch");
	assert(res == B_OK);
	res = cata.WriteToFile("./locale/catalogs/"catSig"/german.catalog");
	assert(res == B_OK);

	// check if we are getting back the correct strings:
	s = cata.GetString(("string"), B_TRANSLATION_CONTEXT);
	assert(s == "Schnur");
	s = cata.GetString(hashVal);
	assert(s == "Schnur_id");
	s = cata.GetString("string", "programming");
	assert(s == "String");
	s = cata.GetString("string", "programming", "Deutsches Fachbuch");
	assert(s == "Textpuffer");
	s = cata.GetString("string", B_TRANSLATION_CONTEXT, "Deutsches Fachbuch");
	assert(s == "Leine");

	// now we create a new (base) catalog and embed this one into the app-file:
	BPrivate::EditableCatalog catb("Default", catSig, "English");
	assert(catb.InitCheck() == B_OK);
	// the following string is unique to the embedded catalog:
	res = catb.SetString("string", "string", "base");
	assert(res == B_OK);
	// the following id is unique to the embedded catalog:
	res = catb.SetString(32, "hashed string");
	assert(res == B_OK);
	// the following string will be hidden by the definition inside the
	// german catalog:
	res = catb.SetString("string", "hidden", B_TRANSLATION_CONTEXT);
	assert(res == B_OK);
	app_info appInfo;
	res = be_app->GetAppInfo(&appInfo);
	assert(res == B_OK);
	// embed created catalog into application file (catalogTest):
	res = catb.WriteToResource(&appInfo.ref);
	assert(res == B_OK);

	printf("ok.\n");
	Check();
}


void
CatalogTest::Check()
{
	status_t res;
	printf("app-check...");
	BString s;
	s << "string" << "\x01" << B_TRANSLATION_CONTEXT << "\x01";
	size_t hashVal = CatKey::HashFun(s.String());
	// ok, we now try to re-load the catalog that has just been written:
	//
	// actually, the following code can be seen as an example of what an
	// app needs in order to translate strings:
	BCatalog cat;
	res = be_locale->GetAppCatalog(&cat);
	assert(res == B_OK);
	// fetch basic data:
	uint32 fingerprint = 0;
	res = cat.GetFingerprint(&fingerprint);
	assert(res == B_OK);
	BString lang;
	res = cat.GetLanguage(&lang);
	assert(res == B_OK);
	BString sig;
	res = cat.GetSignature(&sig);
	assert(res == B_OK);

	// now check strings:
	s = B_TRANSLATE_ID(hashVal);
	assert(s == "Schnur_id");
	s = B_TRANSLATE_ALL("string", "programming", "");
	assert(s == "String");
	s = B_TRANSLATE_ALL("string", "programming", "Deutsches Fachbuch");
	assert(s == "Textpuffer");
	s = B_TRANSLATE_COMMENT("string", "Deutsches Fachbuch");
	assert(s == "Leine");
	// the following string should be found in the embedded catalog only:
	s = B_TRANSLATE_ALL("string", "base", NULL);
	assert(s == "string");
	// the following id should be found in the embedded catalog only:
	s = B_TRANSLATE_ID(32);
	assert(s == "hashed string");
	// the following id doesn't exist anywhere (hopefully):
	s = B_TRANSLATE_ID(-1);
	assert(s == "");
	// the following string exists twice, in the embedded as well as in the
	// external catalog. So we should get the external translation (as it should
	// override the embedded one):
	s = B_TRANSLATE("string");
	assert(s == "Schnur");

	// now check if trying to access same catalog by specifying its data works:
	BCatalog cat2(sig.String(), lang.String(), fingerprint);
	assert(cat2.InitCheck() == B_OK);
	// now check if trying to access same catalog with wrong fingerprint fails:
	BCatalog cat3(sig.String(), lang.String(), fingerprint*-1);
	assert(cat3.InitCheck() == B_NO_INIT);
	// translating through an invalid catalog should yield the native string:
	s = cat3.GetString("string");
	assert(s == "string");

	printf("ok.\n");
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
	appEntry.GetParent( &appFolder);
	BPath appPath;
	appFolder.GetPath( &appPath);
	chdir( appPath.Path());

	CatalogTest catTest;
	catTest.Run();

	char cwd[B_FILE_NAME_LENGTH];
	getcwd(cwd, B_FILE_NAME_LENGTH);
	BString addonName(cwd);
	addonName << "/" "catalogTestAddOn";
	image_id image = load_add_on(addonName.String());
	assert(image >= B_OK);
	void (*runAddonFunc)() = 0;
	get_image_symbol(image, "run_test_add_on",
		B_SYMBOL_TYPE_TEXT, (void **)&runAddonFunc);
	assert(runAddonFunc);
	runAddonFunc();

	catTest.Check();

	delete testApp;
}
