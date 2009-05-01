#ifndef _CATALOG_H_
#define _CATALOG_H_

#include <LocaleBuild.h>

#include <SupportDefs.h>
#include <String.h>

class BCatalogAddOn;
class BLocale;
class BMessage;
struct entry_ref;


class _IMPEXP_LOCALE BCatalog {

	public:
		BCatalog();
		BCatalog(const char *signature, const char *language = NULL,
			int32 fingerprint = 0);
		virtual ~BCatalog();

		const char *GetString(const char *string, const char *context = NULL,
						const char *comment = NULL);
		const char *GetString(uint32 id);

		status_t GetData(const char *name, BMessage *msg);
		status_t GetData(uint32 id, BMessage *msg);

		status_t GetSignature(BString *sig);
		status_t GetLanguage(BString *lang);
		status_t GetFingerprint(int32 *fp);

		status_t InitCheck() const;
		int32 CountItems() const;

		BCatalogAddOn *CatalogAddOn();

	protected:
		BCatalog(const BCatalog&);
		const BCatalog& operator= (const BCatalog&);
			// hide assignment and copy-constructor

		static status_t GetAppCatalog(BCatalog*);

		BCatalogAddOn *fCatalog;

	private:
		friend class BLocale;
		friend status_t get_add_on_catalog(BCatalog*, const char *);
};


extern _IMPEXP_LOCALE BCatalog* be_catalog;
extern _IMPEXP_LOCALE BCatalog* be_app_catalog;


#ifndef B_AVOID_TRANSLATION_MACROS
// macros for easy catalog-access, define B_AVOID_TRANSLATION_MACROS if
// you don't want these:

#undef TR_CONTEXT
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
	// programmer needs to define TR_CONTEXT with the context he'd like 
	// to be associated with the strings used in this specifc source file.
	// example:
	//		#define TR_CONTEXT "Folder-Window"
	// Tip: Use a descriptive name of the class implemented in that 
	//		source-file. 


// Translation macros which may be used to shorten translation requests:
#undef TR
#define TR(str) \
	be_catalog->GetString((str), TR_CONTEXT)

#undef TR_CMT
#define TR_CMT(str,cmt) \
	be_catalog->GetString((str), TR_CONTEXT, (cmt))

#undef TR_ALL
#define TR_ALL(str,ctx,cmt) \
	be_catalog->GetString((str), (ctx), (cmt))

#undef TR_ID
#define TR_ID(id) \
	be_catalog->GetString((id))


// Translation markers which can be used to mark static strings/IDs which
// are used as key for translation requests (at other places in the code):
/* example:
		#define TR_CONTEXT "MyDecentApp-Menu"

		static const char *choices[] = {
			TR_MARK("left"),
			TR_MARK("right"),
			TR_MARK("up"),
			TR_MARK("down")
		};

		void MyClass::AddChoices(BMenu *menu) {
			for (char **ch = choices; *ch; ch++) {
				menu->AddItem(
					new BMenuItem(
						TR(*ch), 
						new BMessage(...)
					)
				)
			}
		}
*/
#undef TR_MARK
#define TR_MARK(str) \
	BCatalogAddOn::MarkForTranslation((str), TR_CONTEXT, "")

#undef TR_MARK_CMT
#define TR_MARK_CMT(str,cmt) \
	BCatalogAddOn::MarkForTranslation((str), TR_CONTEXT, (cmt))

#undef TR_MARK_ALL
#define TR_MARK_ALL(str,ctx,cmt) \
	BCatalogAddOn::MarkForTranslation((str), (ctx), (cmt))
	
#undef TR_MARK_ID
#define TR_MARK_ID(id) \
	BCatalogAddOn::MarkForTranslation((id))
	
#endif	/* B_AVOID_TRANSLATION_MACROS */


/************************************************************************/
// For BCatalog add-on implementations:

class _IMPEXP_LOCALE BCatalogAddOn {
		friend class BLocaleRoster;
	public:
		BCatalogAddOn(const char *signature, const char *language,
					  int32 fingerprint);
		virtual ~BCatalogAddOn();

		virtual const char *GetString(const char *string, 
								const char *context=NULL,
								const char *comment=NULL) = 0;
		virtual const char *GetString(uint32 id) = 0;

		status_t InitCheck() const;
		BCatalogAddOn *Next();

		// the following could be used to localize non-textual data (e.g. icons),
		// but these will only be implemented if there's demand for such a 
		// feature:
		virtual bool CanHaveData() const;
		virtual status_t GetData(const char *name, BMessage *msg);
		virtual status_t GetData(uint32 id, BMessage *msg);

		// interface for catalog-editor-app and testing apps:
		virtual status_t SetString(const char *string, 
							const char *translated,
							const char *context=NULL,
							const char *comment=NULL);
		virtual status_t SetString(int32 id, const char *translated);
		//
		virtual bool CanWriteData() const;
		virtual status_t SetData(const char *name, BMessage *msg);
		virtual status_t SetData(uint32 id, BMessage *msg);
		//
		virtual status_t ReadFromFile(const char *path = NULL);
		virtual status_t ReadFromAttribute(entry_ref *appOrAddOnRef);
		virtual status_t ReadFromResource(entry_ref *appOrAddOnRef);
		virtual status_t WriteToFile(const char *path = NULL);
		virtual status_t WriteToAttribute(entry_ref *appOrAddOnRef);
		virtual status_t WriteToResource(entry_ref *appOrAddOnRef);
		//
		virtual void MakeEmpty();
		virtual int32 CountItems() const;

		// magic marker functions which are used to mark a string/id
		// which will be translated elsewhere in the code (where it can
		// not be found since it is references by a variable):
		static const char *MarkForTranslation(const char *str, const char *ctx, 
								const char *cmt);
		static int32 MarkForTranslation(int32 id);

	protected:
		virtual void UpdateFingerprint();

		status_t 			fInitCheck;
		BString 			fSignature;
		BString 			fLanguageName;
		int32				fFingerprint;
		BCatalogAddOn 		*fNext;
		
		friend class BCatalog;
		friend status_t get_add_on_catalog(BCatalog*, const char *);
};

// every catalog-add-on should export these symbols...
// ...the function that instantiates a catalog for this add-on-type...
extern "C" _IMPEXP_LOCALE 
BCatalogAddOn *instantiate_catalog(const char *signature,
	const char *language, int32 fingerprint);
// ...the function that creates an empty catalog for this add-on-type...
extern "C" _IMPEXP_LOCALE 
BCatalogAddOn *create_catalog(const char *signature,
	const char *language);
// ...and the priority which will be used to order the catalog-add-ons:
extern _IMPEXP_LOCALE uint8 gCatalogAddOnPriority;


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
BCatalog::GetFingerprint(int32 *fp) 
{
	if (!fp)
		return B_BAD_VALUE;
	if (!fCatalog)
		return B_NO_INIT;
	*fp = fCatalog->fFingerprint;
	return B_OK;
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


namespace BPrivate {

/*
 * EditableCatalog
 */
class _IMPEXP_LOCALE EditableCatalog : public BCatalog {

	public:
		EditableCatalog(const char *type, const char *signature, 
			const char *language);
		~EditableCatalog();

		status_t SetString(const char *string, 
					const char *translated,
					const char *context=NULL,
					const char *comment=NULL);
		status_t SetString(int32 id, const char *translated);
		//
		bool CanWriteData() const;
		status_t SetData(const char *name, BMessage *msg);
		status_t SetData(uint32 id, BMessage *msg);
		//
		status_t ReadFromFile(const char *path = NULL);
		status_t ReadFromAttribute(entry_ref *appOrAddOnRef);
		status_t ReadFromResource(entry_ref *appOrAddOnRef);
		status_t WriteToFile(const char *path = NULL);
		status_t WriteToAttribute(entry_ref *appOrAddOnRef);
		status_t WriteToResource(entry_ref *appOrAddOnRef);
		//
		void MakeEmpty();

	private:
		EditableCatalog();
		EditableCatalog(const EditableCatalog&);
		const EditableCatalog& operator= (const EditableCatalog&);
			// hide assignment, default- and copy-constructor

};

} // namespace BPrivate

#endif	/* _CATALOG_H_ */
