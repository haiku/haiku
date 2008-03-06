/*  */

#ifndef _HCICONTROLLER_ACCESSOR_H_
#define _HCICONTROLLER_ACCESSOR_H_

#include "HCIDelegate.h"


class HCIControllerAccessor : public HCIDelegate {

	public:
		HCIControllerAccessor(BPath* path);
		status_t IssueCommand(raw_command* rc,  size_t size);
};

#endif
