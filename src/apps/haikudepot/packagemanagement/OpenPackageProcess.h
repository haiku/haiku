/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file has been re-factored from `PackageManager.h` and
 * copyrights have been carried across in 2021.
 */
#ifndef OPEN_PACKAGE_PROCESS_H
#define OPEN_PACKAGE_PROCESS_H


#include "AbstractPackageProcess.h"
#include "DeskbarLink.h"
#include "PackageProgressListener.h"


class OpenPackageProcess
	: public AbstractPackageProcess,
		private PackageProgressListener {
public:
								OpenPackageProcess(
									PackageInfoRef package, Model* model,
									const DeskbarLink& link);
	virtual						~OpenPackageProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

	static	bool				FindAppToLaunch(const PackageInfoRef& package,
									std::vector<DeskbarLink>& foundLinks);

protected:
	virtual	status_t			RunInternal();

private:
			BString				_DeriveDescription();

private:
			BString				fDescription;
			DeskbarLink			fDeskbarLink;
};

#endif // OPEN_PACKAGE_PROCESS_H
