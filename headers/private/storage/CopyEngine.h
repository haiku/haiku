/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _COPY_ENGINE_H
#define _COPY_ENGINE_H


#include <stdarg.h>

#include <EntryOperationEngineBase.h>


class BFile;
class BNode;


namespace BPrivate {


class BCopyEngine : public BEntryOperationEngineBase {
public:
			class BController;

			enum {
				COPY_RECURSIVELY			= 0x01,
				MERGE_EXISTING_DIRECTORIES	= 0x02,
				UNLINK_DESTINATION			= 0x04,
			};

public:
								BCopyEngine(uint32 flags = 0);
								~BCopyEngine();

			BController*		Controller() const;
			void				SetController(BController* controller);

			uint32				Flags() const;
			BCopyEngine&		SetFlags(uint32 flags);
			BCopyEngine&		AddFlags(uint32 flags);
			BCopyEngine&		RemoveFlags(uint32 flags);

			status_t			CopyEntry(const Entry& sourceEntry,
									const Entry& destEntry);

private:
			status_t			_CopyEntry(const char* sourcePath,
									const char* destPath);
			status_t			_CopyFileData(const char* sourcePath,
									BFile& source, const char* destPath,
									BFile& destination);
			status_t			_CopyAttributes(const char* sourcePath,
									BNode& source, const char* destPath,
									BNode& destination);

			void				_NotifyError(status_t error, const char* format,
									...);
			void				_NotifyErrorVarArgs(status_t error,
									const char* format, va_list args);
			status_t			_HandleEntryError(const char* path,
									status_t error, const char* format, ...);
			status_t			_HandleAttributeError(const char* path,
									const char* attribute, uint32 attributeType,
									status_t error, const char* format, ...);

private:
			BController*		fController;
			uint32				fFlags;
			char*				fBuffer;
			size_t				fBufferSize;
};


class BCopyEngine::BController {
public:
								BController();
	virtual						~BController();

	virtual	bool				EntryStarted(const char* path);
	virtual	bool				EntryFinished(const char* path, status_t error);

	virtual	bool				AttributeStarted(const char* path,
									const char* attribute,
									uint32 attributeType);
	virtual	bool				AttributeFinished(const char* path,
									const char* attribute,
									uint32 attributeType, status_t error);

	virtual	void				ErrorOccurred(const char* message,
									status_t error);
};


} // namespace BPrivate


using ::BPrivate::BCopyEngine;


#endif	// _COPY_ENGINE_H
