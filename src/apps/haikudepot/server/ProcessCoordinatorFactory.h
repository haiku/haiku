/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef PROCESS_COORDINATOR_FACTORY_H
#define PROCESS_COORDINATOR_FACTORY_H

#include <SupportDefs.h>

class Model;
class PackageInfoListener;
class ProcessCoordinator;
class ProcessCoordinatorListener;

/*! This class is able to create ProcessCoordinators that are loaded-up with
    Processes that together complete some larger job.
*/

class ProcessCoordinatorFactory {
public:
	static	ProcessCoordinator*	CreateBulkLoadCoordinator(
									PackageInfoListener *packageInfoListener,
									ProcessCoordinatorListener*
										processCoordinatorListener,
									Model* model, bool forceLocalUpdate);
private:
	static	uint32				_CalculateServerProcessOptions();

};

#endif // PROCESS_COORDINATOR_FACTORY_H