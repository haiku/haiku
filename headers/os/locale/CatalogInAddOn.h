#ifndef _CATALOG_IN_ADD_ON_H_
#define _CATALOG_IN_ADD_ON_H_

#include <Catalog.h>
#include <Entry.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Node.h>
#include <TypeConstants.h>

/*
 * This header file is somewhat special...
 *
 * Since the TR-macros are referencing be_catalog, we want each add-on 
 * to have its own be_catalog (instead of sharing the one from the lib).
 * In order to do so, we define another be_catalog in this file, which
 * will override the one from liblocale.so. 
 * This way we implement something like add-on-local storage. It would
 * be desirable to find a better way, so please tell us if you know one!
 *
 * Every add-on that wants to use the Locale Kit needs to include this
 * file once (no more, exactly once!).
 * You have to include it in the source-file that loads the add-on's
 * catalog (by use of get_add_on_catalog().
 *
 */

BCatalog *be_catalog = NULL;
	// global that points to this add-on's catalog


/*
 * utility function which determines the entry-ref for "this" add-on:
 */
static status_t
get_add_on_ref(entry_ref *ref) 
{
	if (!ref)
		return B_BAD_VALUE;

	image_info imageInfo;
	int32 cookie = 0;
	status_t res = B_ERROR;
	void *dataPtr = static_cast<void*>(&be_catalog);
	while ((res = get_next_image_info(0, &cookie, &imageInfo)) == B_OK) {
		char *dataStart = static_cast<char*>(imageInfo.data);
		if (imageInfo.type == B_ADD_ON_IMAGE && dataStart <= dataPtr 
			&& dataStart+imageInfo.data_size > dataPtr) {
			get_ref_for_path(imageInfo.name, ref);
			break;
		}
	}
	return res;
}


/*
 * Every add-on should load its catalog through this function, since
 * it does not only load the add-on's catalog, but it does several
 * other useful things:
 *    - be_catalog (the one belonging to this add-on) is initialized
 *      such that the TR-macros use the add-on's catalog.
 *    - if givenSig is empty (NULL or "") the add-on's signature is
 *      used to determine the catalog-sig.
 *    - if the add-on's executable contains an embedded catalog, that
 *      one is loaded, too.
 */
status_t 
get_add_on_catalog(BCatalog* cat, const char *givenSig) 
{
	if (!cat)
		return B_BAD_VALUE;

	// determine entry-ref of the add-on this code lives in:
	entry_ref ref;
	status_t res = get_add_on_ref(&ref);
	if (res == B_OK) {
		// ok, we have found the entry-ref for our add-on,
		// so we try to fetch the fingerprint from our add-on-file:
		int32 fp = 0;
		BNode addOnNode(&ref);
		addOnNode.ReadAttr(BLocaleRoster::kCatFingerprintAttr, 
			B_INT32_TYPE, 0, &fp, sizeof(int32));
		BString sig(givenSig);
		if (sig.Length() == 0) {
			res = addOnNode.ReadAttrString("BEOS:MIME", &sig);
			if (res == B_OK) {
				// drop supertype from mimetype (should be "application/"):
				int32 pos = sig.FindFirst('/');
				if (pos >= 0)
					sig.Remove(0, pos+1);
			}
		}
		if (res == B_OK) {
			// try to load it's catalog...
			cat->fCatalog 
				= be_locale_roster->LoadCatalog(sig.String(), NULL, fp);
			// ...and try to load an embedded catalog, too (if that exists):
			BCatalogAddOn *embeddedCatalog 
				= be_locale_roster->LoadEmbeddedCatalog(&ref);
			if (embeddedCatalog) {
				if (!cat->fCatalog)
					// embedded catalog is the only catalog that was found:
					cat->fCatalog = embeddedCatalog;
				else {
					// append embedded catalog to list of loaded catalogs:
					BCatalogAddOn *currCat = cat->fCatalog;
					while (currCat->fNext)
						currCat = currCat->fNext;
					currCat->fNext = embeddedCatalog;
				}
			}
			be_catalog = cat;
		}
	}
	return cat->InitCheck();
}


#endif	/* _CATALOG_IN_ADD_ON_H_ */
