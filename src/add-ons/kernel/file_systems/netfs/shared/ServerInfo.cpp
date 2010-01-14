// ServerInfo.cpp

#include "RequestFlattener.h"
#include "RequestUnflattener.h"
#include "ServerInfo.h"

// VisitString
static
void
VisitString(RequestMember* member, RequestMemberVisitor* visitor,
	HashString& string)
{
	StringData stringData;
	stringData.SetTo(string.GetString());
	visitor->Visit(member, stringData);
}


// #pragma mark -
// #pragma mark ----- ShareInfo -----

// constructor
ShareInfo::ShareInfo()
	: FlattenableRequestMember(),
	  fShareName()
{
}

// IsValid
bool
ShareInfo::IsValid() const
{
	return (fShareName.GetLength() > 0);
}

// ShowAround
void
ShareInfo::ShowAround(RequestMemberVisitor* visitor)
{
	VisitString(this, visitor, fShareName);
}

// Flatten
status_t
ShareInfo::Flatten(RequestFlattener* flattener)
{
	return flattener->WriteString(fShareName.GetString());
}

// Unflatten
status_t
ShareInfo::Unflatten(RequestUnflattener* unflattener)
{
	return unflattener->ReadString(fShareName);
}

// SetShareName
status_t
ShareInfo::SetShareName(const char* shareName)
{
	return (fShareName.SetTo(shareName) ? B_OK : B_NO_MEMORY);
}

// GetShareName
const char*
ShareInfo::GetShareName() const
{
	return fShareName.GetString();
}


// #pragma mark -
// #pragma mark ----- ServerInfo -----

// constructor
ServerInfo::ServerInfo()
	: FlattenableRequestMember(),
	  fServerName(),
	  fConnectionMethod(),
	  fShareInfos()
{
}

// constructor
ServerInfo::ServerInfo(const ServerInfo& other)
	: FlattenableRequestMember(),
	  fServerName(),
	  fConnectionMethod(),
	  fShareInfos()
{
	(*this) = other;
}

// ShowAround
void
ServerInfo::ShowAround(RequestMemberVisitor* visitor)
{
	VisitString(this, visitor, fServerName);
	VisitString(this, visitor, fConnectionMethod);
	int32 count = fShareInfos.Count();
	visitor->Visit(this, count);
	for (int32 i = 0; i < count; i++)
		visitor->Visit(this, fShareInfos.ElementAt(i));
}

// Flatten
status_t
ServerInfo::Flatten(RequestFlattener* flattener)
{
	flattener->WriteString(fServerName.GetString());
	flattener->WriteString(fConnectionMethod.GetString());

	int32 count = fShareInfos.Count();
	flattener->WriteInt32(count);
	for (int32 i = 0; i < count; i++)
		flattener->Visit(this, fShareInfos.ElementAt(i));

	return flattener->GetStatus();
}

// Unflatten
status_t
ServerInfo::Unflatten(RequestUnflattener* unflattener)
{
	unflattener->ReadString(fServerName);
	unflattener->ReadString(fConnectionMethod);

	int32 count;
	if (unflattener->ReadInt32(count) != B_OK)
		return unflattener->GetStatus();

	for (int32 i = 0; i < count; i++) {
		ShareInfo info;
		unflattener->Visit(this, info);
		if (info.IsValid())
			AddShare(info.GetShareName());
	}

	return unflattener->GetStatus();
}

// SetServerName
status_t
ServerInfo::SetServerName(const char* serverName)
{
	return (fServerName.SetTo(serverName) ? B_OK : B_NO_MEMORY);
}

// GetServerName
const char*
ServerInfo::GetServerName() const
{
	return fServerName.GetString();
}

// SetConnectionMethod
status_t
ServerInfo::SetConnectionMethod(const char* connectionMethod)
{
	return (fConnectionMethod.SetTo(connectionMethod) ? B_OK : B_NO_MEMORY);
}

// GetConnectionMethod
const char*
ServerInfo::GetConnectionMethod() const
{
	return fConnectionMethod.GetString();
}

// AddShare
status_t
ServerInfo::AddShare(const char* shareName)
{
	ShareInfo shareInfo;
	status_t error = shareInfo.SetShareName(shareName);
	if (error == B_OK)
		error = fShareInfos.PushBack(shareInfo);
	return error;
}

// CountShares
int32
ServerInfo::CountShares() const
{
	return fShareInfos.Count();
}

// ShareInfoAt
const ShareInfo&
ServerInfo::ShareInfoAt(int32 index) const
{
	return fShareInfos.ElementAt(index);
}

// =
ServerInfo&
ServerInfo::operator=(const ServerInfo& other)
{
	fServerName = other.fServerName;
	fConnectionMethod = other.fConnectionMethod;
	fShareInfos.MakeEmpty();
	int32 count = other.fShareInfos.Count();
	for (int32 i = 0; i < count; i++)
		fShareInfos.PushBack(other.fShareInfos.ElementAt(i));
	return *this;
}

