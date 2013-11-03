/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


// This file is included in StringConstants.h and StringConstants.cpp with the
// macros DEFINE_STRING[_ARRAY]_CONSTANT() defined to generate the member
// variable declarations respectively the initializations.


DEFINE_STRING_CONSTANT(kPackageLinksDirectoryName, "package-links")
DEFINE_STRING_CONSTANT(kSelfLinkName, ".self")
DEFINE_STRING_CONSTANT(kSettingsLinkName, ".settings")

DEFINE_STRING_ARRAY_CONSTANT(kAutoPackageAttributeNames,
	AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT,
	"SYS:PACKAGE", "SYS:PACKAGE_FILE")
