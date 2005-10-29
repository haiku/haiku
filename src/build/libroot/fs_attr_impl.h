#ifndef FS_ATTR_IMPL_H
#define FS_ATTR_IMPL_H

#include <dirent.h>

#include <string>

#include "NodeRef.h"

namespace BPrivate {

DIR *open_attr_dir(NodeRef ref);

status_t get_attribute_path(NodeRef ref, const char *attribute,
	std::string &attrPath, std::string &typePath);

}	// namespace BPrivate

#endif	// FS_ATTR_IMPL_H
