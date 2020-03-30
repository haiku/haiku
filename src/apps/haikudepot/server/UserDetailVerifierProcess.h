/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef USER_DETAIL_VERIFIER_PROCESS_H
#define USER_DETAIL_VERIFIER_PROCESS_H

#include <String.h>

#include "AbstractProcess.h"
#include "Model.h"


class UserDetailVerifierListener {
public:
	virtual	void				UserUsageConditionsNotLatest(
									const UserDetail& userDetail) = 0;
	virtual	void				UserCredentialsFailed() = 0;
};


/*!	This service has the purpose of querying the application server (HDS)
	for details of the authenticated user.  This will check that the user
	has the correct username / password and also that the user has agreed
	to the current terms and conditions.
 */

class UserDetailVerifierProcess : public AbstractProcess {
public:
								UserDetailVerifierProcess(
									Model* model,
									UserDetailVerifierListener* listener);
	virtual						~UserDetailVerifierProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

protected:
	virtual	status_t			RunInternal();

private:
			status_t			_TryFetchUserDetail(UserDetail& userDetail);
			bool				_ShouldVerify();

private:
			Model*				fModel;
			UserDetailVerifierListener*
								fListener;
};


#endif // USER_DETAIL_VERIFIER_PROCESS_H
