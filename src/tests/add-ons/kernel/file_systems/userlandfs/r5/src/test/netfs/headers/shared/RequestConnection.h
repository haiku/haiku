// RequestConnection.h

#ifndef NET_FS_REQUEST_CONNECTION_H
#define NET_FS_REQUEST_CONNECTION_H

#include <SupportDefs.h>

class Connection;
class Request;
class RequestHandler;

class RequestConnection {
public:
								RequestConnection(Connection* connection,
									RequestHandler* requestHandler,
									bool ownsRequestHandler = false);
								~RequestConnection();

			status_t			Init();

			void				Close();

			status_t			SendRequest(Request* request,
									Request** reply = NULL);
			status_t			SendRequest(Request* request,
									RequestHandler* replyHandler);
private:
			class DownStreamThread;
			friend class DownStreamThread;

private:
			bool				DownStreamChannelError(DownStreamThread* thread,
									status_t error);

			status_t			_SendRequest(Request* request, Request** reply,
									RequestHandler* replyHandler);

private:
			Connection*			fConnection;
			RequestHandler*		fRequestHandler;
			bool				fOwnsRequestHandler;
			DownStreamThread*	fThreads;
			int32				fThreadCount;
			vint32				fTerminationCount;
};

#endif	// NET_FS_REQUEST_CONNECTION_H
