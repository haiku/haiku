/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _MIME_INSTALLED_TYPES_H
#define _MIME_INSTALLED_TYPES_H

#include "Supertype.h"
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
		status_t _AddSupertype(const char *super,
					std::map<std::string, Supertype>::iterator &i);
		status_t _AddSubtype(const char *super, const char *sub);		
		status_t _AddSubtype(Supertype &super, const char *sub);

		status_t _RemoveSupertype(const char *super);
		status_t _RemoveSubtype(const char *super, const char *sub);		

		void _Unset();
		void _ClearCachedMessages();

		status_t _CreateMessageWithTypes(BMessage **result) const;
		status_t _CreateMessageWithSupertypes(BMessage **result) const;
		void _FillMessageWithSupertypes(BMessage *msg);

		status_t _BuildInstalledTypesList();

		std::map<std::string, Supertype> fSupertypes;
		BMessage *fCachedMessage;	
		BMessage *fCachedSupertypesMessage;
		bool fHaveDoneFullBuild;
};

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_INSTALLED_TYPES_H
