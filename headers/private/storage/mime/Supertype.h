//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file Supertype.h
	Supertype class declarations
*/

#ifndef _MIME_SUPERTYPE_H
#define _MIME_SUPERTYPE_H

#include <SupportDefs.h>

#include <string>
#include <set>

class BMessage;

namespace BPrivate {
namespace Storage {
namespace Mime {

class Supertype {
public:
	Supertype(const char *super = NULL);
	~Supertype();
	
	status_t GetInstalledSubtypes(BMessage *types);
		
	status_t AddSubtype(const char *sub);
	status_t RemoveSubtype(const char *sub);
	
	void SetName(const char *super);
	const char* GetName();
	
	status_t FillMessageWithTypes(BMessage &msg) const;
private:
	status_t CreateMessageWithTypes(BMessage **result) const;

	std::set<std::string> fSubtypes;
	BMessage *fCachedMessage;
	std::string fName;
};
	
} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_SUPERTYPE_H
