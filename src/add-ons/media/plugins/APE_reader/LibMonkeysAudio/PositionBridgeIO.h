#ifndef	___POSITION_BRIDGE_IO_H_
#define	___POSITION_BRIDGE_IO_H_
//------------------------------------------------------------------------------
// BeOS
#include <DataIO.h>
#include <SupportDefs.h>
// C++
// MAC
#include "NoWindows.h"	// before IO.h
#include "IO.h"
//------------------------------------------------------------------------------
//==============================================================================
class	TPositionBridgeIO : public CIO
{
public:
	TPositionBridgeIO();
	virtual	~TPositionBridgeIO();

	virtual int	Open(const wchar_t* oName);
	virtual int	Close();

	virtual int	Read(void* oBuf, unsigned int oBytesToRead, unsigned int* oBytesRead);
	virtual int	Write(const void* oBuf, unsigned int oBytesToWrite, unsigned int* oBytesWritten);

	virtual int	Seek(int oDistance, unsigned int oMoveMode);

	virtual int	Create(const wchar_t* oName);
	virtual int	Delete();

	virtual int	SetEOF();

	virtual int	GetPosition();
	virtual int	GetSize();
	virtual int	GetName(wchar_t* oBuffer);

	status_t	SetPositionIO(BPositionIO* oPositionIO);

private:
	BPositionIO*	mPositionIO;
};
//==============================================================================
#endif	// ___POSITION_BRIDGE_IO_H_
