/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: FileInterface.cpp
 *  DESCR: 
 ***********************************************************************/
#include <FileInterface.h>
#include "debug.h"

/*************************************************************
 * protected BFileInterface
 *************************************************************/

BFileInterface::~BFileInterface()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * public BFileInterface
 *************************************************************/

/* nothing */

/*************************************************************
 * protected BFileInterface
 *************************************************************/

BFileInterface::BFileInterface()
	: BMediaNode("XXX fixme")
{
	UNIMPLEMENTED();

	AddNodeKind(B_FILE_INTERFACE);
}


status_t
BFileInterface::HandleMessage(int32 message,
							  const void *data,
							  size_t size)
{
	UNIMPLEMENTED();

	return B_OK;
}

/*************************************************************
 * private BFileInterface
 *************************************************************/

/*
private unimplemented
BFileInterface::BFileInterface(const BFileInterface &clone)
FileInterface & BFileInterface::operator=(const BFileInterface &clone)
*/

status_t BFileInterface::_Reserved_FileInterface_0(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_1(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_2(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_3(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_4(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_5(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_6(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_7(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_8(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_9(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_10(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_11(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_12(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_13(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_14(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_15(void *) { return B_ERROR; }

