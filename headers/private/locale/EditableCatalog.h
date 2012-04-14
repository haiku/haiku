/*
 * Copyright 2003-2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _EDITABLE_CATALOG_H_
#define _EDITABLE_CATALOG_H_


#include <Catalog.h>


class BMessage;
struct entry_ref;


namespace BPrivate {


class EditableCatalog : public BCatalog {
public:
								EditableCatalog(const char* type,
									const char* signature,
									const char* language);
	virtual						~EditableCatalog();

			status_t			SetString(const char* string,
									const char* translated,
									const char* context = NULL,
									const char* comment = NULL);
			status_t			SetString(int32 id, const char* translated);

			bool				CanWriteData() const;
			status_t			SetData(const char* name, BMessage* msg);
			status_t			SetData(uint32 id, BMessage* msg);

			status_t			ReadFromFile(const char* path = NULL);
			status_t			ReadFromAttribute(
									const entry_ref& appOrAddOnRef);
			status_t			ReadFromResource(
									const entry_ref& appOrAddOnRef);
			status_t			WriteToFile(const char* path = NULL);
			status_t			WriteToAttribute(
									const entry_ref& appOrAddOnRef);
			status_t			WriteToResource(
									const entry_ref& appOrAddOnRef);

			void				MakeEmpty();

			BCatalogAddOn*		CatalogAddOn();

private:
								EditableCatalog();
								EditableCatalog(const EditableCatalog& other);
			const EditableCatalog&	operator=(const EditableCatalog& other);
									// hide assignment, default-, and
									// copy-constructor
};


} // namespace BPrivate


#endif /* _EDITABLE_CATALOG_H_ */
