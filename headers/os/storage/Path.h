//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Path.h
	BPath interface declaration.
*/

#ifndef __sk_path_h__
#define __sk_path_h__

#include <Flattenable.h>
#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// Forward declarations
class BDirectory;
class BEntry;
struct entry_ref;

/*!
	\class BPath
	\brief An absolute pathname wrapper class
	
	Provides a convenient means of managing pathnames.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	\author <a href="mailto:tylerdauwalder@users.sf.net">Tyler Dauwalder</a>
	
	\version 0.0.0
*/
class BPath : public BFlattenable {
public:

	BPath();
	BPath(const BPath &path);
	BPath(const entry_ref *ref);
	BPath(const BEntry *entry);
	BPath(const char *dir, const char *leaf = NULL, bool normalize = false);
	BPath(const BDirectory *dir, const char *leaf, bool normalize = false);
	
	virtual ~BPath();

	status_t InitCheck() const;

	status_t SetTo(const entry_ref *ref);	
	status_t SetTo(const BEntry *entry);
	status_t SetTo(const char *path, const char *leaf = NULL,
				   bool normalize = false);
	status_t SetTo(const BDirectory *dir, const char *path,
				   bool normalize = false);
	void Unset();
	
	status_t Append(const char *path, bool normalize = false);
	
	const char *Path() const;
	const char *Leaf() const;
	status_t GetParent(BPath *path) const;
	
	bool operator==(const BPath &item) const;
	bool operator==(const char *path) const;
	bool operator!=(const BPath &item) const;
	bool operator!=(const char *path) const;
	BPath& operator=(const BPath &item);
	BPath& operator=(const char *path);
	
	// BFlattenable protocol
	virtual bool IsFixedSize() const;
	virtual type_code TypeCode() const;
	virtual ssize_t FlattenedSize() const;
	virtual status_t Flatten(void *buffer, ssize_t size) const;
	virtual bool AllowsTypeCode(type_code code) const;
	virtual status_t Unflatten(type_code c, const void *buf, ssize_t size);

private:
	virtual void _WarPath1();
	virtual void _WarPath2();
	virtual void _WarPath3();

	uint32 _warData[4];

	char *fName;	
	status_t fCStatus;

	class EBadInput { };

	status_t set_path(const char *path);

	static bool MustNormalize(const char *path);
};

#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

#endif	// __sk_path_h__
