/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_RESOLVALBE_H_
#define _PACKAGE__PACKAGE_RESOLVALBE_H_


#include <String.h>

#include <package/PackageVersion.h>


namespace BPackageKit {


enum BPackageResolvableType {
	B_PACKAGE_RESOLVABLE_TYPE_DEFAULT = 0,
	B_PACKAGE_RESOLVABLE_TYPE_LIBRARY,
	B_PACKAGE_RESOLVABLE_TYPE_COMMAND,
	B_PACKAGE_RESOLVABLE_TYPE_APPLICATION,
	B_PACKAGE_RESOLVABLE_TYPE_ADD_ON,
	//
	B_PACKAGE_RESOLVABLE_TYPE_ENUM_COUNT,
};


/*
 * Defines a resolvable (something other packages can depend upon).
 * Each resolvable is defined as a name (with an optional type prefix)
 * and an optional version.
 *
 * 		resolvable ::= <name>['='<version>]
 * 		name       ::= [<type>':']<word>
 * 		type       ::= 'lib' | 'cmd' | 'app' | 'add_on'
 *
 * The type doesn't have any specific meaning to the dependency resolver,
 * it just facilitates doing specific queries on the repository (like "is
 * there any package providing the 'svn' command that the user just typed?").
 * At a later stage, more types may be added in order to declare additional
 * entities, e.g. translators.
 *
 * String examples:
 * 		haiku=r1
 * 		lib:libssl=0.9.8i
 * 		subversion=1.5
 * 		cmd:svn
 */
class BPackageResolvable {
public:
								BPackageResolvable();
								BPackageResolvable(const BString& name,
									BPackageResolvableType type
										= B_PACKAGE_RESOLVABLE_TYPE_DEFAULT,
									const BPackageVersion& version
										= BPackageVersion());

			status_t			InitCheck() const;

			const BString&		Name() const;
			BPackageResolvableType	Type() const;
			const BPackageVersion& Version() const;

			void				GetAsString(BString& string) const;

			void				SetTo(const BString& name,
									BPackageResolvableType type
										= B_PACKAGE_RESOLVABLE_TYPE_DEFAULT,
									const BPackageVersion& version
										= BPackageVersion());
			void				Clear();

public:
	static	const char*			kTypeNames[];

private:
			BString				fName;
			BPackageResolvableType	fType;
			BPackageVersion		fVersion;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_RESOLVALBE_H_
