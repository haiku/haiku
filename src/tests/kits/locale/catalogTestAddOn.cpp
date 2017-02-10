/*
** Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <CatalogInAddOn.h>
#include <DefaultCatalog.h>

class CatalogTestAddOn {
	public:
		void Run();
		void Check();
};

#define B_TRANSLATION_CONTEXT "CatalogTestAddOn"

#define catSig "add-ons/catalogTest/catalogTestAddOn"
#define catName catSig".catalog"


void
CatalogTestAddOn::Run() {
	printf("addon...");
	status_t res;
	BString s;
	s << "string" << "\x01" << B_TRANSLATION_CONTEXT << "\x01";
	size_t hashVal = CatKey::HashFun(s.String());
	assert(be_locale != NULL);
	system("mkdir -p ./locale/catalogs/"catSig);

	// create an empty catalog of default type...
	BPrivate::EditableCatalog cat1("Default", catSig, "German");
	assert(cat1.InitCheck() == B_OK);

	// ...and populate the catalog with some data:
	res = cat1.SetString("string", "Schnur_A", B_TRANSLATION_CONTEXT);
	assert(res == B_OK);
	res = cat1.SetString(hashVal, "Schnur_id_A");
		// add a second entry for the same hash-value, but with different
		// translation
	assert(res == B_OK);
	res = cat1.SetString("string", "String_A", "programming");
	assert(res == B_OK);
	res = cat1.SetString("string", "Textpuffer_A", "programming",
		"Deutsches Fachbuch");
	assert(res == B_OK);
	res = cat1.SetString("string", "Leine_A", B_TRANSLATION_CONTEXT,
		"Deutsches Fachbuch");
	assert(res == B_OK);
	res = cat1.WriteToFile("./locale/catalogs/"catSig"/german.catalog");
	assert(res == B_OK);

	// check if we are getting back the correct strings:
	s = cat1.GetString("string", B_TRANSLATION_CONTEXT);
	assert(s == "Schnur_A");
	s = cat1.GetString(hashVal);
	assert(s == "Schnur_id_A");
	s = cat1.GetString("string", "programming");
	assert(s == "String_A");
	s = cat1.GetString("string", "programming", "Deutsches Fachbuch");
	assert(s == "Textpuffer_A");
	s = cat1.GetString("string", B_TRANSLATION_CONTEXT, "Deutsches Fachbuch");
	assert(s == "Leine_A");

	// now we create a new (base) catalog and embed this one into the
	// add-on-file:
	BPrivate::EditableCatalog cat2("Default", catSig, "English");
	assert(cat2.InitCheck() == B_OK);
	// the following string is unique to the embedded catalog:
	res = cat2.SetString("string", "string_A", "base");
	assert(res == B_OK);
	// the following id is unique to the embedded catalog:
	res = cat2.SetString(32, "hashed string_A");
	assert(res == B_OK);
	// the following string will be hidden by the definition inside the german
	// catalog:
	res = cat2.SetString("string", "hidden_A", B_TRANSLATION_CONTEXT);
	assert(res == B_OK);
	entry_ref addOnRef;
	res = get_add_on_ref(&addOnRef);
	assert(res == B_OK);
	res = cat2.WriteToResource(&addOnRef);
	assert(res == B_OK);

	printf("ok.\n");
	Check();
}


void
CatalogTestAddOn::Check() {
	status_t res;
	printf("addon-check...");
	BString s;
	s << "string" << "\x01" << B_TRANSLATION_CONTEXT << "\x01";
	size_t hashVal = CatKey::HashFun(s.String());
	// ok, we now try to re-load the catalog that has just been written:
	//
	// actually, the following code can be seen as an example of what an
	// add_on needs in order to translate strings:
	BCatalog cat;
	res = get_add_on_catalog(&cat, catSig);
	assert(res == B_OK);
	// fetch basic data:
	uint32 fingerprint;
	res = cat.GetFingerprint(&fingerprint);
	assert(res == B_OK);
	BString lang;
	res = cat.GetLanguage(&lang);
	assert(res == B_OK);
	assert(lang == "german");
	BString sig;
	res = cat.GetSignature(&sig);
	assert(res == B_OK);
	assert(sig == catSig);

	// now check strings:
	s = B_TRANSLATE_ID(hashVal);
	assert(s == "Schnur_id_A");
	s = B_TRANSLATE_ALL("string", "programming", "");
	assert(s == "String_A");
	s = B_TRANSLATE_ALL("string", "programming", "Deutsches Fachbuch");
	assert(s == "Textpuffer_A");
	s = B_TRANSLATE_COMMENT("string", "Deutsches Fachbuch");
	assert(s == "Leine_A");
	// the following string should be found in the embedded catalog only:
	s = B_TRANSLATE_ALL("string", "base", "");
	assert(s == "string_A");
	// the following id should be found in the embedded catalog only:
	s = B_TRANSLATE_ID(32);
	assert(s == "hashed string_A");
	// the following id doesn't exist anywhere (hopefully):
	s = B_TRANSLATE_ID(-1);
	assert(s == "");
	// the following string exists twice, in the embedded as well as in the
	// external catalog. So we should get the external translation (as it should
	// override the embedded one):
	s = B_TRANSLATE("string");
	assert(s == "Schnur_A");

	// check access to app-catalog from inside add-on:
	BCatalog appCat;
	res = be_locale->GetAppCatalog(&appCat);
	assert(res == B_OK);
	s = be_app_catalog->GetString("string", "CatalogTest");
	assert(s == "Schnur");
	s = be_app_catalog->GetString("string", "CatalogTest",
		"Deutsches Fachbuch");
	assert(s == "Leine");
	s = be_app_catalog->GetString("string", "programming",
		"Deutsches Fachbuch");
	assert(s == "Textpuffer");

	printf("ok.\n");
}


extern "C" _EXPORT void run_test_add_on()
{
	CatalogTestAddOn catTest;
	catTest.Run();
}
