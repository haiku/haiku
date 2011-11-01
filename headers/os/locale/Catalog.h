/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CATALOG_H_
#define _CATALOG_H_


#include <LocaleRoster.h>
#include <SupportDefs.h>
#include <String.h>


class BCatalogAddOn;
class BLocale;
class BMessage;
struct entry_ref;


class BCatalog {
public:
								BCatalog();
								BCatalog(const entry_ref& catalogOwner,
									const char* language = NULL,
									uint32 fingerprint = 0);
	virtual						~BCatalog();

			const char*			GetString(const char* string,
									const char* context = NULL,
									const char* comment = NULL);
			const char*			GetString(uint32 id);

			const char*			GetNoAutoCollectString(const char* string,
									const char* context = NULL,
									const char* comment = NULL);
			const char*			GetNoAutoCollectString(uint32 id);

			status_t			GetData(const char* name, BMessage* msg);
			status_t			GetData(uint32 id, BMessage* msg);

			status_t			GetSignature(BString* signature);
			status_t			GetLanguage(BString* language);
			status_t			GetFingerprint(uint32* fingerprint);

			status_t			SetCatalog(const entry_ref& catalogOwner,
									uint32 fingerprint);

			status_t			InitCheck() const;
			int32				CountItems() const;

			BCatalogAddOn*		CatalogAddOn();

protected:
								BCatalog(const BCatalog&);
			const BCatalog&		operator= (const BCatalog&);
									// hide assignment and copy-constructor

			BCatalogAddOn*		fCatalog;

private:
			friend class BLocale;
			friend status_t get_add_on_catalog(BCatalog*, const char*);
};


#undef B_TRANSLATE_SYSTEM_NAME_CONTEXT
#define B_TRANSLATE_SYSTEM_NAME_CONTEXT "System name"


#ifndef B_COLLECTING_CATKEYS

#ifndef B_AVOID_TRANSLATION_MACROS
// macros for easy catalog-access, define B_AVOID_TRANSLATION_MACROS if
// you don't want these (in which case you need to collect the catalog keys
// manually, as collectcatkeys won't do it for you):

// TODO: maybe rename this to B_TRANSLATE_DEFAULT_CONTEXT, so that
// B_TRANSLATE_WITH_CONTEXT() can just be called B_TRANSLATE_CONTEXT()
#undef B_TRANSLATE_CONTEXT
	// In a single application, several strings (e.g. 'Ok') will be used
	// more than once, in different contexts.
	// As the application programmer can not know if all translations of
	// this string will be the same for all languages, each occurrence of
	// the string must be translated on its own.
	// Specifying the context explicitly with each string allows the person
	// translating a catalog to separate these different occurrences of the
	// same string and tell which strings appears in what context of the
	// application.
	// In order to give the translator a useful hint, the application
	// programmer needs to define B_TRANSLATE_CONTEXT with the context he'd
	// like to be associated with the strings used in this specifc source file.
	// example:
	//		#define B_TRANSLATE_CONTEXT "Folder-Window"
	// Tip: Use a descriptive name of the class implemented in that
	//		source-file.

// Translation macros which may be used to shorten translation requests:
#undef B_TRANSLATE
#define B_TRANSLATE(string) \
	BLocaleRoster::Default()->GetCatalog()->GetString((string), \
		B_TRANSLATE_CONTEXT)

#undef B_TRANSLATE_WITH_CONTEXT
#define B_TRANSLATE_WITH_CONTEXT(string, context) \
	BLocaleRoster::Default()->GetCatalog()->GetString((string), (context))

#undef B_TRANSLATE_COMMENT
#define B_TRANSLATE_COMMENT(string, comment) \
	BLocaleRoster::Default()->GetCatalog()->GetString((string), \
		B_TRANSLATE_CONTEXT, (comment))

#undef B_TRANSLATE_ALL
#define B_TRANSLATE_ALL(string, context, comment) \
	BLocaleRoster::Default()->GetCatalog()->GetString((string), (context), \
		(comment))

#undef B_TRANSLATE_ID
#define B_TRANSLATE_ID(id) \
	BLocaleRoster::Default()->GetCatalog()->GetString((id))

#undef B_TRANSLATE_SYSTEM_NAME
#define B_TRANSLATE_SYSTEM_NAME(string) \
	BLocaleRoster::Default()->IsFilesystemTranslationPreferred() \
	? BLocaleRoster::Default()->GetCatalog()->GetString((string), \
		B_TRANSLATE_SYSTEM_NAME_CONTEXT) : (string)

// Translation markers which can be used to mark static strings/IDs which
// are used as key for translation requests (at other places in the code):
/* example:
		#define B_TRANSLATE_CONTEXT "MyDecentApp-Menu"

		static const char *choices[] = {
			B_TRANSLATE_MARK("left"),
			B_TRANSLATE_MARK("right"),
			B_TRANSLATE_MARK("up"),
			B_TRANSLATE_MARK("down")
		};

		void MyClass::AddChoices(BMenu *menu) {
			for (char **ch = choices; *ch; ch++) {
				menu->AddItem(
					new BMenuItem(
						B_TRANSLATE(*ch),
						new BMessage(...)
					)
				)
			}
		}
*/
#undef B_TRANSLATE_MARK
#define B_TRANSLATE_MARK(str) \
	BCatalogAddOn::MarkForTranslation((str), B_TRANSLATE_CONTEXT, "")

#undef B_TRANSLATE_MARK_COMMENT
#define B_TRANSLATE_MARK_COMMENT(str, cmt) \
	BCatalogAddOn::MarkForTranslation((str), B_TRANSLATE_CONTEXT, (cmt))

#undef B_TRANSLATE_MARK_ALL
#define B_TRANSLATE_MARK_ALL(str, ctx, cmt) \
	BCatalogAddOn::MarkForTranslation((str), (ctx), (cmt))

#undef B_TRANSLATE_MARK_ID
#define B_TRANSLATE_MARK_ID(id) \
	BCatalogAddOn::MarkForTranslation((id))

#undef B_TRANSLATE_MARK_SYSTEM_NAME
#define B_TRANSLATE_MARK_SYSTEM_NAME(str) \
	BCatalogAddOn::MarkForTranslation((str), B_TRANSLATE_SYSTEM_NAME_CONTEXT, "")

// Translation macros which do not let collectcatkeys try to collect the key
// (useful in combination with the marking macros above):
#undef B_TRANSLATE_NOCOLLECT
#define B_TRANSLATE_NOCOLLECT(str) \
	B_TRANSLATE(str)

#undef B_TRANSLATE_NOCOLLECT_COMMENT
#define B_TRANSLATE_NOCOLLECT_COMMENT(str, cmt) \
	B_TRANSLATE_COMMENT(str, cmt)

#undef B_TRANSLATE_NOCOLLECT_ALL
#define B_TRANSLATE_NOCOLLECT_ALL(str, ctx, cmt) \
	B_TRANSLATE_ALL(str, ctx, cmt)

#undef B_TRANSLATE_NOCOLLECT_ID
#define B_TRANSLATE_NOCOLLECT_ID(id) \
	B_TRANSLATE_ID(id)

#undef B_TRANSLATE_NOCOLLECT_SYSTEM_NAME
#define B_TRANSLATE_NOCOLLECT_SYSTEM_NAME(str) \
	B_TRANSLATE_SYSTEM_NAME(str)

#endif	/* B_AVOID_TRANSLATION_MACROS */

#else	/* B_COLLECTING_CATKEYS */
// TODO: why define them here? Since we obviously control the preprocessor, we
// could simply always include a certain file that defines them; this doesn't
// really belong into a public header.

// Translation macros used when executing collectcatkeys

#undef B_TRANSLATE_CONTEXT

#undef B_TRANSLATE
#define B_TRANSLATE(string) \
	B_CATKEY((string), B_TRANSLATE_CONTEXT)

#undef B_TRANSLATE_WITH_CONTEXT
#define B_TRANSLATE_WITH_CONTEXT(string, context) \
	B_CATKEY((string), (context))

#undef B_TRANSLATE_COMMENT
#define B_TRANSLATE_COMMENT(string, comment) \
	B_CATKEY((string), B_TRANSLATE_CONTEXT, (comment))

#undef B_TRANSLATE_ALL
#define B_TRANSLATE_ALL(string, context, comment) \
	B_CATKEY((string), (context), (comment))

#undef B_TRANSLATE_ID
#define B_TRANSLATE_ID(id) \
	B_CATKEY((id))

#undef B_TRANSLATE_SYSTEM_NAME
#define B_TRANSLATE_SYSTEM_NAME(string) \
	B_CATKEY((string), B_TRANSLATE_SYSTEM_NAME_CONTEXT)

#undef B_TRANSLATE_MARK
#define B_TRANSLATE_MARK(str) \
	B_CATKEY((str), B_TRANSLATE_CONTEXT)

#undef B_TRANSLATE_MARK_COMMENT
#define B_TRANSLATE_MARK_COMMENT(str, cmt) \
	B_CATKEY((str), B_TRANSLATE_CONTEXT, (cmt))

#undef B_TRANSLATE_MARK_ALL
#define B_TRANSLATE_MARK_ALL(str, ctx, cmt) \
	B_CATKEY((str), (ctx), (cmt))

#undef B_TRANSLATE_MARK_ID
#define B_TRANSLATE_MARK_ID(id) \
	B_CATKEY((id))

#undef B_TRANSLATE_MARK_SYSTEM_NAME
#define B_TRANSLATE_MARK_SYSTEM_NAME(str) \
	B_CATKEY((str), B_TRANSLATE_SYSTEM_NAME_CONTEXT, "")

#undef B_TRANSLATE_NOCOLLECT
#define B_TRANSLATE_NOCOLLECT(str) \
	(void)

#undef B_TRANSLATE_NOCOLLECT_COMMENT
#define B_TRANSLATE_NOCOLLECT_COMMENT(str, cmt) \
	(void)

#undef B_TRANSLATE_NOCOLLECT_ALL
#define B_TRANSLATE_NOCOLLECT_ALL(str, ctx, cmt) \
	(void)

#undef B_TRANSLATE_NOCOLLECT_ID
#define B_TRANSLATE_NOCOLLECT_ID(id) \
	(void)

#undef B_TRANSLATE_NOCOLLECT_SYSTEM_NAME
#define B_TRANSLATE_NOCOLLECT_SYSTEM_NAME(str) \
	(void)

#endif	/* B_COLLECTING_CATKEYS */


/************************************************************************/
// For BCatalog add-on implementations:
// TODO: should go into another header

class BCatalogAddOn {
public:
								BCatalogAddOn(const char* signature,
									const char* language,
									uint32 fingerprint);
	virtual						~BCatalogAddOn();

	virtual	const char*			GetString(const char* string,
									const char* context = NULL,
									const char* comment = NULL) = 0;
	virtual	const char*			GetString(uint32 id) = 0;

			status_t			InitCheck() const;
			BCatalogAddOn*		Next();

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

	// magic marker functions which are used to mark a string/id
	// which will be translated elsewhere in the code (where it can
	// not be found since it is references by a variable):
	static	const char*			MarkForTranslation(const char* string,
									const char* context, const char* comment);
	static	int32				MarkForTranslation(int32 id);

			void				SetNext(BCatalogAddOn* next);

protected:
	virtual	void				UpdateFingerprint();

protected:
			friend class BCatalog;
			friend status_t get_add_on_catalog(BCatalog*, const char*);

			status_t 			fInitCheck;
			BString 			fSignature;
			BString 			fLanguageName;
			uint32				fFingerprint;
			BCatalogAddOn*		fNext;
};

// every catalog-add-on should export these symbols...
// ...the function that instantiates a catalog for this add-on-type...
extern "C"
BCatalogAddOn *instantiate_catalog(const char *signature,
	const char *language, uint32 fingerprint);
// ...the function that creates an empty catalog for this add-on-type...
extern "C"
BCatalogAddOn *create_catalog(const char *signature,
	const char *language);
// ...and the priority which will be used to order the catalog-add-ons:
extern uint8 gCatalogAddOnPriority;


/*
 * BCatalog - inlines for trivial accessors:
 */
inline status_t
BCatalog::GetSignature(BString *sig)
{
	if (!sig)
		return B_BAD_VALUE;
	if (!fCatalog)
		return B_NO_INIT;
	*sig = fCatalog->fSignature;
	return B_OK;
}


inline status_t
BCatalog::GetLanguage(BString *lang)
{
	if (!lang)
		return B_BAD_VALUE;
	if (!fCatalog)
		return B_NO_INIT;
	*lang = fCatalog->fLanguageName;
	return B_OK;
}


inline status_t
BCatalog::GetFingerprint(uint32 *fp)
{
	if (!fp)
		return B_BAD_VALUE;
	if (!fCatalog)
		return B_NO_INIT;
	*fp = fCatalog->fFingerprint;
	return B_OK;
}


inline const char *
BCatalog::GetNoAutoCollectString(const char *string, const char *context,
	const char *comment)
{
	return GetString(string, context, comment);
}


inline const char *
BCatalog::GetNoAutoCollectString(uint32 id)
{
	return GetString(id);
}


inline status_t
BCatalog::InitCheck() const
{
	return fCatalog
				? fCatalog->InitCheck()
				: B_NO_INIT;
}


inline int32
BCatalog::CountItems() const
{
	if (!fCatalog)
		return 0;
	return fCatalog->CountItems();
}


inline BCatalogAddOn *
BCatalog::CatalogAddOn()
{
	return fCatalog;
}


/*
 * BCatalogAddOn - inlines for trivial accessors:
 */
inline BCatalogAddOn *
BCatalogAddOn::Next()
{
	return fNext;
}

inline const char *
BCatalogAddOn::MarkForTranslation(const char *str, const char *ctx,
	const char *cmt)
{
	return str;
}


inline int32
BCatalogAddOn::MarkForTranslation(int32 id)
{
	return id;
}


// TODO: does not belong here, either
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

private:
								EditableCatalog();
								EditableCatalog(const EditableCatalog& other);
		const EditableCatalog&	operator=(const EditableCatalog& other);
									// hide assignment, default-, and
									// copy-constructor
};


} // namespace BPrivate


#endif /* _CATALOG_H_ */
