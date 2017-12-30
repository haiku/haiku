#ifdef HAIKU_HOST_USE_XATTR
#	ifdef HAIKU_HOST_PLATFORM_HAIKU
#		include "fs_attr_haiku.cpp"
#	else
#		include "fs_attr_untyped.cpp"
#	endif
#else
#	include "fs_attr_generic.cpp"
#endif
