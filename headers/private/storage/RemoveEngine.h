/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _REMOVE_ENGINE_H
#define _REMOVE_ENGINE_H


#include <stdarg.h>

#include <EntryOperationEngineBase.h>


namespace BPrivate {


class BRemoveEngine : public BEntryOperationEngineBase {
public:
			class BController;

public:
								BRemoveEngine();
								~BRemoveEngine();

			BController*		Controller() const;
			void				SetController(BController* controller);

			status_t			RemoveEntry(const Entry& entry);

private:
			status_t			_RemoveEntry(const char* path);

			void				_NotifyErrorVarArgs(status_t error,
									const char* format, va_list args);
			status_t			_HandleEntryError(const char* path,
									status_t error, const char* format, ...);

private:
			BController*		fController;
};


class BRemoveEngine::BController {
public:
								BController();
	virtual						~BController();

	virtual	bool				EntryStarted(const char* path);
	virtual	bool				EntryFinished(const char* path, status_t error);

	virtual	void				ErrorOccurred(const char* message,
									status_t error);
};


} // namespace BPrivate


using ::BPrivate::BRemoveEngine;


#endif	// _REMOVE_ENGINE_H
