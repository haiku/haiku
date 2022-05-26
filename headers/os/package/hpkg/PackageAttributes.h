/*
 * Copyright 2009-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


// This file defines the HPKG file attributes with all their properties in one
// place. Includers of the file need to define the macro
// B_DEFINE_HPKG_ATTRIBUTE(id, type, name, constant) so that it evaluates to
// whatever is desired in their context.


B_DEFINE_HPKG_ATTRIBUTE( 0, STRING,	"dir:entry",			DIRECTORY_ENTRY)
B_DEFINE_HPKG_ATTRIBUTE( 1, UINT,	"file:type",			FILE_TYPE)
B_DEFINE_HPKG_ATTRIBUTE( 2, UINT,	"file:permissions",		FILE_PERMISSIONS)
B_DEFINE_HPKG_ATTRIBUTE( 3, STRING,	"file:user",			FILE_USER)
B_DEFINE_HPKG_ATTRIBUTE( 4, STRING,	"file:group",			FILE_GROUP)
B_DEFINE_HPKG_ATTRIBUTE( 5, UINT,	"file:atime",			FILE_ATIME)
B_DEFINE_HPKG_ATTRIBUTE( 6, UINT,	"file:mtime",			FILE_MTIME)
B_DEFINE_HPKG_ATTRIBUTE( 7, UINT,	"file:crtime",			FILE_CRTIME)
B_DEFINE_HPKG_ATTRIBUTE( 8, UINT,	"file:atime:nanos",		FILE_ATIME_NANOS)
B_DEFINE_HPKG_ATTRIBUTE( 9, UINT,	"file:mtime:nanos",		FILE_MTIME_NANOS)
B_DEFINE_HPKG_ATTRIBUTE(10, UINT,	"file:crtime:nanos",	FILE_CRTIM_NANOS)
B_DEFINE_HPKG_ATTRIBUTE(11, STRING,	"file:attribute",		FILE_ATTRIBUTE)
B_DEFINE_HPKG_ATTRIBUTE(12, UINT,	"file:attribute:type",	FILE_ATTRIBUTE_TYPE)
B_DEFINE_HPKG_ATTRIBUTE(13, RAW,	"data",					DATA)
B_DEFINE_HPKG_ATTRIBUTE(14, STRING,	"symlink:path",			SYMLINK_PATH)
B_DEFINE_HPKG_ATTRIBUTE(15, STRING,	"package:name",			PACKAGE_NAME)
B_DEFINE_HPKG_ATTRIBUTE(16, STRING,	"package:summary",		PACKAGE_SUMMARY)
B_DEFINE_HPKG_ATTRIBUTE(17, STRING,	"package:description",	PACKAGE_DESCRIPTION)
B_DEFINE_HPKG_ATTRIBUTE(18, STRING,	"package:vendor",		PACKAGE_VENDOR)
B_DEFINE_HPKG_ATTRIBUTE(19, STRING,	"package:packager",		PACKAGE_PACKAGER)
B_DEFINE_HPKG_ATTRIBUTE(20, UINT,	"package:flags",		PACKAGE_FLAGS)
B_DEFINE_HPKG_ATTRIBUTE(21, UINT,	"package:architecture",
	PACKAGE_ARCHITECTURE)
B_DEFINE_HPKG_ATTRIBUTE(22, STRING,	"package:version.major",
	PACKAGE_VERSION_MAJOR)
B_DEFINE_HPKG_ATTRIBUTE(23, STRING,	"package:version.minor",
	PACKAGE_VERSION_MINOR)
B_DEFINE_HPKG_ATTRIBUTE(24, STRING,	"package:version.micro",
	PACKAGE_VERSION_MICRO)
B_DEFINE_HPKG_ATTRIBUTE(25, UINT,	"package:version.revision",
	PACKAGE_VERSION_REVISION)
B_DEFINE_HPKG_ATTRIBUTE(26, STRING,	"package:copyright",	PACKAGE_COPYRIGHT)
B_DEFINE_HPKG_ATTRIBUTE(27, STRING,	"package:license",		PACKAGE_LICENSE)
B_DEFINE_HPKG_ATTRIBUTE(28, STRING,	"package:provides",		PACKAGE_PROVIDES)
B_DEFINE_HPKG_ATTRIBUTE(29, STRING,	"package:requires",		PACKAGE_REQUIRES)
B_DEFINE_HPKG_ATTRIBUTE(30, STRING,	"package:supplements",	PACKAGE_SUPPLEMENTS)
B_DEFINE_HPKG_ATTRIBUTE(31, STRING,	"package:conflicts",	PACKAGE_CONFLICTS)
B_DEFINE_HPKG_ATTRIBUTE(32, STRING,	"package:freshens",		PACKAGE_FRESHENS)
B_DEFINE_HPKG_ATTRIBUTE(33, STRING,	"package:replaces",		PACKAGE_REPLACES)
B_DEFINE_HPKG_ATTRIBUTE(34, UINT,	"package:resolvable.operator",
	PACKAGE_RESOLVABLE_OPERATOR)
B_DEFINE_HPKG_ATTRIBUTE(35, STRING,	"package:checksum",		PACKAGE_CHECKSUM)
B_DEFINE_HPKG_ATTRIBUTE(36, STRING,	"package:version.prerelease",
	PACKAGE_VERSION_PRE_RELEASE)
B_DEFINE_HPKG_ATTRIBUTE(37, STRING,	"package:provides.compatible",
	PACKAGE_PROVIDES_COMPATIBLE)
B_DEFINE_HPKG_ATTRIBUTE(38, STRING,	"package:url",			PACKAGE_URL)
B_DEFINE_HPKG_ATTRIBUTE(39, STRING,	"package:source-url",	PACKAGE_SOURCE_URL)
B_DEFINE_HPKG_ATTRIBUTE(40, STRING,	"package:install-path",
	PACKAGE_INSTALL_PATH)
B_DEFINE_HPKG_ATTRIBUTE(41, STRING,	"package:base-package",
	PACKAGE_BASE_PACKAGE)
B_DEFINE_HPKG_ATTRIBUTE(42, STRING,	"package:global-writable-file",
	PACKAGE_GLOBAL_WRITABLE_FILE)
B_DEFINE_HPKG_ATTRIBUTE(43, STRING,	"package:user-settings-file",
	PACKAGE_USER_SETTINGS_FILE)
B_DEFINE_HPKG_ATTRIBUTE(44, UINT,	"package:writable-file-update-type",
	PACKAGE_WRITABLE_FILE_UPDATE_TYPE)
B_DEFINE_HPKG_ATTRIBUTE(45, STRING,	"package:settings-file-template",
	PACKAGE_SETTINGS_FILE_TEMPLATE)
B_DEFINE_HPKG_ATTRIBUTE(46, STRING,	"package:user",			PACKAGE_USER)
B_DEFINE_HPKG_ATTRIBUTE(47, STRING,	"package:user.real-name",
	PACKAGE_USER_REAL_NAME)
B_DEFINE_HPKG_ATTRIBUTE(48, STRING,	"package:user.home",	PACKAGE_USER_HOME)
B_DEFINE_HPKG_ATTRIBUTE(49, STRING,	"package:user.shell",	PACKAGE_USER_SHELL)
B_DEFINE_HPKG_ATTRIBUTE(50, STRING,	"package:user.group",	PACKAGE_USER_GROUP)
B_DEFINE_HPKG_ATTRIBUTE(51, STRING,	"package:group",		PACKAGE_GROUP)
B_DEFINE_HPKG_ATTRIBUTE(52, STRING,	"package:post-install-script",
	PACKAGE_POST_INSTALL_SCRIPT)
B_DEFINE_HPKG_ATTRIBUTE(53, UINT,	"package:is-writable-directory",
	PACKAGE_IS_WRITABLE_DIRECTORY)
B_DEFINE_HPKG_ATTRIBUTE(54, STRING,	"package",				PACKAGE)
B_DEFINE_HPKG_ATTRIBUTE(55, STRING,	"package:pre-uninstall-script",
	PACKAGE_PRE_UNINSTALL_SCRIPT)
// Note: add new entries at the end to avoid breaking index numbers, which are
// in previously built .hpkg files the build process downloads from elsewhere.
// Also remember to bump B_HPKG_MINOR_VERSION and B_HPKG_REPO_MINOR_VERSION.
// And yes, the build (on Haiku) runs packaging tools compiled using your code,
// which makes it extra fun to debug :-)
