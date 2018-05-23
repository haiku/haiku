/*==============================================================================
	Bridge class for passing BPositionIO to functions who need CIO in MACLib
	Ver 1.01
	Copyright (C) 2005-2008 by SHINTA
==============================================================================*/

/*==============================================================================
BPositionIO* passed to SetPositionIO() must not be released before rleasing this class.
==============================================================================*/

//------------------------------------------------------------------------------
#include "PositionBridgeIO.h"
//------------------------------------------------------------------------------
// BeOS
// C++
// Proj
#include "All.h"
//------------------------------------------------------------------------------
//==============================================================================
TPositionBridgeIO::TPositionBridgeIO()
		: CIO()
{
	mPositionIO = NULL;
}
//------------------------------------------------------------------------------
TPositionBridgeIO::~TPositionBridgeIO()
{
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Close()
{
	return B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Create(const char* oName)
{
	return B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Delete()
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::GetName(char* oBuffer)
{
	strcpy(oBuffer, "<TPositionBridgeIO>");
	return B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::GetPosition()
{
	if ( mPositionIO == NULL )
		return 0;
	return mPositionIO->Position();
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::GetSize()
{
	if ( mPositionIO == NULL )
		return 0;

	off_t aSize;
	status_t err = mPositionIO->GetSize(&aSize);
	if (err != B_OK)
		return -1;

	return aSize;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Open(const char* oName)
{
	return B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Read(void* oBuf, unsigned int oBytesToRead, unsigned int* oBytesRead)
{
	if ( mPositionIO == NULL )
		return ERROR_IO_READ;
	*oBytesRead = mPositionIO->Read(oBuf, oBytesToRead);
	return *oBytesRead == 0 ? ERROR_IO_READ : B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::Seek(int oDistance, unsigned int oMoveMode)
{
	if ( mPositionIO == NULL )
		return B_ERROR;
	return mPositionIO->Seek(oDistance, oMoveMode) < B_OK ? B_ERROR : B_OK;
}
//------------------------------------------------------------------------------
int	TPositionBridgeIO::SetEOF()
{
	if ( mPositionIO == NULL )
		return B_ERROR;
	mPositionIO->SetSize(mPositionIO->Position());
	return B_OK;
}
//------------------------------------------------------------------------------
status_t	TPositionBridgeIO::SetPositionIO(BPositionIO* oPositionIO)
{
	mPositionIO = oPositionIO;
	return B_OK;
}
//------------------------------------------------------------------------------
/* ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
untested function
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' */
int	TPositionBridgeIO::Write(const void* oBuf, unsigned int oBytesToWrite, unsigned int* oBytesWritten)
{
	if ( mPositionIO == NULL )
		return ERROR_IO_WRITE;
	*oBytesWritten = mPositionIO->Write(oBuf, oBytesToWrite);
	return *oBytesWritten != oBytesToWrite ? ERROR_IO_WRITE : B_OK;
}
//------------------------------------------------------------------------------
//==============================================================================
