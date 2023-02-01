/*
 * Copyright 2021-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef INCREMENT_VIEW_COUNTER_PROCESS_H
#define INCREMENT_VIEW_COUNTER_PROCESS_H

#include "AbstractProcess.h"
#include "Model.h"
#include "PackageInfo.h"


class Model;


class IncrementViewCounterProcess : public AbstractProcess {
public:
								IncrementViewCounterProcess(
									Model* model,
									const PackageInfoRef& package);
	virtual						~IncrementViewCounterProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual	status_t			RunInternal();

private:
			void				_SpinBetweenAttempts();

private:
			BString				fDescription;
			PackageInfoRef		fPackage;
			Model*				fModel;
};

#endif // INCREMENT_VIEW_COUNTER_PROCESS_H
