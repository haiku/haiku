#ifndef _DEFAULT_CATALOG_H_
#define _DEFAULT_CATALOG_H_

#ifdef __MWERKS__
#	include <hashmap.h>
#else
#	include <hash_map>
#endif

#include <LocaleBuild.h>

#include <Catalog.h>
#include <DataIO.h>
#include <String.h>

class BFile;


namespace BPrivate {
	struct CatKey;
}

/*
 * the hash-access functor which is being used to access the hash-value
 * stored inside of each key.
 */
#ifdef __MWERKS__
	// Codewarrior doesn't provide this declaration, so we do:
template <class T> struct hash : public unary_function<T,size_t> {
	size_t operator() (const T &key) const;
};
#endif

struct hash<BPrivate::CatKey> {
	size_t operator() (const BPrivate::CatKey &key) const;
};


namespace BPrivate {

/*
 * The key-type for the hash_map which maps native strings or IDs to
 * the corresponding translated string.
 * The key-type should be efficient to use if it is just created by an ID
 * but it should also support being created from up to three strings,
 * which as a whole specify the key to the translated string.
 */
struct _IMPEXP_LOCALE CatKey {
	BString fKey;
		// the key-string consists of three values separated by a special 
		// token:
		// - the native string
		// - the context of the string's usage
		// - a comment that can be used to separate strings that
		//   are identical otherwise (useful for the translator)
	size_t fHashVal;
		// the hash-value of fKey
	uint32 fFlags;
		// with respect to the catalog-editor, each translation can be
		// in different states (empty, unchecked, checked, etc.).
		// This state (and potential other flags) lives in the fFlags member.
	CatKey(const char *str, const char *ctx, const char *cmt);
	CatKey(uint32 id);
	CatKey();
	bool operator== (const CatKey& right) const;
	status_t GetStringParts(BString* str, BString* ctx, BString* cmt) const;
	static size_t HashFun(const char* s);
};

/*
 * The implementation of the Locale Kit's standard catalog-type.
 * Currently it only maps CatKey to a BString (the translated string),
 * but the value-type might change to add support for shortcuts and/or
 * graphical data (button-images and the like).
 */
class _IMPEXP_LOCALE DefaultCatalog : public BCatalogAddOn {

	public:
		DefaultCatalog(const char *signature, const char *language,
			int32 fingerprint);
				// constructor for normal use
		DefaultCatalog(entry_ref *appOrAddOnRef);
				// constructor for embedded catalog
		DefaultCatalog(const char *path, const char *signature, 
			const char *language);
				// constructor for editor-app
					   
		~DefaultCatalog();

		// overrides of BCatalogAddOn:
		const char *GetString(const char *string, const char *context = NULL,
						const char *comment = NULL);
		const char *GetString(uint32 id);
		const char *GetString(const CatKey& key);
		//
		status_t SetString(const char *string, const char *translated, 
					const char *context = NULL, const char *comment = NULL);
		status_t SetString(int32 id, const char *translated);
		status_t SetString(const CatKey& key, const char *translated);
		void UpdateFingerprint();

		// implementation for editor-interface:
		status_t ReadFromFile(const char *path = NULL);
		status_t ReadFromAttribute(entry_ref *appOrAddOnRef);
		status_t ReadFromResource(entry_ref *appOrAddOnRef);
		status_t WriteToFile(const char *path = NULL);
		status_t WriteToAttribute(entry_ref *appOrAddOnRef);
		status_t WriteToResource(entry_ref *appOrAddOnRef);
		//
		void MakeEmpty();
		int32 CountItems() const;

		static BCatalogAddOn *Instantiate(const char *signature,
								const char *language,
								int32 fingerprint);
		static BCatalogAddOn *InstantiateEmbedded(entry_ref *appOrAddOnRef);
		static BCatalogAddOn *Create(const char *signature,
								const char *language);
		static const uint8 kDefaultCatalogAddOnPriority;
		static const char *kCatMimeType;

	private:
		status_t Flatten(BDataIO *dataIO);
		status_t Unflatten(BDataIO *dataIO);
		int32 ComputeFingerprint() const;
		void UpdateAttributes(BFile& catalogFile);

		typedef hash_map<CatKey, BString, hash<CatKey>, equal_to<CatKey> > CatMap;
		CatMap 				fCatMap;
		mutable BString 	fPath;

	public:
		/*
		 * CatWalker
		 */
		class CatWalker {
			public:
				CatWalker() {}
				CatWalker(CatMap &catMap);
				bool AtEnd() const;
				const CatKey& GetKey() const;
				const char *GetValue() const;
				void Next();
			private:
				CatMap::iterator fPos;
				CatMap::iterator fEnd;
		};
		status_t GetWalker(CatWalker *walker);		
};

inline 
DefaultCatalog::CatWalker::CatWalker(CatMap &catMap)
	: fPos(catMap.begin()),
	  fEnd(catMap.end())
{
}

inline bool 
DefaultCatalog::CatWalker::AtEnd() const
{
	return fPos == fEnd;
}

inline const CatKey & 
DefaultCatalog::CatWalker::GetKey() const
{
	assert(fPos != fEnd);
	return fPos->first;
}

inline const char * 
DefaultCatalog::CatWalker::GetValue() const
{
	assert(fPos != fEnd);
	return fPos->second.String();
}

inline void 
DefaultCatalog::CatWalker::Next()
{
	++fPos;
}

inline status_t 
DefaultCatalog::GetWalker(CatWalker *walker)
{
	if (!walker)
		return B_BAD_VALUE;
	*walker = CatWalker(fCatMap);
	return B_OK;
}		

}	// namespace BPrivate

using namespace BPrivate;

#endif	/* _DEFAULT_CATALOG_H_ */
