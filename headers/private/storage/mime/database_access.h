//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file database_access.h
	Mime database atomic read functions and miscellany declarations
*/

#ifndef _MIME_DATABASE_ACCESS_H
#define _MIME_DATABASE_ACCESS_H

#include <Mime.h>

class BNode;
class BBitmap;
class BMessage;

namespace BPrivate {
namespace Storage {
namespace Mime {

// Get() functions
status_t get_app_hint(const char *type, entry_ref *ref);
status_t get_attr_info(const char *type, BMessage *info);
status_t get_short_description(const char *type, char *description);
status_t get_long_description(const char *type, char *description);
status_t get_file_extensions(const char *type, BMessage *extensions);
status_t get_icon(const char *type, BBitmap *icon, icon_size size);
status_t get_icon_for_type(const char *type, const char *fileType, BBitmap *icon,
			icon_size which);
status_t get_preferred_app(const char *type, char *signature, app_verb verb);
status_t get_sniffer_rule(const char *type, BString *result);
status_t get_supported_types(const char *type, BMessage *types);

bool is_installed(const char *type);

// Called by BMimeType to get properly formatted icon data ready
// to be shipped off to SetIcon*() and written to the database
status_t get_icon_data(const BBitmap *icon, icon_size size, void **data, int32 *dataSize);

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_DATABASE_H
