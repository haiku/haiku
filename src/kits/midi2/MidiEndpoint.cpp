
#include "debug.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

const char* BMidiEndpoint::Name() const
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

void BMidiEndpoint::SetName(const char* name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

int32 BMidiEndpoint::ID() const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsProducer() const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

bool BMidiEndpoint::IsConsumer() const
{
	UNIMPLEMENTED
	return false;
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
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiEndpoint::Acquire()
{
	UNIMPLEMENTED
	return B_ERROR;
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
	UNIMPLEMENTED
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

