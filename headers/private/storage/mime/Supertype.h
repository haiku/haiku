// mime/Supertype.h

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
	Supertype(const char *super);
	~Supertype();
		
	void AddSubtype(const char *sub);
	void FillMessageWithTypes(BMessage *msg);
private:
	std::set<std::string> fSubtypes;
	BMessage *fCachedMessage;
	std::string fType;
};
	
} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_SUPERTYPE_H
