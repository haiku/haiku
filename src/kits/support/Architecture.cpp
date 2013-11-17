/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Architecture.h>

#include <algorithm>

#include <StringList.h>


static const size_t kMaxArchitectureCount = 16;


static status_t
string_array_to_string_list(const char* const* architectures, size_t count,
	BStringList& _architectures)
{
	_architectures.MakeEmpty();

	for (size_t i = 0; i < count; i++) {
		BString architecture(architectures[i]);
		if (architecture.IsEmpty() || !_architectures.Add(architecture)) {
			_architectures.MakeEmpty();
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
get_secondary_architectures(BStringList& _architectures)
{
	const char* architectures[kMaxArchitectureCount];
	size_t count = get_secondary_architectures(architectures,
		kMaxArchitectureCount);
	return string_array_to_string_list(architectures,
		std::min(count, kMaxArchitectureCount), _architectures);
}


status_t
get_architectures(BStringList& _architectures)
{
	const char* architectures[kMaxArchitectureCount];
	size_t count = get_architectures(architectures, kMaxArchitectureCount);
	return string_array_to_string_list(architectures,
		std::min(count, kMaxArchitectureCount), _architectures);
}
