/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Ma Jie, china.majie at gmail
 */
#ifndef POOR_MAN_SERVER_H
#define POOR_MAN_SERVER_H

#include <pthread.h>

#include <OS.h>
#include <SupportDefs.h>

#include "libhttpd/libhttpd.h"

#define POOR_MAN_BUF_SIZE 1048576ll

#ifdef __cplusplus
class PoorManServer{
public:
						PoorManServer(const char* webDir, int32 maxConns,
							bool listDir, const char* idxName);
	virtual             ~PoorManServer();

			status_t	Run();
			status_t	Stop();

			bool        IsRunning()const{return fIsRunning;}
			
			status_t    SetWebDir(const char* webDir);
			status_t    SetMaxConns(int32 count);
			status_t	SetListDir(bool listDir);
			status_t	SetIndexName(const char* idxName);

			pthread_rwlock_t* GetWebDirLock(){return &fWebDirLock;}
			pthread_rwlock_t* GetIndexNameLock(){return &fIndexNameLock;}
private:
			bool        fIsRunning;
			int32       fMaxConns;//Max Thread Count
			char*		fIndexName;//Index File Name
			
			thread_id	  fListenerTid;
			int32		  fCurConns;
			httpd_server* fHttpdServer;
			
			pthread_rwlock_t fWebDirLock;
			pthread_rwlock_t fIndexNameLock;
			
			PoorManServer(){}
			PoorManServer(PoorManServer& s){}
			PoorManServer& operator=(PoorManServer& s){return *this;}

	//two thread functions.
	static 	int32		_Listener(void* data);
	static  int32		_Worker(void* data);
	
			status_t   _HandleGet(httpd_conn* hc);
			status_t   _HandleHead(httpd_conn* hc);
			status_t   _HandlePost(httpd_conn* hc);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

pthread_rwlock_t* get_web_dir_lock();
pthread_rwlock_t* get_index_name_lock();

#ifdef __cplusplus
}
#endif

#endif //POOR_MAN_SERVER_H
