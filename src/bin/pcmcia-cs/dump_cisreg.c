/*======================================================================

    PCMCIA card configuration register dump

    dump_cisreg.c 1.31 2001/11/30 23:10:17

    The contents of this file are subject to the Mozilla Public
    License Version 1.1 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
    are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.

    Alternatively, the contents of this file may be used under the
    terms of the GNU General Public License version 2 (the "GPL"), in
    which case the provisions of the GPL are applicable instead of the
    above.  If you wish to allow the use of your version of this file
    only under the terms of the GPL and not to allow others to use
    your version of this file under the MPL, indicate your decision
    by deleting the provisions above and replace them with the notice
    and other provisions required by the GPL.  If you do not delete
    the provisions above, a recipient may use your version of this
    file under either the MPL or the GPL.
    
======================================================================*/
#if (defined(__BEOS__) || defined(__HAIKU__))
#include <OS.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

/*====================================================================*/
#if (!defined(__BEOS__) && !defined(__HAIKU__))
static int major = 0;

static int lookup_dev(char *name)
{
    FILE *f;
    int n;
    char s[32], t[32];
    
    f = fopen("/proc/devices", "r");
    if (f == NULL)
	return -1;
    while (fgets(s, 32, f) != NULL) {
	if (sscanf(s, "%d %s", &n, t) == 2)
	    if (strcmp(name, t) == 0)
		break;
    }
    fclose(f);
    if (strcmp(name, t) == 0)
	return n;
    else
	return -1;
}
#endif

/*====================================================================*/

static int open_sock(int sock)
{
#if (defined(__BEOS__) || defined(__HAIKU__))
    char fn[B_OS_NAME_LENGTH];
    sprintf(fn, "/dev/bus/pcmcia/sock/%d", sock);
    return open(fn, O_RDONLY);
#else
    static char *paths[] = {
	"/var/lib/pcmcia", "/var/run", "/dev", "/tmp", NULL
    };
    int fd;
    char **p, fn[64];
    dev_t dev = (major<<8) + sock;

    for (p = paths; *p; p++) {
	sprintf(fn, "%s/dc%d", *p, getpid());
	if (mknod(fn, (S_IFCHR|S_IREAD|S_IWRITE), dev) == 0) {
	    fd = open(fn, O_RDONLY);
	    unlink(fn);
	    if (fd >= 0)
		return fd;
	    if (errno == ENODEV) break;
	}
    }
    return -1;
#endif
} /* open_sock */

/*====================================================================*/

static int get_reg(int fd, int fn, off_t off)
{
    ds_ioctl_arg_t arg;
    int ret;

    arg.conf_reg.Function = fn;
    arg.conf_reg.Action = CS_READ;
    arg.conf_reg.Offset = off;
    ret = ioctl(fd, DS_ACCESS_CONFIGURATION_REGISTER, &arg);
    if (ret != 0) {
	printf("  read config register: %s\n\n", strerror(errno));
	return -1;
    }
    return arg.conf_reg.Value;
}

static int dump_option(int fd, int fn, int mfc)
{
    int v = get_reg(fd, fn, CISREG_COR);

    if (v == -1) return -1;
    printf("  Configuration option register = %#2.2x\n", v);
    printf("   ");
    if (v & COR_LEVEL_REQ) printf(" [level_req]");
    if (v & COR_SOFT_RESET) printf(" [soft_reset]");
    if (mfc) {
	if (v & COR_FUNC_ENA) printf(" [func_ena]");
	if (v & COR_ADDR_DECODE) printf(" [addr_decode]");
	if (v & COR_IREQ_ENA) printf(" [ireq_ena]");
	printf(" [index = %#2.2x]\n", v & COR_MFC_CONFIG_MASK);
    } else
	printf(" [index = %#2.2x]\n", v & COR_CONFIG_MASK);
    return 0;
}

static void dump_status(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_CCSR);
    
    printf("  Card configuration and status register = %#2.2x\n", v);
    printf("   ");
    if (v & CCSR_INTR_ACK) printf(" [intr_ack]");
    if (v & CCSR_INTR_PENDING) printf(" [intr_pending]");
    if (v & CCSR_POWER_DOWN) printf(" [power_down]");
    if (v & CCSR_AUDIO_ENA) printf(" [audio]");
    if (v & CCSR_IOIS8) printf(" [IOis8]");
    if (v & CCSR_SIGCHG_ENA) printf(" [sigchg]");
    if (v & CCSR_CHANGED) printf(" [changed]");
    printf("\n");
}

static void dump_pin(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_PRR);
    
    printf("  Pin replacement register = %#2.2x\n", v);
    printf("   ");
    if (v & PRR_WP_STATUS) printf(" [wp]");
    if (v & PRR_READY_STATUS) printf(" [ready]");
    if (v & PRR_BVD2_STATUS) printf(" [bvd2]");
    if (v & PRR_BVD1_STATUS) printf(" [bvd1]");
    if (v & PRR_WP_EVENT) printf(" [wp_event]");
    if (v & PRR_READY_EVENT) printf(" [ready_event]");
    if (v & PRR_BVD2_EVENT) printf(" [bvd2_event]");
    if (v & PRR_BVD1_EVENT) printf(" [bvd1_event]");
    printf("\n");
}

static void dump_copy(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_SCR);
    
    printf("  Socket and copy register = %#2.2x\n", v);
    printf("    [socket = %d] [copy = %d]\n",
	   v & SCR_SOCKET_NUM,
	   (v & SCR_COPY_NUM) >> 4);
}

static void dump_ext_status(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_ESR);
    printf("  Extended status register = %#2.2x\n", v);
    printf("   ");
    if (v & ESR_REQ_ATTN_ENA) printf(" [req_attn_ena]");
    if (v & ESR_REQ_ATTN) printf(" [req_attn]");
    printf("\n");
}

/*====================================================================*/

static void dump_all(int fd, int fn, int mfc, u_int mask)
{
    int addr;
    if (mask & PRESENT_OPTION) {
	if (dump_option(fd, fn, mfc) != 0)
	    return;
    }
    if (mask & PRESENT_STATUS)
	dump_status(fd, fn);
    if (mask & PRESENT_PIN_REPLACE)
	dump_pin(fd, fn);
    if (mask & PRESENT_COPY)
	dump_copy(fd, fn);
    if (mask & PRESENT_EXT_STATUS)
	dump_ext_status(fd, fn);
    if (mask & PRESENT_IOBASE_0) {
	addr = get_reg(fd, fn, CISREG_IOBASE_0);
	addr += get_reg(fd, fn, CISREG_IOBASE_1) << 8;
	printf("  IO base = 0x%04x\n", addr);
    }
    if (mask & PRESENT_IOSIZE)
	printf("  IO size = %d\n", get_reg(fd, fn, CISREG_IOSIZE));
    if (mask == 0)
	printf("  no config registers\n\n");
    else
	printf("\n");
}

/*====================================================================*/

#define MAX_SOCKS 8

int main(int argc, char *argv[])
{
    int i, j, nfn, fd, ret;
    u_int mask;
    ds_ioctl_arg_t arg;

#if (!defined(__BEOS__) && !defined(__HAIKU__))
    major = lookup_dev("pcmcia");
    if (major < 0) {
	fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	exit(EXIT_FAILURE);
    }
#endif
    
    for (i = 0; i < MAX_SOCKS; i++) {
	fd = open_sock(i);
	if (fd < 0) break;
	
	arg.tuple.TupleDataMax = sizeof(arg.tuple_parse.data);
	arg.tuple.Attributes = TUPLE_RETURN_COMMON;
	arg.tuple.DesiredTuple = CISTPL_LONGLINK_MFC;
	arg.tuple.TupleOffset = 0;
	if (ioctl(fd, DS_GET_FIRST_TUPLE, &arg) == 0) {
	    ioctl(fd, DS_GET_TUPLE_DATA, &arg);
	    ioctl(fd, DS_PARSE_TUPLE, &arg);
	    nfn = arg.tuple_parse.parse.longlink_mfc.nfn;
	} else {
	    nfn = 1;
	    arg.tuple.DesiredTuple = CISTPL_DEVICE;
	    ret = ioctl(fd, DS_GET_FIRST_TUPLE, &arg);
	    if (ret != 0) {
		if (errno != ENODEV) perror("ioctl()");
		continue;
	    }
	}

	arg.tuple.DesiredTuple = CISTPL_CONFIG;
	
	for (j = 0; j < nfn; j++) {
	    printf("Socket %d function %d:\n", i, j);
	    if (ioctl(fd, DS_GET_NEXT_TUPLE, &arg) != 0) {
		printf("  no config tuple: %s\n\n", strerror(errno));
		continue;
	    }
	    ioctl(fd, DS_GET_TUPLE_DATA, &arg);
	    ioctl(fd, DS_PARSE_TUPLE, &arg);
	    printf("  Config register base = %#4.4x, mask = %#4.4x\n",
		   arg.tuple_parse.parse.config.base,
		   arg.tuple_parse.parse.config.rmask[0]);
	    mask = arg.tuple_parse.parse.config.rmask[0];
	    dump_all(fd, j, (nfn > 1), mask);
	}
    
    }
    return 0;
}
