// mime/InstalledTypes.h

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

	void AddType(const char *type);
	void AddSupertype(const char *type);
	void AddSubtype(const char *super, const char *sub);		
private:
	void FillMessageWithTypes(BMessage *msg);
	void FillMessageWithSupertypes(BMessage *msg);

	std::map<std::string, Supertype> fSupertypes;
	BMessage *fCachedMessage;	
};

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_INSTALLED_TYPES_H
