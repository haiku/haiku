/**
 * @file MidiEndpoint.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#include "debug.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

const char* BMidiEndpoint::Name() const
{
	return fName.String();
}

//------------------------------------------------------------------------------

void BMidiEndpoint::SetName(const char* name)
{
	BMidiRoster *roster = BMidiRoster::MidiRoster();
	roster->Rename(this, name);
	fName.SetTo(name);
}

//------------------------------------------------------------------------------

int32 BMidiEndpoint::ID() const
{
	return fID;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsProducer() const
{
return (fFlags && 0x01) == 0x01;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsConsumer() const
{
return (fFlags && 0x01) == 0x01;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsRemote() const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsLocal() const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsPersistent() const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsValid() const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::Release()
{
	if (1 == atomic_add(&fRefCount, -1))
	{
		Unregister();
		delete this;
		return B_OK;
	}
	else
	{
		return B_ERROR;
	}
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::Acquire()
{
	atomic_add(&fRefCount, 1);
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::SetProperties(const BMessage* props)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::GetProperties(BMessage* props) const
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::Register(void)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::Unregister(void)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

BMidiEndpoint::BMidiEndpoint(const char* name)
{
	fName = BString(name);
	fStatus = B_OK;
	fFlags = 0;
	fRefCount = 0;
//	fID = MidiRosterApp::GetNextFreeID();
}

//------------------------------------------------------------------------------

BMidiEndpoint::~BMidiEndpoint()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiEndpoint::_Reserved1() { }
void BMidiEndpoint::_Reserved2() { }
void BMidiEndpoint::_Reserved3() { }
void BMidiEndpoint::_Reserved4() { }
void BMidiEndpoint::_Reserved5() { }
void BMidiEndpoint::_Reserved6() { }
void BMidiEndpoint::_Reserved7() { }
void BMidiEndpoint::_Reserved8() { }

//------------------------------------------------------------------------------

