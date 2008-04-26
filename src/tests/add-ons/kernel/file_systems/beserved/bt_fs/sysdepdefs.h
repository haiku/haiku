#ifndef _SYSDEPDEFS_H_
#define _SYSDEPDEFS_H_

#ifdef BONE_VERSION

#include <sys/socket_module.h>

//extern struct bone_socket_info *bone_module;

#define closesocket(s)				close(s)
#define kclosesocket(s)				close(s)
#define ksend(s, buf, len, flg)		bone_module->send(s, buf, len, flg)
#define krecv(s, buf, len, flg)		bone_module->recv(s, buf, len, flg)
#define ksocket(fam, type, prot)	bone_module->socket(fam, type, prot)
#define kconnect(s, addr, len)		bone_module->connect(s, addr, len)
#define ksetsockopt(s, type, opt, flag, len)	bone_module->setsockopt(s, type, opt, flag, len)
#endif

#endif
