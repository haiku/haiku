#include "ksocket_internal.h"
#include "ksocket.h"

#include <OS.h>
#include <errno.h>
#include <socket.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

struct ks_globals
{
	port_id daemon;
	
	sem_id port_pool_sem;
	port_id *port_pool;
	int32 port_pool_count;
	int32 port_pool_size;
	int32 refcount;
};

struct ks_globals *ksocket_globals=NULL;

static port_id acquire_port ();
static void release_port (port_id port);
static status_t send_call (int32 code, struct ks_param_header *param, size_t size, struct ks_reply_header **reply);
static int kmulti1 (int32 code, int fd, const struct sockaddr *addr, int size);
static int kmulti2 (int32 code, int fd, struct sockaddr *addr, int *size);

port_id 
acquire_port()
{
	port_id port;
	
	while (acquire_sem (ksocket_globals->port_pool_sem)==B_INTERRUPTED)
	{
	}
	
	if (ksocket_globals->port_pool_count==0)
	{
		port=create_port (1,"ksocket_port");
		set_port_owner (port,B_SYSTEM_TEAM);
		
		while (release_sem (ksocket_globals->port_pool_sem)==B_INTERRUPTED)
		{
		}
			
		return port;
	}

	port=ksocket_globals->port_pool[ksocket_globals->port_pool_count-1];
	ksocket_globals->port_pool_count--;
	
	while (release_sem (ksocket_globals->port_pool_sem)==B_INTERRUPTED)
	{
	}

	return port;
}

void 
release_port(port_id port)
{
	while (acquire_sem (ksocket_globals->port_pool_sem)==B_INTERRUPTED)
	{
	}

	if (ksocket_globals->port_pool_count+1>ksocket_globals->port_pool_size)
	{
		ksocket_globals->port_pool_size+=8;
		ksocket_globals->port_pool=(port_id *)realloc(ksocket_globals->port_pool,ksocket_globals->port_pool_size*sizeof(port_id));
	}

	ksocket_globals->port_pool[ksocket_globals->port_pool_count]=port;
	ksocket_globals->port_pool_count++;
	
	while (release_sem (ksocket_globals->port_pool_sem)==B_INTERRUPTED)
	{
	}	
}

status_t 
send_call(int32 code, struct ks_param_header *param, size_t size, struct ks_reply_header **reply)
{
	status_t result;
	ssize_t bytes;
	
	param->port=acquire_port();
	
	do
	{
		result=write_port (ksocket_globals->daemon,code,param,size);
	}
	while (result==B_INTERRUPTED);
	
	if (result<B_OK)	
	{
		release_port (param->port);
		return result;
	}
	
	do
	{
		bytes=port_buffer_size (param->port);
	}
	while (bytes==B_INTERRUPTED);
		
	if (bytes<B_OK)
	{
		release_port (param->port);
		return bytes;
	}

	*reply=(struct ks_reply_header *)malloc(bytes);
	
	do
	{
		int32 dummy;
		result=read_port (param->port,&dummy,*reply,bytes);
	}
	while (result==B_INTERRUPTED);
	
	if (result<B_OK)
	{
		free (*reply);
		release_port (param->port);
		return result;
	}
	
	release_port (param->port);
	return B_OK;
}


extern int ksocket (int family, int type, int proto)
{
	struct ks_socket_param param;
	struct ks_socket_reply *reply;
	status_t result;
		
	param.family=family;
	param.type=type;
	param.proto=proto;
	
	result=send_call(KS_SOCKET,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	
	return result;
}

int kmulti1 (int32 code, int fd, const struct sockaddr *addr, int size)
{
	size_t paramSize=sizeof(struct ks_bind_param)+size;
	struct ks_bind_param *param=(struct ks_bind_param *)malloc(paramSize);
	struct ks_bind_reply *reply;
	status_t result;

	param->fd=fd;
	param->size=size;
	memcpy (param->addr,addr,size);
	
	result=send_call(code,&param->header,paramSize,(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		free (param);
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	free (param);
	
	return result;	
}

int kmulti2 (int32 code, int fd, struct sockaddr *addr, int *size)
{
	struct ks_getsockname_param param;
	struct ks_getsockname_reply *reply;
	status_t result;
	
	param.fd=fd;
	param.size=*size;
	
	result=send_call (code,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	else
	{
		*size=reply->size;
		memcpy (addr,reply->addr,reply->size);
	}
	
	free (reply);
	
	return result;	
}

extern int kbind (int fd, const struct sockaddr *addr, int size)
{
	return kmulti1 (KS_BIND,fd,addr,size);
}

extern int kconnect (int fd, const struct sockaddr *addr, int size)
{
	return kmulti1 (KS_CONNECT,fd,addr,size);
}

extern int kgetsockname (int fd, struct sockaddr *addr, int *size)
{
	return kmulti2 (KS_GETSOCKNAME,fd,addr,size);
}

extern int kgetpeername (int fd, struct sockaddr *addr, int *size)
{
	return kmulti2 (KS_GETPEERNAME,fd,addr,size);
}

extern int kaccept (int fd, struct sockaddr *addr, int *size)
{
	return kmulti2 (KS_ACCEPT,fd,addr,size);
}

extern ssize_t krecvfrom (int fd, void *buf, size_t size, int flags,
					struct sockaddr *from, int *fromlen)
{
	struct ks_recvfrom_param param;
	struct ks_recvfrom_reply *reply;
	status_t result;
	
	param.fd=fd;
	param.size=size;
	param.flags=flags;
	param.fromlen=*fromlen;
	
	result=send_call (KS_RECVFROM,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	else
	{
		memcpy (buf,reply->data,result);		
		*fromlen=reply->fromlen;
		memcpy (from,reply->data+result,reply->fromlen);
	}
	
	free (reply);
	
	return result;	
}

extern ssize_t ksendto (int fd, const void *buf, size_t size, int flags,
					const struct sockaddr *to, int tolen)
{
	size_t paramSize=sizeof(struct ks_sendto_param)+size+tolen;
	struct ks_sendto_param *param=(struct ks_sendto_param *)malloc(paramSize);
	struct ks_sendto_reply *reply;
	status_t result;

	param->fd=fd;
	param->size=size;
	param->flags=flags;
	param->tolen=tolen;
	memcpy (param->data,buf,size);
	memcpy (param->data+size,to,tolen);
	
	result=send_call(KS_SENDTO,&param->header,paramSize,(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		free (param);
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	free (param);
	
	return result;
}

extern ssize_t ksend (int fd, const void *buf, size_t size, int flags)
{
	size_t paramSize=sizeof(struct ks_send_param)+size;
	struct ks_send_param *param=(struct ks_send_param *)malloc(paramSize);
	struct ks_send_reply *reply;
	status_t result;

	param->fd=fd;
	param->size=size;
	param->flags=flags;
	memcpy (param->data,buf,size);
	
	result=send_call(KS_SEND,&param->header,paramSize,(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		free (param);
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	free (param);
	
	return result;
}

extern ssize_t krecv (int fd, void *buf, size_t size, int flags)
{
	struct ks_recv_param param;
	struct ks_recv_reply *reply;
	status_t result;
	
	param.fd=fd;
	param.size=size;
	param.flags=flags;
	
	result=send_call (KS_RECV,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	else
		memcpy (buf,reply->data,result);		
	
	free (reply);
	
	return result;	
}

extern int klisten (int fd, int backlog)
{
	struct ks_listen_param param;
	struct ks_listen_reply *reply;
	status_t result;
		
	param.fd=fd;
	param.backlog=backlog;
	
	result=send_call(KS_LISTEN,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	
	return result;
}

extern int kclosesocket (int fd)
{
	struct ks_closesocket_param param;
	struct ks_closesocket_reply *reply;
	status_t result;
		
	param.fd=fd;
	
	result=send_call(KS_CLOSESOCKET,&param.header,sizeof(param),(struct ks_reply_header **)&reply);

	if (result<B_OK)	
	{
		errno=result;
		return -1;
	}
	
	result=reply->result;

	if (result<0)
		errno=reply->header.error;
	
	free (reply);
	
	return result;
}

extern status_t 
ksocket_init()
{
	if (ksocket_globals)
	{
		ksocket_globals->refcount++;
		
		return B_OK;
	}
		
	ksocket_globals=(struct ks_globals *)malloc(sizeof(struct ks_globals));
	
	ksocket_globals->refcount=1;
	ksocket_globals->daemon=find_port (KSOCKET_DAEMON_NAME);
	
	if (ksocket_globals->daemon<B_OK)
	{
		free (ksocket_globals);
		ksocket_globals=NULL;
		
		return B_ERROR;
	}

	ksocket_globals->port_pool_sem=create_sem(1,"port_pool_sem");
	
	if (ksocket_globals->port_pool_sem<B_OK)
	{
		delete_port (ksocket_globals->daemon);
		
		free (ksocket_globals);
		ksocket_globals=NULL;
		
		return B_ERROR;
	}
	
	set_sem_owner (ksocket_globals->port_pool_sem,B_SYSTEM_TEAM);
	
	ksocket_globals->port_pool=NULL;
	ksocket_globals->port_pool_count=0;
	ksocket_globals->port_pool_size=0;
		
	return B_OK;
}

extern status_t 
ksocket_cleanup()
{
	int32 i;

	ksocket_globals->refcount--;
	if (ksocket_globals->refcount>0)
		return B_OK;
		
	for (i=0;i<ksocket_globals->port_pool_count;i++)
		delete_port (ksocket_globals->port_pool[i]);
	
	free (ksocket_globals->port_pool);

	delete_sem (ksocket_globals->port_pool_sem);
	
	free (ksocket_globals);
	ksocket_globals=NULL;
	
	return B_OK;
}

extern void kmessage (const char *format, ...)
{
	status_t result;
	char msg[256];	

	va_list l;
	va_start (l,format);
	vsprintf (msg,format,l);
	va_end (l);
	
	do
	{
		result=write_port (ksocket_globals->daemon,KS_MESSAGE,msg,strlen(msg)+1);
	}
	while (result==B_INTERRUPTED);
}
