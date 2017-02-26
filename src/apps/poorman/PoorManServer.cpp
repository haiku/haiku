/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Ma Jie, china.majie at gmail
 */

#include "PoorManServer.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> //for struct timeval
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <File.h>
#include <Debug.h>
#include <OS.h>
#include <String.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#include "PoorManApplication.h"
#include "PoorManLogger.h"
#include "PoorManWindow.h"
#include "libhttpd/libhttpd.h"


PoorManServer::PoorManServer(const char* webDir,
	int32 maxConns,	bool listDir,const char* idxName)
	:fIsRunning(false),
	 fMaxConns(maxConns),
	 fIndexName(new char[strlen(idxName) + 1]),
	 fCurConns(0)
{
	fHttpdServer = httpd_initialize(
		(char*)0,//hostname
		(httpd_sockaddr*)0,//sa4P
		(httpd_sockaddr*)0,//sa6P
		(unsigned short)80,//port
		(char*)0,//cgi pattern
		0,//cgi_limit
		(char *)"iso-8859-1",//charset
		(char *)"",//p3p
		-1,//max_age
		const_cast<char*>(webDir),//cwd
		1,//no_log
		(FILE*)0,//logfp
		0,//no_symlink_check
		0,//vhost
		0,//global_passwd
		(char*)0,//url_pattern
		(char*)0,//local_pattern
		0//no_empty_referers
	);
	
	strcpy(fIndexName, idxName);
	
	size_t cwdLen = strlen(fHttpdServer->cwd);
	if (fHttpdServer->cwd[cwdLen-1] == '/') {
		fHttpdServer->cwd[cwdLen-1] = '\0';
	}
	
	fHttpdServer->do_list_dir = (listDir ? 1 : 0);
	fHttpdServer->index_name = fIndexName;
	
	pthread_rwlock_init(&fWebDirLock, NULL);
	pthread_rwlock_init(&fIndexNameLock, NULL);
}


PoorManServer::~PoorManServer()
{
	Stop();
	httpd_terminate(fHttpdServer);
	delete[] fIndexName;
	pthread_rwlock_destroy(&fWebDirLock);
	pthread_rwlock_destroy(&fIndexNameLock);
}


status_t PoorManServer::Run()
{
	if (chdir(fHttpdServer->cwd) == -1) {
		poorman_log("no web directory, can't start up.\n", false, INADDR_NONE, RED);
		return B_ERROR;
	}
	
	httpd_sockaddr sa4;
	memset(&sa4, 0, sizeof(httpd_sockaddr));
	sa4.sa_in.sin_family = AF_INET;
	sa4.sa_in.sin_port = htons(80);
	sa4.sa_in.sin_addr.s_addr = htonl(INADDR_ANY);
	fHttpdServer->listen4_fd = httpd_initialize_listen_socket(&sa4);
	if (fHttpdServer->listen4_fd == -1)
		return B_ERROR;

	fListenerTid = spawn_thread(
		PoorManServer::_Listener,
		"www listener",
		B_NORMAL_PRIORITY,
		static_cast<void*>(this)
	);
	if (fListenerTid < B_OK) {
		poorman_log("can't create listener thread.\n", false, INADDR_NONE, RED);
		return B_ERROR;
	}
	fIsRunning = true;
	if (resume_thread(fListenerTid) != B_OK) {
		fIsRunning = false;
		return B_ERROR;
	}
	
	//our server is up and running
	return B_OK;
}


status_t PoorManServer::Stop()
{
	if (fIsRunning) {
		fIsRunning = false;
		httpd_unlisten(fHttpdServer);
	}
	return B_OK;
}


/*The Web Dir is not changed if an error occured.
 */
status_t PoorManServer::SetWebDir(const char* webDir)
{
	if (chdir(webDir) == -1) {
		//log it
		return B_ERROR;
	}

	char* tmp = strdup(webDir);
	if (tmp == NULL)
		return B_ERROR;

	if (pthread_rwlock_wrlock(&fWebDirLock) == 0) {
		free(fHttpdServer->cwd);
		fHttpdServer->cwd = tmp;
		if (tmp[strlen(tmp) - 1] == '/') {
			tmp[strlen(tmp) - 1] = '\0';
		}
		pthread_rwlock_unlock(&fWebDirLock);
	} else {
		free(tmp);
		return B_ERROR;
	}

	return B_OK;
}


status_t PoorManServer::SetMaxConns(int32 count)
{
	fMaxConns = count;
	return B_OK;
}


status_t PoorManServer::SetListDir(bool listDir)
{
	fHttpdServer->do_list_dir = (listDir ? 1 : 0);
	return B_OK;
}


status_t PoorManServer::SetIndexName(const char* idxName)
{
	size_t length = strlen(idxName);
	if (length > B_PATH_NAME_LENGTH + 1)
		return B_ERROR;
	
	char* tmp = new char[length + 1];
	if (tmp == NULL)
		return B_ERROR;
	
	strcpy(tmp, idxName);
	if (pthread_rwlock_wrlock(&fIndexNameLock) == 0) {
		delete[] fIndexName;
		fIndexName = tmp;
		fHttpdServer->index_name = fIndexName;
		pthread_rwlock_unlock(&fIndexNameLock);
	} else {
		delete[] tmp;
		return B_ERROR;
	}
	
	return B_OK;
}


int32 PoorManServer::_Listener(void* data)
{
	PRINT(("The listener thread is working.\n"));
	int retval;
	thread_id tid;
	httpd_conn* hc;
	PoorManServer* s = static_cast<PoorManServer*>(data);
	
	while (s->fIsRunning) {
		hc = new httpd_conn;
		hc->initialized = 0;
		PRINT(("calling httpd_get_conn()\n"));
		retval = //accept(), blocked here
			httpd_get_conn(s->fHttpdServer, s->fHttpdServer->listen4_fd, hc);
		switch (retval) {
			case GC_OK:
				break;
			case GC_FAIL:
				httpd_destroy_conn(hc);
				delete hc;
				s->fIsRunning = false;
				return -1;
			case GC_NO_MORE:
				//should not happen, since we have a blocking socket
				httpd_destroy_conn(hc);
				continue;
				break;
			default: 
				//shouldn't happen
				continue;
				break;
		}
		
		if (s->fCurConns > s->fMaxConns) {
			httpd_send_err(hc, 503,
				httpd_err503title, (char *)"", httpd_err503form, (char *)"");
			httpd_write_response(hc);
			continue;
		}
		
		tid = spawn_thread(
			PoorManServer::_Worker,
			"www connection",
			B_NORMAL_PRIORITY,
			static_cast<void*>(s)
		);
		if (tid < B_OK) {
			continue;
		}
		/*We don't check the return code here.
		 *As we can't kill a thread that doesn't receive the
		 *httpd_conn, we simply let it die itself.
		 */
		send_data(tid, 512, &hc, sizeof(httpd_conn*));
		atomic_add(&s->fCurConns, 1);
		resume_thread(tid);
	}//while
	return 0;
}


int32 PoorManServer::_Worker(void* data)
{
	static const struct timeval kTimeVal = {60, 0};
	PoorManServer* s = static_cast<PoorManServer*>(data);
	httpd_conn* hc;
	int retval;
	
	if (has_data(find_thread(NULL))) {
		thread_id sender;
		if (receive_data(&sender, &hc, sizeof(httpd_conn*)) != 512)
			goto cleanup;
	} else {
		// No need to go throught the whole cleanup, as we haven't open
		// nor allocated ht yet.
		atomic_add(&s->fCurConns, -1);
		return 0;
	}
	
	PRINT(("A worker thread starts to work.\n"));

	setsockopt(hc->conn_fd, SOL_SOCKET, SO_RCVTIMEO, &kTimeVal,
		sizeof(struct timeval));
	retval = recv(
		hc->conn_fd,
		&(hc->read_buf[hc->read_idx]),
		hc->read_size - hc->read_idx,
		0
	);
	if (retval < 0)
		goto cleanup;

	hc->read_idx += retval;
	switch(httpd_got_request(hc)) {
		case GR_GOT_REQUEST:
			break;
		case GR_BAD_REQUEST:
			httpd_send_err(hc, 400,
				httpd_err400title, (char *)"", httpd_err400form, (char *)"");
			httpd_write_response(hc);//fall through
		case GR_NO_REQUEST: //fall through
		default: //won't happen
			goto cleanup;
			break;
	}
	
	if (httpd_parse_request(hc) < 0) {
		httpd_write_response(hc);
		goto cleanup;
	}
	
	retval = httpd_start_request(hc, (struct timeval*)0);
	if (retval < 0) {
		httpd_write_response(hc);
		goto cleanup;
	}
	
	/*true means the connection is already handled
	 *by the directory index generator in httpd_start_request().
	 */
	if (hc->processed_directory_index == 1) {
		if (hc->method == METHOD_GET) {
			static_cast<PoorManApplication*>(be_app)->GetPoorManWindow()->SetHits(
				static_cast<PoorManApplication*>(be_app)->GetPoorManWindow()->GetHits() + 1
			);
		}
		goto cleanup;
	}
	
	switch (hc->method) {
		case METHOD_GET:
			s->_HandleGet(hc);
			break;
		case METHOD_HEAD:
			s->_HandleHead(hc);
			break;
		case METHOD_POST:
			s->_HandlePost(hc);
			break;
	}
	
cleanup: ;
	httpd_close_conn(hc, (struct timeval*)0);
	httpd_destroy_conn(hc);
	
	delete hc;
	atomic_add(&s->fCurConns, -1);
	return 0;
}


status_t PoorManServer::_HandleGet(httpd_conn* hc)
{
	PRINT(("HandleGet() called\n"));

	ssize_t bytesRead;
	uint8* buf;
	BString log;

	BFile file(hc->expnfilename, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return B_ERROR;

	buf = new uint8[POOR_MAN_BUF_SIZE];
	if (buf == NULL)
		return B_ERROR;
	
	static_cast<PoorManApplication*>(be_app)->GetPoorManWindow()->SetHits(
		static_cast<PoorManApplication*>(be_app)->
			GetPoorManWindow()->GetHits() + 1);
	
	log.SetTo("Sending file: ");
	if (pthread_rwlock_rdlock(&fWebDirLock) == 0) {
		log << hc->hs->cwd;
		pthread_rwlock_unlock(&fWebDirLock);
	}
	log << '/' << hc->expnfilename << '\n';
	poorman_log(log.String(), true, hc->client_addr.sa_in.sin_addr.s_addr);
	
	//send mime headers
	if (send(hc->conn_fd, hc->response, hc->responselen, 0) < 0) {
		delete [] buf;
		return B_ERROR;
	}
	
	file.Seek(hc->first_byte_index, SEEK_SET);
	while (true) {
		bytesRead = file.Read(buf, POOR_MAN_BUF_SIZE);
		if (bytesRead == 0)
			break;
		else if (bytesRead < 0) {
			delete [] buf;
			return B_ERROR;
		}
		if (send(hc->conn_fd, (void*)buf, bytesRead, 0) < 0) {
			log.SetTo("Error sending file: ");
			if (pthread_rwlock_rdlock(&fWebDirLock) == 0) {
				log << hc->hs->cwd;
				pthread_rwlock_unlock(&fWebDirLock);
			}
			log << '/' << hc->expnfilename << '\n';
			poorman_log(log.String(), true, hc->client_addr.sa_in.sin_addr.s_addr, RED);
			delete [] buf;
			return B_ERROR;
		}
	}
	
	delete [] buf;
	return B_OK;
}


status_t PoorManServer::_HandleHead(httpd_conn* hc)
{
	int retval = send(hc->conn_fd, hc->response, hc->responselen, 0);
	if (retval == -1)
		return B_ERROR;
	return B_OK;
}


status_t PoorManServer::_HandlePost(httpd_conn* hc)
{
	//not implemented
	return B_OK;
}


pthread_rwlock_t* get_web_dir_lock()
{
	return static_cast<PoorManApplication*>(be_app)->
		GetPoorManWindow()->GetServer()->GetWebDirLock();
}


pthread_rwlock_t* get_index_name_lock()
{
	return static_cast<PoorManApplication*>(be_app)->
		GetPoorManWindow()->GetServer()->GetIndexNameLock();
}
