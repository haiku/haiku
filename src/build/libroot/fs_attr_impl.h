#ifndef FS_ATTR_IMPL_H
#define FS_ATTR_IMPL_H

#include <dirent.h>

#include <string>

#include "NodeRef.h"

namespace BPrivate {

// defined in fs_attr.cpp

// Note: Only one of path or fd can be supplied.
DIR *open_attr_dir(NodeRef ref, const char *path, int fd);

// Note: Only one of path or fd can be supplied.
status_t get_attribute_path(NodeRef ref, const char *path, int fd,
	const char *attribute, std::string &attrPath, std::string &typePath);


// defined in fs.cpp

status_t get_path(int fd, const char *name, std::string &path);

}	// namespace BPrivate

#endif	// FS_ATTR_IMPL_H
