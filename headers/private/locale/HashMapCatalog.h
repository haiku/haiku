/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */
/*
 * This file declares all the things we need to add to a catalog add-on to be
 * able to use it in the developper tools (linkcatkeys, dumpcatalog, and the
 * catalog editor, when we will have one.
 */


#ifndef _HASH_MAP_CATALOG_H_
#define _HASH_MAP_CATALOG_H_


#include <assert.h>

#include <Catalog.h>
#include <HashMap.h>
#include <String.h>


namespace BPrivate {

/*
 * The key-type for the hash_map which maps native strings or IDs to
 * the corresponding translated string.
 * The key-type should be efficient to use if it is just created by an ID
 * but it should also support being created from up to three strings,
 * which as a whole specify the key to the translated string.
 */
class CatKey {
	public:
		BString fString;
			// The native string
		BString fContext;
			// The context of the string's usage
		BString fComment;
			// A comment that can be used to separate strings that
			// are identical otherwise in the native language, but different in
			// the translation (useful for the translator)
		uint32 fHashVal;
			// the hash-value of the key, computed from the three strings
		uint32 fFlags;
			// with respect to the catalog-editor, each translation can be
			// in different states (empty, unchecked, checked, etc.). This
			// state (and potential other flags) lives in the fFlags member.

	public:
		CatKey(const char *str, const char *ctx, const char *cmt);
		CatKey(uint32 id);
		CatKey();

		bool operator== (const CatKey& right) const;
		bool operator!= (const CatKey& right) const;
		status_t GetStringParts(BString* str, BString* ctx, BString* cmt) const;
		static uint32 HashFun(const char* s, int startvalue = 0);
			// The hash function is called 3 times, cumulating the 3 strings to
			// calculate the key
		uint32 GetHashCode() const { return fHashVal; }
};

} // namespace BPrivate


namespace BPrivate {

class BHashMapCatalog: public BCatalogAddOn {
	protected:
		uint32 ComputeFingerprint() const;
		typedef HashMap<CatKey, BString> CatMap;
		CatMap 				fCatMap;

	public:
		BHashMapCatalog(const char* signature, const char* language,
			uint32 fingerprint);
			// Constructor for normal use
			//
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

		// implementation for editor-interface
		virtual status_t ReadFromFile(const char *path = NULL)
			{return B_NOT_SUPPORTED;}
		virtual status_t ReadFromAttribute(const entry_ref &appOrAddOnRef)
			{return B_NOT_SUPPORTED;}
		virtual status_t ReadFromResource(const entry_ref &appOrAddOnRef)
			{return B_NOT_SUPPORTED;}
		virtual status_t WriteToFile(const char *path = NULL)
			{return B_NOT_SUPPORTED;}
		virtual status_t WriteToAttribute(const entry_ref &appOrAddOnRef)
			{return B_NOT_SUPPORTED;}
		virtual status_t WriteToResource(const entry_ref &appOrAddOnRef)
			{return B_NOT_SUPPORTED;}

		void UpdateFingerprint();
		//
		void MakeEmpty();
		int32 CountItems() const;

		/*
		 * CatWalker allows to walk trough all the strings stored in the
		 * catalog. We need that for dumpcatalog, linkcatkeys (to extract the
		 * data from the plaintext catalog) and in the catalog editor (to
		 * display the list of strings in a given catalog).
		 */
		class CatWalker {
			public:
				//CatWalker() {}; // if you use this there is no way to set fPos
				// properly.
				CatWalker(BHashMapCatalog* catalog);
				bool AtEnd() const;
				const CatKey& GetKey() const;
				const char *GetValue() const;
				void Next();
			private:
				CatMap::Iterator fPos;
				CatMap::Entry current;
				bool atEnd;
		};
		friend class CatWalker;
		status_t GetWalker(CatWalker *walker);


};


inline BHashMapCatalog::BHashMapCatalog(const char* signature,
	const char* language, uint32 fingerprint)
	:
	BCatalogAddOn(signature, language, fingerprint)
{
}


inline
BHashMapCatalog::CatWalker::CatWalker(BHashMapCatalog* catalog)
	:
	fPos(catalog->fCatMap.GetIterator())
{
	if (fPos.HasNext()) {
		current = fPos.Next();
		atEnd = false;
	} else
		atEnd = true;
}


inline bool
BHashMapCatalog::CatWalker::AtEnd() const
{
	return atEnd;
}


inline const CatKey &
BHashMapCatalog::CatWalker::GetKey() const
{
	assert(!atEnd);
	return current.key;
}


inline const char *
BHashMapCatalog::CatWalker::GetValue() const
{
	assert(!atEnd);
	return current.value.String();
}


inline void
BHashMapCatalog::CatWalker::Next()
{
	if (fPos.HasNext()) {
		current = fPos.Next();
		atEnd = false;
	} else
		atEnd = true;
}


inline status_t
BHashMapCatalog::GetWalker(CatWalker *walker)
{
	if (!walker)
		return B_BAD_VALUE;
	*walker = CatWalker(this);
	return B_OK;
}


} // namespace BPrivate

using namespace BPrivate;

#endif // _HASH_MAP_CATALOG_H_
