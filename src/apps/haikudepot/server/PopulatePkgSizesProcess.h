/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef POPULATE_PKG_SIZES_PROCESS_H
#define POPULATE_PKG_SIZES_PROCESS_H

#include <String.h>

#include "AbstractProcess.h"
#include "Model.h"


/*!	This service will investigate any packages that do not have a size in the
	model. For these packages, it will attempt to derive a size based on the
	locally present package file.
 */

class PopulatePkgSizesProcess : public AbstractProcess {
public:
								PopulatePkgSizesProcess(Model* model);
	virtual						~PopulatePkgSizesProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

protected:
	virtual	status_t			RunInternal();

private:
			off_t				_DeriveSize(const PackageInfoRef modelInfo) const;

private:
			Model*				fModel;
};


#endif // POPULATE_PKG_SIZES_PROCESS_H
