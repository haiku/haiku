/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_RUNTIME_INFORMATION_VERIFIER_PROCESS_H
#define SERVER_RUNTIME_INFORMATION_VERIFIER_PROCESS_H


#include "AbstractProcess.h"
#include "Model.h"


class ServerRuntimeInformation {
public:
								ServerRuntimeInformation();
	virtual						~ServerRuntimeInformation();

			int64				CurrentTimestamp() const;
			void				SetCurrentTimestamp(int64 value);

			bool				AllowsNicknamePasswordAuthentication() const;
			void				SetAllowsNicknamePasswordAuthentication(bool value);

private:
	int64 fCurrentTimestamp;
	bool fAllowsNicknamePasswordAuthentication;
};


/*!	This process will pull down the server runtime information. This includes
 *	if the server supports username + password authentication and the
 *	current timestamp that the server has.
 */
class ServerRuntimeInformationVerifierProcess : public AbstractProcess {
public:
								ServerRuntimeInformationVerifierProcess(Model* model);
	virtual						~ServerRuntimeInformationVerifierProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

protected:
	virtual	status_t			RunInternal();

private:
			bool				_ShouldVerify();
			status_t			_TryFetchServerRuntimeInformation(
									ServerRuntimeInformation& serverInformation);
			void				_CheckAndNotifyExcessCurrentTimestampDelta(
									int64 serverCurrentTimestamp);

private:
			Model*				fModel;
};


#endif // SERVER_RUNTIME_INFORMATION_VERIFIER_PROCESS_H
