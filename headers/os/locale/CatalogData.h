/*
 * Copyright 2003-2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CATALOG_DATA_H_
#define _CATALOG_DATA_H_


#include <SupportDefs.h>
#include <String.h>


class BCatalog;
class BMessage;
struct entry_ref;


/**
 * Base class for the catalog-data provided by every catalog add-on. An instance
 * of this class represents (the data of) a single catalog. Several of these
 * catalog data objects may be chained together in order to represent
 * variations of a specific language. If for instance the catalog data 'en_uk'
 * is chained to the data for 'en', a BCatalog using this catalog data chain
 * will prefer any entries in the 'en_uk' catalog, but fallback onto 'en' for
 * entries missing in the former.
 */
class BCatalogData {
public:
								BCatalogData(const char* signature,
									const char* language,
									uint32 fingerprint);
	virtual						~BCatalogData();

	virtual	const char*			GetString(const char* string,
									const char* context = NULL,
									const char* comment = NULL) = 0;
	virtual	const char*			GetString(uint32 id) = 0;

			status_t			InitCheck() const;
			BCatalogData*		Next();

	// the following could be used to localize non-textual data (e.g.
	// icons), but these will only be implemented if there's demand for such
	// a feature:
	virtual	bool				CanHaveData() const;
	virtual	status_t			GetData(const char* name, BMessage* msg);
	virtual	status_t			GetData(uint32 id, BMessage* msg);

	// interface for catalog-editor-app and testing apps:
	virtual	status_t			SetString(const char* string,
									const char* translated,
									const char* context = NULL,
									const char* comment = NULL);
	virtual	status_t			SetString(int32 id, const char* translated);

	virtual	bool				CanWriteData() const;
	virtual	status_t			SetData(const char* name, BMessage* msg);
	virtual	status_t			SetData(uint32 id, BMessage* msg);

	virtual	status_t			ReadFromFile(const char* path = NULL);
	virtual	status_t			ReadFromAttribute(
									const entry_ref& appOrAddOnRef);
	virtual	status_t			ReadFromResource(
									const entry_ref& appOrAddOnRef);
	virtual	status_t			WriteToFile(const char* path = NULL);
	virtual	status_t			WriteToAttribute(
									const entry_ref& appOrAddOnRef);
	virtual	status_t			WriteToResource(
									const entry_ref& appOrAddOnRef);

	virtual	void				MakeEmpty();
	virtual	int32				CountItems() const;

			void				SetNext(BCatalogData* next);

protected:
	virtual	void				UpdateFingerprint();

protected:
	friend	class BCatalog;
	friend	status_t 			get_add_on_catalog(BCatalog*, const char*);

			status_t 			fInitCheck;
			BString 			fSignature;
			BString 			fLanguageName;
			uint32				fFingerprint;
			BCatalogData*		fNext;
};


inline BCatalogData*
BCatalogData::Next()
{
	return fNext;
}


// every catalog-add-on should export the following three symbols:
//
// 1. the function that instantiates a catalog for this add-on-type
extern "C"
BCatalogData* instantiate_catalog(const char* signature, const char* language,
	uint32 fingerprint);

// 2. the function that creates an empty catalog for this add-on-type
extern "C"
BCatalogData* create_catalog(const char* signature, const char* language);

// 3. the priority which will be used to order the catalog add-ons
extern uint8 gCatalogAddOnPriority;


#endif /* _CATALOG_DATA_H_ */
