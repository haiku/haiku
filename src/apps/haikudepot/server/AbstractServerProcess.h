/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef ABSTRACT_SERVER_PROCESS_H
#define ABSTRACT_SERVER_PROCESS_H

#include <HttpRequest.h>
#include <Json.h>
#include <String.h>
#include <Url.h>

#include "StandardMetaData.h"
#include "Stoppable.h"


typedef enum process_options {
	SERVER_PROCESS_NO_NETWORKING	= 1 << 0,
	SERVER_PROCESS_PREFER_CACHE		= 1 << 1,
	SERVER_PROCESS_DROP_CACHE		= 1 << 2
} process_options;


typedef enum process_state {
	SERVER_PROCESS_INITIAL			= 1,
	SERVER_PROCESS_RUNNING			= 2,
	SERVER_PROCESS_COMPLETE			= 3
} process_state;


/*! Clients are able to subclass from this 'interface' in order to accept
    call-backs when a process has exited; either through success or through
    failure.
 */

class AbstractServerProcessListener {
public:
	virtual	void				ServerProcessExited() = 0;
};


class AbstractServerProcess : public Stoppable {
public:
								AbstractServerProcess(
									AbstractServerProcessListener* listener,
									uint32 options);
	virtual						~AbstractServerProcess();

	virtual	const char*				Name() = 0;
			status_t			Run();
			status_t			Stop();
			status_t			ErrorStatus();
			bool				IsRunning();
			bool				WasStopped();

protected:
	virtual status_t			RunInternal() = 0;
	virtual status_t			StopInternal();

	virtual	void				GetStandardMetaDataPath(
									BPath& path) const = 0;
	virtual	void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const = 0;

			status_t			IfModifiedSinceHeaderValue(
									BString& headerValue) const;
			status_t			IfModifiedSinceHeaderValue(
									BString& headerValue,
									const BPath& metaDataPath,
									const BString& jsonPath) const;

			status_t			PopulateMetaData(
									StandardMetaData& metaData,
									const BPath& path,
									const BString& jsonPath) const;

			status_t			ParseJsonFromFileWithListener(
									BJsonEventListener *listener,
									const BPath& path) const;

			status_t			DownloadToLocalFileAtomically(
									const BPath& targetFilePath,
									const BUrl& url);

			status_t			DeleteLocalFile(const BPath& filePath);
			status_t			MoveDamagedFileAside(const BPath& filePath);

			bool				HasOption(uint32 flag);
			bool				ShouldAttemptNetworkDownload(
									bool hasDataAlready);

	static	bool				IsSuccess(status_t e);

private:
			BLocker				fLock;
			AbstractServerProcessListener*
								fListener;
			bool				fWasStopped;
			process_state		fProcessState;
			status_t			fErrorStatus;
			uint32				fOptions;

			BHttpRequest*		fRequest;

			process_state		ProcessState();
			void				SetErrorStatus(status_t value);
			void				SetProcessState(process_state value);

			status_t			DownloadToLocalFile(
									const BPath& targetFilePath,
									const BUrl& url,
									uint32 redirects, uint32 failures);

			bool				LooksLikeGzip(const char *pathStr) const;

};

#endif // ABSTRACT_SERVER_PROCESS_H
