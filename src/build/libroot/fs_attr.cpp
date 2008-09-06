#ifdef HAIKU_HOST_USE_XATTR
#	include "fs_attr_untyped.cpp"
#else
#	include "fs_attr_generic.cpp"
#endif
