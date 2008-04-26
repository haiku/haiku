#ifndef _KSOCKET_INTERNAL_H

#define _KSOCKET_INTERNAL_H

#include <OS.h>

struct ks_param_header
{
	port_id port;
};

struct ks_reply_header
{
	int error;
};

struct ks_socket_param
{
	struct ks_param_header header;
	int family,type,proto;
};

struct ks_socket_reply
{
	struct ks_reply_header header;
	int result;
};

struct ks_bind_param
{
	struct ks_param_header header;
	int fd;
	int size;
	char addr[1];
};

struct ks_bind_reply
{
	struct ks_reply_header header;
	int result;
};

struct ks_getsockname_param
{
	struct ks_param_header header;
	int fd;
	int size;
};

struct ks_getsockname_reply
{
	struct ks_reply_header header;
	int result;
	int size;
	char addr[1];
};

struct ks_recvfrom_param
{
	struct ks_param_header header;
	int fd;
	size_t size;
	int flags;
	int fromlen;
};

struct ks_recvfrom_reply
{
	struct ks_reply_header header;
	ssize_t result;
	int fromlen;
	char data[1];
};

struct ks_sendto_param
{
	struct ks_param_header header;
	int fd;
	size_t size;
	int flags;
	int tolen;
	char data[1];
};

struct ks_sendto_reply
{
	struct ks_reply_header header;
	ssize_t result;
};

struct ks_recv_param
{
	struct ks_param_header header;
	int fd;
	size_t size;
	int flags;
};

struct ks_recv_reply
{
	struct ks_reply_header header;
	ssize_t result;
	char data[1];
};

struct ks_send_param
{
	struct ks_param_header header;
	int fd;
	size_t size;
	int flags;
	char data[1];
};

struct ks_send_reply
{
	struct ks_reply_header header;
	ssize_t result;
};

struct ks_listen_param
{
	struct ks_param_header header;
	int fd,backlog;
};

struct ks_listen_reply
{
	struct ks_reply_header header;
	int result;
};

struct ks_closesocket_param
{
	struct ks_param_header header;
	int fd;
};

struct ks_closesocket_reply
{
	struct ks_reply_header header;
	int result;
};

enum
{
	KS_SOCKET,
	KS_BIND,
	KS_GETSOCKNAME,
	KS_GETPEERNAME,
	KS_CONNECT,
	KS_ACCEPT,
	KS_RECVFROM,
	KS_SENDTO,
	KS_RECV,
	KS_SEND,
	KS_LISTEN,
	KS_CLOSESOCKET,
	KS_MESSAGE,
	KS_QUIT
};

#define KSOCKET_DAEMON_NAME "ksocket_daemon"
#define KSOCKETD_SIGNATURE "application/x-vnd.BareCode-ksocketd"

#endif
