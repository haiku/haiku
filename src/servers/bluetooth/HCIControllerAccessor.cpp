
#include "HCIControllerAccessor.h"

HCIControllerAccessor::HCIControllerAccessor(BPath* path) : HCIDelegate(path)
{


}

status_t
HCIControllerAccessor::IssueCommand(raw_command* rc, size_t size)
{

	if (GetID() < 0)
		return B_ERROR;


	return B_ERROR;
}
