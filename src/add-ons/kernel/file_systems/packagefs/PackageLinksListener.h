/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINKS_LISTENER_H
#define PACKAGE_LINKS_LISTENER_H


class PackageLinkDirectory;


class PackageLinksListener {
public:
	virtual						~PackageLinksListener();

	virtual	void				PackageLinkDirectoryAdded(
										PackageLinkDirectory* directory) = 0;
	virtual	void				PackageLinkDirectoryRemoved(
										PackageLinkDirectory* directory) = 0;
};


#endif	// PACKAGE_LINKS_LISTENER_H
