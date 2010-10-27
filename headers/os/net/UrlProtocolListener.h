/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_LISTENER_H_
#define _B_URL_PROTOCOL_LISTENER_H_


#include <stddef.h>
#include <cstdlib>

class BUrlProtocol;


enum BUrlProtocolDebugMessage {
	B_URL_PROTOCOL_DEBUG_TEXT,
	B_URL_PROTOCOL_DEBUG_ERROR,
	B_URL_PROTOCOL_DEBUG_HEADER_IN,
	B_URL_PROTOCOL_DEBUG_HEADER_OUT,
	B_URL_PROTOCOL_DEBUG_TRANSFER_IN,
	B_URL_PROTOCOL_DEBUG_TRANSFER_OUT
};


class BUrlProtocolListener {
public:
	/*
		ConnectionOpened()
		Frequency:	Once
		
		 Called when the socket is opened.
	*/
	virtual	void				ConnectionOpened(BUrlProtocol* caller);
	
	/*
		HostnameResolved(ip)
		Frequency:	Once
		Parameters:	ip		 String representing the IP address of the resource
							host.
		
		 Called when the final IP is discovered
	*/
	virtual void				HostnameResolved(BUrlProtocol* caller,
									const char* ip);
									
	/*
		ReponseStarted()
		Frequency:	Once
		
		 Called when the request has been emitted and the server begins to
		reply. Typically when the HTTP status code is received.
	*/
	virtual void				ResponseStarted(BUrlProtocol* caller);
	
	/*
		HeadersReceived()
		Frequency:	Once
		
		 Called when all the server response metadata (such as headers) have
		been read and parsed.
	*/
	virtual void				HeadersReceived(BUrlProtocol* caller);
	
	/*
		DataReceived(data, size)
		Frequency:	Zero or more
		Parameters:	data	 Pointer to the data block in memory
					size	 Size of the data block 
		
		 Called each time a full block of data is received.
	*/
	virtual void				DataReceived(BUrlProtocol* caller,
									const char* data, ssize_t size);
									
	/*
		DownloadProgress(bytesReceived, bytesTotal)
		Frequency:	Once or more
		Parameters:	bytesReceived	Number of data bytes received
					bytesTotal		Total number of data bytes expected
					
		 Called each time a data block is received.
	*/
	virtual	void				DownloadProgress(BUrlProtocol* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
									
	/*
		UploadProgress(bytesSent, bytesTotal)
		Frequency:	Once or more
		Parameters:	bytesSent		Number of data bytes sent
					bytesTotal		Total number of data bytes expected
					
		 Called each time a data block is emitted.
	*/
	virtual void				UploadProgress(BUrlProtocol* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
																	
	/*
		RequestCompleted(success)
		Frequency:	Once
		Parameters:	success			true if the resource have been successfully
									false if not
					
		 Called once the request is complete.
	*/
	virtual void				RequestCompleted(BUrlProtocol* caller, 
									bool success);
									
	/*
		DebugMessage(type, text)
		Frequency:	zero or more
		Parameters:	type	Type of the verbose message (see BUrlProtocolDebug)
		
		 Called each time a debug message is emitted		
	*/
	virtual void				DebugMessage(BUrlProtocol* caller,
									BUrlProtocolDebugMessage type,
									const char* text);
};

#endif // _B_URL_PROTOCOL_LISTENER_H_
