// LibBeAdapter.h

#ifndef _sk_lib_be_adapter_h_
#define _sk_lib_be_adapter_h_

#include <SupportDefs.h>

struct entry_ref;

extern status_t entry_ref_to_path_adapter(dev_t device, ino_t directory,
										  const char *name,
										  char *buffer, size_t size);

extern status_t swap_data_adapter(type_code type, void *data, size_t length,
								  swap_action action);

extern bool is_type_swapped_adapter(type_code type);

#endif	// _sk_lib_be_adapter_h_
