/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__CONTEXT_H_
#define _HAIKU__PACKAGE__CONTEXT_H_


#include <package/TempEntryManager.h>


namespace Haiku {

namespace Package {


class JobStateListener;


class Context {
public:
								Context();
								~Context();

			TempEntryManager&	GetTempEntryManager() const;

			JobStateListener*	DefaultJobStateListener() const;
			void				SetDefaultJobStateListener(
									JobStateListener* listener);

private:
	mutable	TempEntryManager	fTempEntryManager;
			JobStateListener*	fDefaultJobStateListener;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__CONTEXT_H_
