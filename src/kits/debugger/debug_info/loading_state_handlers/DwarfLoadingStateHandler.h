/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_LOADING_STATE_HANDLER
#define DWARF_LOADING_STATE_HANDLER


#include "ImageDebugLoadingStateHandler.h"


namespace BPackageKit {
	class BPackageVersion;
}


class BString;


class DwarfLoadingStateHandler : public ImageDebugLoadingStateHandler {
public:
								DwarfLoadingStateHandler();
	virtual						~DwarfLoadingStateHandler();

	virtual	bool				SupportsState(
									SpecificImageDebugInfoLoadingState* state);

	virtual	void				HandleState(
									SpecificImageDebugInfoLoadingState* state,
									UserInterface* interface);

private:
			status_t			_GetMatchingDebugInfoPackage(
									const BString& debugFileName,
									BString& _packageName);

			status_t			_GetResolvableName(const BString& debugFileName,
									BString& _resolvableName,
									BPackageKit::BPackageVersion&
										_resolvableVersion);
};


#endif	// DWARF_LOADING_STATE_HANDLER_H
