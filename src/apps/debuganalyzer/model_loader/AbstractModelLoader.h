/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_MODEL_LOADER_H
#define ABSTRACT_MODEL_LOADER_H

#include <Locker.h>
#include <Messenger.h>


class AbstractModelLoader {
public:
								AbstractModelLoader(const BMessenger& target,
									void* targetCookie);

protected:
	virtual						~AbstractModelLoader();

public:
	virtual	status_t			StartLoading();
	virtual	void				Abort(bool wait);
	virtual	void				Delete();

protected:
	virtual	status_t			PrepareForLoading();
	virtual	status_t			Load();
	virtual	void				FinishLoading(bool success);

			void				NotifyTarget(bool success);

private:
	static	status_t			_LoaderEntry(void* data);
			status_t			_Loader();

protected:
			BLocker				fLock;
			BMessenger			fTarget;
			void*				fTargetCookie;
			thread_id			fLoaderThread;
			bool				fLoading;
			bool				fAborted;
};


#endif	// ABSTRACT_MODEL_LOADER_H
