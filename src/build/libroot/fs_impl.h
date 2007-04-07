#ifndef FS_IMPL_H
#define FS_IMPL_H

#include <string>

#include <SupportDefs.h>

namespace BPrivate {

// defined in fs.cpp

status_t get_path(int fd, const char *name, std::string &path);

}	// namespace BPrivate

#endif	// FS_IMPL_H
