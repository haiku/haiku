# libgnuregex

This library exists because some systems don't have a flavor of regex library
built-in which supports groups.  This variant does include group-support.  An
example of where this comes into play is with the `BUrl` class where URLs are
parsed, in part, using regular expressions.

## Use with MacOS-X

In the case of MacOS-X, the dynamic-library build product
`libgnuregex_build.so` can be configured for use by configuring the
`DYLD_INSERT_LIBRARIES` environment variable.
