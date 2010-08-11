/*
 *
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Many parts
 *
 * Copyright 2001 The Aerospace Corporation.
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 * Distributed under the terms of the 2-clause BSD license.
 *
 * Authors:
 *		Alex Botero-Lowry, alex.boterolowry@gmail.com
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/sockio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_haiku.h>


extern const char* __progname;


static const char*
get_string(const char* val, const char* sep, u_int8_t* buf, int* lenp)
{
	int len;
	int hexstr;
	u_int8_t* p;

	len = *lenp;
	p = buf;
	hexstr = (val[0] == '0' && tolower((u_char) val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char) val[0])) {
				printf("%s: bad hexadecimal digits", __func__);
				return NULL;
			}
			if (!isxdigit((u_char) val[1])) {
				printf("%s: odd count hexadecimal digits", __func__);
				return NULL;
			}
		}
		if (p >= buf + len) {
			if (hexstr)
				printf("%s: hexadecimal digits too long", __func__);
			else
				printf("%s: string too long", __func__);
			return NULL;
		}
		if (hexstr) {
#define tohex(x) (isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char) val[0]) << 4)
				| tohex((u_char) val[1]);
#undef tohex
			val += 2;
		} else
			*p++ = *val++;
	}
	len = p - buf;
	/* The string "-" is treated as the empty string. */
	if (!hexstr && len == 1 && buf[0] == '-') {
		len = 0;
		memset(buf, 0, *lenp);
	} else if (len < *lenp)
		memset(p, 0, *lenp - len);

	*lenp = len;
	return val;
}


static void
set80211(int s, const char* dev, int type, int val, int len, void* data)
{
	struct ieee80211req ireq;

	(void)memset(&ireq, 0, sizeof(ireq));
	(void)strncpy(ireq.i_name, dev, sizeof(ireq.i_name));
	ireq.i_type = type;
	ireq.i_val = val;
	ireq.i_len = len;
	ireq.i_data = data;
	if (ioctl(s, SIOCS80211, &ireq, sizeof(struct ieee80211req)) < 0) {
		fprintf(stderr, "%s: error in handling SIOCS80211 (type %d): %s\n",
			__progname, type, strerror(errno));
	}
}


static void
set80211ssid(const char* dev, const char* val, int s)
{
	int ssid;
	int len;
	u_int8_t data[IEEE80211_NWID_LEN];

	ssid = 0;
	len = strlen(val);
	if (len > 2 && isdigit((int)val[0]) && val[1] == ':') {
		ssid = atoi(val) - 1;
		val += 2;
	}
	bzero(data, sizeof(data));
	len = sizeof(data);
	if (get_string(val, NULL, data, &len) == NULL)
		exit(1);

	set80211(s, dev, IEEE80211_IOC_SSID, ssid, len, data);
}


static void
set80211nwkey(const char* dev, const char* val, int s)
{
	int txkey;
	int i;
	int len;
	u_int8_t data[IEEE80211_KEYBUF_SIZE];

	set80211(s, dev, IEEE80211_IOC_WEP, IEEE80211_WEP_ON, 0, NULL);

	if (isdigit((int)val[0]) && val[1] == ':') {
		txkey = val[0] - '0' - 1;
		val += 2;

		for (i = 0; i < 4; i++) {
			bzero(data, sizeof(data));
			len = sizeof(data);
			val = get_string(val, ",", data, &len);
			if (val == NULL)
				exit(1);

			set80211(s, dev, IEEE80211_IOC_WEPKEY, i, len, data);
		}
	} else {
		bzero(data, sizeof(data));
		len = sizeof(data);
		get_string(val, NULL, data, &len);
		txkey = 0;

		set80211(s, dev, IEEE80211_IOC_WEPKEY, 0, len, data);

		bzero(data, sizeof(data));
		for (i = 1; i < 4; i++)
			set80211(s, dev, IEEE80211_IOC_WEPKEY, i, 0, data);
	}

	set80211(s, dev, IEEE80211_IOC_WEPTXKEY, txkey, 0, NULL);
}


static int
get80211val(int s, const char* dev, int type, int* val)
{
	struct ieee80211req ireq;

	(void) memset(&ireq, 0, sizeof(ireq));
	(void) strncpy(ireq.i_name, dev, sizeof(ireq.i_name));
	ireq.i_type = type;
	if (ioctl(s, SIOCG80211, &ireq) < 0)
		return -1;
	*val = ireq.i_val;
	return 0;
}


static int
getid(int s, const char* dev, int ix, void* data, size_t len, int* plen,
	int mesh)
{
	struct ieee80211req ireq;

	(void)memset(&ireq, 0, sizeof(ireq));
	(void)strncpy(ireq.i_name, dev, sizeof(ireq.i_name));
	ireq.i_type = (!mesh) ? IEEE80211_IOC_SSID : IEEE80211_IOC_MESH_ID;
	ireq.i_val = ix;
	ireq.i_data = data;
	ireq.i_len = len;
	if (ioctl(s, SIOCG80211, &ireq) < 0)
		return -1;
	*plen = ireq.i_len;
	return 0;
}


static void
print_string(const u_int8_t* buf, int len)
{
	int i;
	int hasspc;

	i = 0;
	hasspc = 0;
	for (; i < len; i++) {
		if (!isprint(buf[i]) && buf[i] != '\0')
			break;
		if (isspace(buf[i]))
			hasspc++;
	}
	if (i == len) {
		if (hasspc || len == 0 || buf[0] == '\0')
			printf("\"%.*s\"", len, buf);
		else
			printf("%.*s", len, buf);
	} else {
		printf("0x");
		for (i = 0; i < len; i++)
			printf("%02x", buf[i]);
	}
}


static void
show_status(const char* dev, int s)
{
	int len;
	int i;
	int num;
	uint8_t data[32];

	if (getid(s, dev, -1, data, sizeof(data), &len, 0) < 0) {
		fprintf(stderr, "error: not a wifi device\n");
		exit(1);
	}

	if (get80211val(s, dev, IEEE80211_IOC_NUMSSIDS, &num) < 0)
		num = 0;
	printf("ssid ");
	if (num > 1) {
		for (i = 0; i < num; i++) {
			if (getid(s, dev, i, data, sizeof(data), &len, 0) >= 0
				&& len > 0) {
				printf(" %d:", i + 1);
				print_string(data, len);
			}
		}
	} else
		print_string(data, len);

	printf("\n");
}


static void
usage()
{
	fprintf(stderr, "usage: setwep device_path [ssid] [key]\n");
	exit(1);
}


int
main(int argc, char** argv)
{
	int s = socket(AF_INET, SOCK_DGRAM, 0);

	if (argc < 2)
		usage();

	if (argc == 2)
		show_status(argv[1], s);

	if (argc > 3)
		set80211ssid(argv[1], argv[2], s);

	if (argc == 4)
		set80211nwkey(argv[1], argv[3], s);

	return 0;
}
