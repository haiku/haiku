//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file InstalledTypes.h
	InstalledTypes class declarations
*/

#ifndef _MIME_INSTALLED_TYPES_H
#define _MIME_INSTALLED_TYPES_H

#include <mime/Supertype.h>
#include <SupportDefs.h>

#include <string>
#include <map>

class BMessage;

namespace BPrivate {
namespace Storage {
namespace Mime {

class InstalledTypes {
public:
	InstalledTypes();
	~InstalledTypes();
		
	status_t GetInstalledTypes(BMessage *types);
	status_t GetInstalledTypes(const char *supertype, BMessage *types);
	status_t GetInstalledSupertypes(BMessage *types);

	status_t AddType(const char *type);
	status_t RemoveType(const char *type);
private:
	status_t AddSupertype(const char *super, std::map<std::string, Supertype>::iterator &i);
	status_t AddSubtype(const char *super, const char *sub);		
	status_t AddSubtype(Supertype &super, const char *sub);
	
	status_t RemoveSupertype(const char *super);
	status_t RemoveSubtype(const char *super, const char *sub);		

	void Unset();
	void ClearCachedMessages();

	status_t CreateMessageWithTypes(BMessage **result) const;
	status_t CreateMessageWithSupertypes(BMessage **result) const;
	void FillMessageWithSupertypes(BMessage *msg);
	
	status_t BuildInstalledTypesList();

	std::map<std::string, Supertype> fSupertypes;
	BMessage *fCachedMessage;	
	BMessage *fCachedSupertypesMessage;
	bool fHaveDoneFullBuild;
};

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_INSTALLED_TYPES_H
