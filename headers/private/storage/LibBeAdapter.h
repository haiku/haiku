// LibBeAdapter.h

#ifndef _LIB_BE_ADAPTER_H
#define _LIB_BE_ADAPTER_H

#include <SupportDefs.h>

struct entry_ref;

extern status_t entry_ref_to_path_adapter(dev_t device, ino_t directory,
										  const char *name,
										  char *buffer, size_t size);

#endif	// _LIB_BE_ADAPTER_H


