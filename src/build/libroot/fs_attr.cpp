#ifdef HAIKU_HOST_USE_XATTR
#	ifdef HAIKU_HOST_PLATFORM_HAIKU
		// Do nothing; we allow libroot's fs_attr symbols to shine through.
#	else
#		include "fs_attr_untyped.cpp"
#	endif
#else
#	include "fs_attr_generic.cpp"
#endif
