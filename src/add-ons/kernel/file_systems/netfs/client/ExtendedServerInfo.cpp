// ExtendedServerInfo.cpp

#include "ExtendedServerInfo.h"
#include "ServerInfo.h"

// constructor
ExtendedShareInfo::ExtendedShareInfo()
	:
	BReferenceable(),
	fShareName()
{
}

// SetTo
status_t
ExtendedShareInfo::SetTo(const ShareInfo* shareInfo)
{
	if (!shareInfo)
		return B_BAD_VALUE;
	if (!fShareName.SetTo(shareInfo->GetShareName()))
		return B_NO_MEMORY;
	return B_OK;
}

// GetShareName
const char*
ExtendedShareInfo::GetShareName() const
{
	return fShareName.GetString();
}


// #pragma mark -

// constructor
ExtendedServerInfo::ExtendedServerInfo(const NetAddress& address)
	:
	BReferenceable(),
	fAddress(address),
	fState(0)
{
}

// destructor
ExtendedServerInfo::~ExtendedServerInfo()
{
	int32 count = CountShares();
	for (int32 i = 0; i < count; i++)
		ShareInfoAt(i)->ReleaseReference();
}

// GetAddress
const NetAddress&
ExtendedServerInfo::GetAddress() const
{
	return fAddress;
}

// GetServerName
const char*
ExtendedServerInfo::GetServerName() const
{
	return fServerName.GetString();
}

// GetConnectionMethod
const char*
ExtendedServerInfo::GetConnectionMethod() const
{
	return fConnectionMethod.GetString();
}

// CountShares
int32
ExtendedServerInfo::CountShares() const
{
	return fShareInfos.Count();
}

// ShareInfoAt
ExtendedShareInfo*
ExtendedServerInfo::ShareInfoAt(int32 index) const
{
	if (index < 0 || index >= fShareInfos.Count())
		return NULL;
	return fShareInfos.ElementAt(index);
}

// GetShareInfo
ExtendedShareInfo*
ExtendedServerInfo::GetShareInfo(const char* name)
{
	for (int32 i = 0; ExtendedShareInfo* shareInfo = ShareInfoAt(i); i++) {
		if (strcmp(shareInfo->GetShareName(), name) == 0)
			return shareInfo;
	}

	return NULL;
}

// SetTo
status_t
ExtendedServerInfo::SetTo(ServerInfo* serverInfo)
{
	if (!serverInfo)
		return B_BAD_VALUE;
	// set name and connection method
	const char* name = serverInfo->GetServerName();
	HashString addressString;
	if (!name || strlen(name) == 0) {
		status_t error = fAddress.GetString(&addressString, false);
		if (error != B_OK)
			return error;
		name = addressString.GetString();
	}
	if (!fServerName.SetTo(name)
		|| !fConnectionMethod.SetTo(serverInfo->GetConnectionMethod())) {
		return B_NO_MEMORY;
	}
	// add the shares
	int32 shareCount = serverInfo->CountShares();
	for (int32 i = 0; i < shareCount; i++) {
		const ShareInfo& shareInfo = serverInfo->ShareInfoAt(i);
		status_t error = _AddShare(&shareInfo);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}

// SetState
void
ExtendedServerInfo::SetState(uint32 state)
{
	fState = state;
}

// GetState
uint32
ExtendedServerInfo::GetState() const
{
	return fState;
}

// _AddShare
status_t
ExtendedServerInfo::_AddShare(const ShareInfo* info)
{
	ExtendedShareInfo* extendedInfo = new(std::nothrow) ExtendedShareInfo;
	if (!extendedInfo)
		return B_NO_MEMORY;
	status_t error = extendedInfo->SetTo(info);
	if (error != B_OK) {
		delete extendedInfo;
		return error;
	}
	error = fShareInfos.PushBack(extendedInfo);
	if (error != B_OK) {
		delete extendedInfo;
		return error;
	}
	return B_OK;
}

