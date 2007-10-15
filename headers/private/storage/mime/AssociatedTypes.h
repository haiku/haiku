/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_ASSOCIATED_TYPES_H
#define _MIME_ASSOCIATED_TYPES_H


#include <SupportDefs.h>

#include <map>
#include <set>
#include <string>

class BMessage;
class BString;
struct entry_ref;


namespace BPrivate {
namespace Storage {
namespace Mime {

class AssociatedTypes {
public:
	AssociatedTypes();
	~AssociatedTypes();
		
	status_t GetAssociatedTypes(const char *extension, BMessage *types);	
	status_t GuessMimeType(const char *filename, BString *result);
	status_t GuessMimeType(const entry_ref *ref, BString *result);

	status_t SetFileExtensions(const char *type, const BMessage *extensions);
	status_t DeleteFileExtensions(const char *type);
	
	void PrintToStream() const;	
private:
	status_t AddAssociatedType(const char *extension, const char *type);
	status_t RemoveAssociatedType(const char *extension, const char *type);

	status_t BuildAssociatedTypesTable();
	
	status_t ProcessType(const char *type);
	std::string PrepExtension(const char *extension) const;

	std::map<std::string, std::set<std::string> > fFileExtensions;	// mime type => set of associated file extensions
	std::map<std::string, std::set<std::string> > fAssociatedTypes;	// file extension => set of associated mime types
	
	bool fHaveDoneFullBuild;
};

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_ASSOCIATED_TYPES_H
