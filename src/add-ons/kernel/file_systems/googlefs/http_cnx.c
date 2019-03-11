/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ksocket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "http_cnx.h"

//#define TESTME

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#define printf dprintf
#undef TESTME
#endif

#define DBG "googlefs: http_cnx: "

#define HTTPVER "1.0"

//#define GOOGLEFS_UA "GoogleFS"
//Mozilla/3.0 (compatible; NetPositive/2.2.2; BeOS)
//Mozilla/5.0 (BeOS; U; BeOS BePC; en-US; rv:1.8.1.18) Gecko/20081114 BonEcho/2.0.0.18
#ifdef __HAIKU__
#define GOOGLEFS_UA "Mozilla/5.0 (compatible; GoogleFS/0.1; Haiku)"
#else
#define GOOGLEFS_UA "Mozilla/5.0 (compatible; GoogleFS/0.1; BeOS)"
#endif

#ifdef TESTME
#define BUFSZ (128*1024)
#endif

KSOCKET_MODULE_DECL;

status_t http_init(void)
{
	return ksocket_init();
}

status_t http_uninit(void)
{
	return ksocket_cleanup();
}

status_t http_create(struct http_cnx **cnx)
{
	struct http_cnx *c;
	int err;
	c = malloc(sizeof(struct http_cnx));
	if (!c)
		return ENOMEM;
	memset(c, 0, sizeof(struct http_cnx));
	err = c->sock = ksocket(AF_INET, SOCK_STREAM, 0);
	if (err < 0) {
		free(c);
		return errno;
	}
	*cnx = c;
	return B_OK;
}

status_t http_delete(struct http_cnx *cnx)
{
	if (!cnx)
		return EINVAL;
	if ((unsigned long)cnx < 0x40) {
		dprintf("http: WARNING: cnx ptr = %p\n", cnx);
		return B_OK;
	}	
	if (cnx->sock >= 0) {
		kclosesocket(cnx->sock);
		cnx->sock = -1;
	}
	if (cnx->buffer)
		free(cnx->buffer);
	cnx->buffer = (char *)0xaa55aa55;//XXX
	free(cnx);
	return B_OK;
}

status_t http_connect(struct http_cnx *cnx, struct sockaddr_in *sin)
{
	int err;
	uint32 ip;
	uint16 port;
	if (!sin) {
		dprintf("http_connect(, NULL)!!\n");
		return EINVAL;
	}
	ip = sin->sin_addr.s_addr;
	port = sin->sin_port;
	dprintf("http_connect(, %ld.%ld.%ld.%ld:%d), sock = %d\n", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, ntohs(port), cnx->sock);
	err = kconnect(cnx->sock, (struct sockaddr *)sin, sin->sin_len);
	cnx->err = 0;
	if (err == -1)
		return errno;
	return err;
}

status_t http_close(struct http_cnx *cnx)
{
	int err;
	if (!cnx)
		return EINVAL;
	if (cnx->sock < 0)
		return 0;
	err = kclosesocket(cnx->sock);
	cnx->sock = -1;
	return err;
}

status_t http_get(struct http_cnx *cnx, const char *url)
{
	char *req, *p;
	int reqlen;
	int len;
	int err;
	long headerslen = 0;
	long contentlen = 0;
	if (strlen(url) > 4096 - 128)
		return EINVAL;
	req = malloc(4096+1);
	if (!req)
		return B_NO_MEMORY;
	req[4096] = '\0';
	/* no snprintf in kernel :( */
	sprintf(req, "GET %s HTTP/"HTTPVER"\r\nUser-Agent: " GOOGLEFS_UA "\r\nAccept: */*\r\n\r\n", url);
	reqlen = strlen(req);
	err = len = write(cnx->sock, req, reqlen);
	if (len < 1)
		goto err0;
	reqlen = 4096;
	err = len = read(cnx->sock, req, reqlen);
	printf("read(sock) = %d\n", len);
	if (err < 0)
		goto err0;
	//write(1, req, len);
	{
		int fd;
		// debug output
		fd = open("/tmp/google.html_", O_CREAT|O_TRUNC|O_RDWR, 0644);
		write(fd, req, len);
		close(fd);
	}

	err = EINVAL;
	if (len < 10)
		goto err0;
	if (!strstr(req, "HTTP/1.0 200"))
		goto err0;

	err = B_NO_MEMORY;
	if (!strstr(req, "\r\n\r\n")) {
		if (!strstr(req, "\n\n")) /* shouldn't happen */
			goto err0;
		headerslen = strstr(req, "\n\n") - req + 2;
		req[headerslen-1] = '\0';
		req[headerslen-2] = '\0';
	} else {
		headerslen = strstr(req, "\r\n\r\n") - req + 4;
		req[headerslen-3] = '\0';
		req[headerslen-4] = '\0';
	}
	if (headerslen < 2 || headerslen > 4095)
		goto err0;
	p = strstr(req, "HTTP/1.");
	if (!p)
		goto err0;
	p += strlen("HTTP/1.");
	if (strlen(p) < 5)
		goto err0;
	p += strlen("1 ");
	cnx->err = strtol(p, &p, 10);
	if (cnx->err < 200 || cnx->err > 299)
		goto err0;
	printf("REQ: %ld\n", cnx->err);
	contentlen = len - headerslen;
//	if (!strstr(req, "\n\n") && !strstr(req, "\r\n\r\n"))
//		goto err0;
	while (len > 0) {
		long left = reqlen - headerslen - contentlen;
		if (left < 128) {
			reqlen += 4096;
			p = realloc(req, reqlen);
			left += 4096;
			if (!p)
				goto err0;
			req = p;
		}
		err = len = read(cnx->sock, req + headerslen + contentlen, left);
		if (len < 0)
			goto err0;
		contentlen += len;
	}
	cnx->buffer = req;
	cnx->headers = req;
	cnx->headerslen = headerslen;
	cnx->data = req + headerslen;
	cnx->datalen = contentlen;
	cnx->data[contentlen] = '\0'; /* we have at least 128 bytes allocated ahead */
	return B_OK;
err0:
	if (err > -1)
		err = -1;
	free(req);
	return err;
}













#ifdef TESTME
//#define TESTURL "/"
//#define TESTURL "http://www.google.com/search?as_q=google+api+&num=50&hl=en&ie=ISO-8859-1&btnG=Google+Search&as_epq=frequently+asked&as_oq=help&as_eq=plop&lr=lang_en&as_ft=i&as_filetype=&as_qdr=m3&as_nlo=&as_nhi=&as_occt=any&as_dt=i&as_sitesearch="
#define TESTURL "http://www.google.com/search?hl=en&ie=UTF-8&num=50&q=beos"
int main(int argc, char **argv)
{
	struct sockaddr_in sin;
	struct http_cnx *cnx;
	size_t len;
	char *p;
	int err;
	
	http_init();
	err = http_create(&cnx);
	printf("error 0x%08lx\n", err);
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inet_aton("66.102.11.99", &sin.sin_addr);
	sin.sin_port = htons(80);
	err = http_connect(cnx, &sin);
	printf("error 0x%08lx\n", err);
	err = http_get(cnx, TESTURL);
	printf("error 0x%08lx\n", err);
	if (err < 0)
		return 1;
	printf("HEADERS %d:%s\n", cnx->headerslen, cnx->headers);
	printf("DATA: %d:%s\n", cnx->datalen, cnx->data);
	http_close(cnx);
	http_delete(cnx);
	return 0;
}
#endif
