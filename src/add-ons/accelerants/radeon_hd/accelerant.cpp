/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include "utility.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <AGP.h>


#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


struct accelerant_info *gInfo;


class AreaCloner {
public:
								AreaCloner();
								~AreaCloner();

			area_id				Clone(const char *name, void **_address,
									uint32 spec, uint32 protection,
									area_id sourceArea);
			status_t			InitCheck()
									{return fArea < 0 ? (status_t)fArea : B_OK;}
			void				Keep();

private:
			area_id				fArea;
};


AreaCloner::AreaCloner()
	:
	fArea(-1)
{
}


AreaCloner::~AreaCloner()
{
	if (fArea >= 0)
		delete_area(fArea);
}


area_id
AreaCloner::Clone(const char *name, void **_address, uint32 spec,
	uint32 protection, area_id sourceArea)
{
	fArea = clone_area(name, _address, spec, protection, sourceArea);
	return fArea;
}


void
AreaCloner::Keep()
{
	fArea = -1;
}


//	#pragma mark -


/*! This is the common accelerant_info initializer. It is called by
	both, the first accelerant and all clones.
*/
static status_t
init_common(int device, bool isClone)
{
	// initialize global accelerant info structure

	gInfo = (accelerant_info *)malloc(sizeof(accelerant_info));
	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	gInfo->is_clone = isClone;
	gInfo->device = device;

	// get basic info from driver

	radeon_get_private_data data;
	data.magic = RADEON_PRIVATE_DATA_MAGIC;

	if (ioctl(device, RADEON_GET_PRIVATE_DATA, &data,
			sizeof(radeon_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("radeon hd shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		TRACE(("radeon_init_accelerant() failed shared area%i, %i\n",
			data.shared_info_area, gInfo->shared_info_area));
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("radeon hd regs",
		(void **)&gInfo->regs, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	sharedCloner.Keep();
	regsCloner.Keep();

	return B_OK;
}


/*! Clean up data common to both primary and cloned accelerant */
static void
uninit_common(void)
{
	delete_area(gInfo->regs_area);
	delete_area(gInfo->shared_info_area);

	gInfo->regs_area = gInfo->shared_info_area = -1;

	// close the file handle ONLY if we're the clone
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
}


//	#pragma mark - public accelerant functions


/*! Init primary accelerant */
status_t
radeon_init_accelerant(int device)
{
	TRACE(("radeon_init_accelerant()\n"));

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	radeon_shared_info &info = *gInfo->shared_info;

	init_lock(&info.accelerant_lock, "radeon hd accelerant");
	init_lock(&info.engine_lock, "radeon hd engine");


	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	TRACE(("radeon_init_accelerant() done\n"));
	return B_OK;
}


/*! This function is called for both, the primary accelerant and all of
	its clones.
*/
void
radeon_uninit_accelerant(void)
{
	TRACE(("radeon_uninit_accelerant()\n"));

	gInfo->mode_list = NULL;

	radeon_shared_info &info = *gInfo->shared_info;

	uninit_lock(&info.accelerant_lock);
	uninit_lock(&info.engine_lock);

	uninit_common();
}

