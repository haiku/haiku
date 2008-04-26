#ifndef _KSOCKET_H

#define _KSOCKET_H

#include <socket.h>

int ksocket (int family, int type, int proto);
int kbind (int fd, const struct sockaddr *addr, int size);
int kconnect (int fd, const struct sockaddr *addr, int size);
int kgetsockname (int fd, struct sockaddr *addr, int *size);
int kgetpeername (int fd, struct sockaddr *addr, int *size);
int kaccept (int fd, struct sockaddr *addr, int *size);

ssize_t krecvfrom (int fd, void *buf, size_t size, int flags,
					struct sockaddr *from, int *fromlen);

ssize_t ksendto (int fd, const void *buf, size_t size, int flags,
					const struct sockaddr *to, int tolen);

ssize_t krecv (int fd, void *buf, size_t size, int flags);
ssize_t ksend (int fd, const void *buf, size_t size, int flags);

int klisten (int fd, int backlog);
int kclosesocket (int fd);

status_t ksocket_init ();
status_t ksocket_cleanup ();

void kmessage (const char *format, ...);

#endif
