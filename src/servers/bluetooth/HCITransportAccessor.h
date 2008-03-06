/*  */

#ifndef _HCITRANSPORT_ACCESSOR_H_
#define _HCITRANSPORT_ACCESSOR_H_

#include "HCIDelegate.h"


class HCITransportAccessor : public HCIDelegate {

	public:
		HCITransportAccessor(BPath* path);
		status_t IssueCommand(raw_command rc, size_t size);
};

#endif
