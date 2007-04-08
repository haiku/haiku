#ifdef HAIKU_HOST_USE_XATTR
#	include "fs_attr_xattr.cpp"
#else
#	include "fs_attr_generic.cpp"
#endif
