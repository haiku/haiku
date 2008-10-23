/*======================================================================

    PC Card CIS dump utility

    dump_cis.c 1.63 2001/11/30 23:10:17

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
#include <pcmcia/ds.h>

static int verbose = 0;
static char indent[10] = "  ";

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

static void print_tuple(tuple_parse_t *tup)
{
    int i;
    printf("%soffset 0x%2.2x, tuple 0x%2.2x, link 0x%2.2x\n",
	   indent, tup->tuple.CISOffset, tup->tuple.TupleCode,
	   tup->tuple.TupleLink);
    for (i = 0; i < tup->tuple.TupleDataLen; i++) {
	if ((i % 16) == 0) printf("%s  ", indent);
	printf("%2.2x ", (u_char)tup->data[i]);
	if ((i % 16) == 15) putchar('\n');
    }
    if ((i % 16) != 0) putchar('\n');
}

/*====================================================================*/

static void print_funcid(cistpl_funcid_t *fn)
{
    printf("%sfuncid ", indent);
    switch (fn->func) {
    case CISTPL_FUNCID_MULTI:
	printf("multi_function"); break;
    case CISTPL_FUNCID_MEMORY:
	printf("memory_card"); break;
    case CISTPL_FUNCID_SERIAL:
	printf("serial_port"); break;
    case CISTPL_FUNCID_PARALLEL:
	printf("parallel_port"); break;
    case CISTPL_FUNCID_FIXED:
	printf("fixed_disk"); break;
    case CISTPL_FUNCID_VIDEO:
	printf("video_adapter"); break;
    case CISTPL_FUNCID_NETWORK:
	printf("network_adapter"); break;
    case CISTPL_FUNCID_AIMS:
	printf("aims_card"); break;
    case CISTPL_FUNCID_SCSI:
	printf("scsi_adapter"); break;
    default:
	printf("unknown"); break;
    }
    if (fn->sysinit & CISTPL_SYSINIT_POST)
	printf(" [post]");
    if (fn->sysinit & CISTPL_SYSINIT_ROM)
	printf(" [rom]");
    putchar('\n');
}

/*====================================================================*/

static void print_size(u_int size)
{
    if (size < 1024)
	printf("%ub", size);
    else if (size < 1024*1024)
	printf("%ukb", size/1024);
    else
	printf("%umb", size/(1024*1024));
}

static void print_unit(u_int v, char *unit, char tag)
{
    int n;
    for (n = 0; (v % 1000) == 0; n++) v /= 1000;
    printf("%u", v);
    if (n < strlen(unit)) putchar(unit[n]);
    putchar(tag);
}

static void print_time(u_int tm, u_long scale)
{
    print_unit(tm * scale, "num", 's');
}

static void print_volt(u_int vi)
{
    print_unit(vi * 10, "um", 'V');
}
    
static void print_current(u_int ii)
{
    print_unit(ii / 10, "um", 'A');
}

static void print_speed(u_int b)
{
    if (b < 1000)
	printf("%u bits/sec", b);
    else if (b < 1000000)
	printf("%u kb/sec", b/1000);
    else
	printf("%u mb/sec", b/1000000);
}

/*====================================================================*/

static const char *dtype[] = {
    "NULL", "ROM", "OTPROM", "EPROM", "EEPROM", "FLASH", "SRAM",
    "DRAM", "rsvd", "rsvd", "rsvd", "rsvd", "rsvd", "fn_specific",
    "extended", "rsvd"
};

static void print_device(cistpl_device_t *dev)
{
    int i;
    for (i = 0; i < dev->ndev; i++) {
	printf("%s  %s ", indent, dtype[dev->dev[i].type]);
	printf("%uns, ", dev->dev[i].speed);
	print_size(dev->dev[i].size);
	putchar('\n');
    }
    if (dev->ndev == 0)
	printf("%s  no_info\n", indent);
}

/*====================================================================*/

static void print_power(char *tag, cistpl_power_t *power)
{
    int i, n;
    for (i = n = 0; i < 8; i++)
	if (power->present & (1<<i)) n++;
    i = 0;
    printf("%s  %s", indent, tag);
    if (power->present & (1<<CISTPL_POWER_VNOM)) {
	printf(" Vnom "); i++;
	print_volt(power->param[CISTPL_POWER_VNOM]);
    }
    if (power->present & (1<<CISTPL_POWER_VMIN)) {
	printf(" Vmin "); i++;
	print_volt(power->param[CISTPL_POWER_VMIN]);
    }
    if (power->present & (1<<CISTPL_POWER_VMAX)) {
 	printf(" Vmax "); i++;
	print_volt(power->param[CISTPL_POWER_VMAX]);
    }
    if (power->present & (1<<CISTPL_POWER_ISTATIC)) {
	printf(" Istatic "); i++;
	print_current(power->param[CISTPL_POWER_ISTATIC]);
    }
    if (power->present & (1<<CISTPL_POWER_IAVG)) {
	if (++i == 5) printf("\n%s   ", indent);
	printf(" Iavg ");
	print_current(power->param[CISTPL_POWER_IAVG]);
    }
    if (power->present & (1<<CISTPL_POWER_IPEAK)) {
	if (++i == 5) printf("\n%s ", indent);
	printf(" Ipeak ");
	print_current(power->param[CISTPL_POWER_IPEAK]);
    }
    if (power->present & (1<<CISTPL_POWER_IDOWN)) {
	if (++i == 5) printf("\n%s ", indent);
	printf(" Idown ");
	print_current(power->param[CISTPL_POWER_IDOWN]);
    }
    if (power->flags & CISTPL_POWER_HIGHZ_OK) {
	if (++i == 5) printf("\n%s ", indent);
	printf(" [highz OK]");
    }
    if (power->flags & CISTPL_POWER_HIGHZ_REQ) {
	printf(" [highz]");
    }
    putchar('\n');
}

/*====================================================================*/

static void print_cftable_entry(cistpl_cftable_entry_t *entry)
{
    int i;
    
    printf("%scftable_entry 0x%2.2x%s\n", indent, entry->index,
	   (entry->flags & CISTPL_CFTABLE_DEFAULT) ? " [default]" : "");

    if (entry->flags & ~CISTPL_CFTABLE_DEFAULT) {
	printf("%s ", indent);
	if (entry->flags & CISTPL_CFTABLE_BVDS)
	    printf(" [bvd]");
	if (entry->flags & CISTPL_CFTABLE_WP)
	    printf(" [wp]");
	if (entry->flags & CISTPL_CFTABLE_RDYBSY)
	    printf(" [rdybsy]");
	if (entry->flags & CISTPL_CFTABLE_MWAIT)
	    printf(" [mwait]");
	if (entry->flags & CISTPL_CFTABLE_AUDIO)
	    printf(" [audio]");
	if (entry->flags & CISTPL_CFTABLE_READONLY)
	    printf(" [readonly]");
	if (entry->flags & CISTPL_CFTABLE_PWRDOWN)
	    printf(" [pwrdown]");
	putchar('\n');
    }
    
    if (entry->vcc.present)
	print_power("Vcc", &entry->vcc);
    if (entry->vpp1.present)
	print_power("Vpp1", &entry->vpp1);
    if (entry->vpp2.present)
	print_power("Vpp2", &entry->vpp2);

    if ((entry->timing.wait != 0) || (entry->timing.ready != 0) ||
	(entry->timing.reserved != 0)) {
	printf("%s  timing", indent);
	if (entry->timing.wait != 0) {
	    printf(" wait ");
	    print_time(entry->timing.wait, entry->timing.waitscale);
	}
	if (entry->timing.ready != 0) {
	    printf(" ready ");
	    print_time(entry->timing.ready, entry->timing.rdyscale);
	}
	if (entry->timing.reserved != 0) {
	    printf(" reserved ");
	    print_time(entry->timing.reserved, entry->timing.rsvscale);
	}
	putchar('\n');
    }
    
    if (entry->io.nwin) {
	cistpl_io_t *io = &entry->io;
	printf("%s  io", indent);
	for (i = 0; i < io->nwin; i++) {
	    if (i) putchar(',');
	    printf(" 0x%4.4x-0x%4.4x", io->win[i].base,
		   io->win[i].base+io->win[i].len-1);
	}
	printf(" [lines=%d]", io->flags & CISTPL_IO_LINES_MASK);
	if (io->flags & CISTPL_IO_8BIT) printf(" [8bit]");
	if (io->flags & CISTPL_IO_16BIT) printf(" [16bit]");
	if (io->flags & CISTPL_IO_RANGE) printf(" [range]");
	putchar('\n');
    }

    if (entry->irq.IRQInfo1) {
	printf("%s  irq ", indent);
	if (entry->irq.IRQInfo1 & IRQ_INFO2_VALID)
	    printf("mask 0x%04x", entry->irq.IRQInfo2);
	else
	    printf("%u", entry->irq.IRQInfo1 & IRQ_MASK);
	if (entry->irq.IRQInfo1 & IRQ_LEVEL_ID) printf(" [level]");
	if (entry->irq.IRQInfo1 & IRQ_PULSE_ID) printf(" [pulse]");
	if (entry->irq.IRQInfo1 & IRQ_SHARE_ID) printf(" [shared]");
	putchar('\n');
    }

    if (entry->mem.nwin) {
	cistpl_mem_t *mem = &entry->mem;
	printf("%s  memory", indent);
	for (i = 0; i < mem->nwin; i++) {
	    if (i) putchar(',');
	    printf(" 0x%4.4x-0x%4.4x @ 0x%4.4x", mem->win[i].card_addr,
		   mem->win[i].card_addr + mem->win[i].len-1,
		   mem->win[i].host_addr);
	}
	putchar('\n');
    }

    if (verbose && entry->subtuples)
	printf("%s  %d bytes in subtuples\n", indent, entry->subtuples);
    
}

/*====================================================================*/

static void print_cftable_entry_cb(cistpl_cftable_entry_cb_t *entry)
{
    int i;
    
    printf("%scftable_entry_cb 0x%2.2x%s\n", indent, entry->index,
	   (entry->flags & CISTPL_CFTABLE_DEFAULT) ? " [default]" : "");

    if (entry->flags & ~CISTPL_CFTABLE_DEFAULT) {
	printf("%s ", indent);
	if (entry->flags & CISTPL_CFTABLE_MASTER)
	    printf(" [master]");
	if (entry->flags & CISTPL_CFTABLE_INVALIDATE)
	    printf(" [invalidate]");
	if (entry->flags & CISTPL_CFTABLE_VGA_PALETTE)
	    printf(" [vga palette]");
	if (entry->flags & CISTPL_CFTABLE_PARITY)
	    printf(" [parity]");
	if (entry->flags & CISTPL_CFTABLE_WAIT)
	    printf(" [wait]");
	if (entry->flags & CISTPL_CFTABLE_SERR)
	    printf(" [serr]");
	if (entry->flags & CISTPL_CFTABLE_FAST_BACK)
	    printf(" [fast back]");
	if (entry->flags & CISTPL_CFTABLE_BINARY_AUDIO)
	    printf(" [binary audio]");
	if (entry->flags & CISTPL_CFTABLE_PWM_AUDIO)
	    printf(" [pwm audio]");
	putchar('\n');
    }
    
    if (entry->vcc.present)
	print_power("Vcc", &entry->vcc);
    if (entry->vpp1.present)
	print_power("Vpp1", &entry->vpp1);
    if (entry->vpp2.present)
	print_power("Vpp2", &entry->vpp2);

    if (entry->io) {
	printf("%s  io_base", indent);
	for (i = 0; i < 8; i++)
	    if (entry->io & (1<<i)) printf(" %d", i);
	putchar('\n');
    }

    if (entry->irq.IRQInfo1) {
	printf("%s  irq ", indent);
	if (entry->irq.IRQInfo1 & IRQ_INFO2_VALID)
	    printf("mask 0x%4.4x", entry->irq.IRQInfo2);
	else
	    printf("%u", entry->irq.IRQInfo1 & IRQ_MASK);
	if (entry->irq.IRQInfo1 & IRQ_LEVEL_ID) printf(" [level]");
	if (entry->irq.IRQInfo1 & IRQ_PULSE_ID) printf(" [pulse]");
	if (entry->irq.IRQInfo1 & IRQ_SHARE_ID) printf(" [shared]");
	putchar('\n');
    }

    if (entry->mem) {
	printf("%s  mem_base", indent);
	for (i = 0; i < 8; i++)
	    if (entry->mem & (1<<i)) printf(" %d", i);
	putchar('\n');
    }

    if (verbose && entry->subtuples)
	printf("%s  %d bytes in subtuples\n", indent,  entry->subtuples);
    
}

/*====================================================================*/

static void print_jedec(cistpl_jedec_t *j)
{
    int i;
    for (i = 0; i < j->nid; i++) {
	if (i != 0) putchar(',');
	printf(" 0x%02x 0x%02x", j->id[i].mfr, j->id[i].info);
    }
    putchar('\n');
}

/*====================================================================*/

static void print_device_geo(cistpl_device_geo_t *geo)
{
    int i;
    for (i = 0; i < geo->ngeo; i++) {
	printf("%s  width %d erase 0x%x read 0x%x write 0x%x "
	       "partition 0x%x interleave 0x%x\n", indent,
	       geo->geo[i].buswidth, geo->geo[i].erase_block,
	       geo->geo[i].read_block, geo->geo[i].write_block,
	       geo->geo[i].partition, geo->geo[i].interleave);
    }
}

/*====================================================================*/

static void print_org(cistpl_org_t *org)
{
    printf("%sdata_org ", indent);
    switch (org->data_org) {
    case CISTPL_ORG_FS:
	printf("[filesystem]"); break;
    case CISTPL_ORG_APPSPEC:
	printf("[app_specific]"); break;
    case CISTPL_ORG_XIP:
	printf("[code]"); break;
    default:
	if (org->data_org < 0x80)
	    printf("[reserved]");
	else
	    printf("[vendor_specific]");
    }
    printf(", \"%s\"\n", org->desc);
}

/*====================================================================*/

static char *data_mod[] = {
    "Bell103", "V.21", "V.23", "V.22", "Bell212A", "V.22bis",
    "V.26", "V.26bis", "V.27bis", "V.29", "V.32", "V.32bis",
    "V.34", "rfu", "rfu", "rfu"
};
static char *fax_mod[] = {
    "V.21-C2", "V.27ter", "V.29", "V.17", "V.33", "rfu", "rfu", "rfu"
};
static char *fax_features[] = {
    "T.3", "T.4", "T.6", "error", "voice", "poll", "file", "passwd"
};
static char *cmd_protocol[] = {
    "AT1", "AT2", "AT3", "MNP_AT", "V.25bis", "V.25A", "DMCL"
};
static char *uart[] = {
    "8250", "16450", "16550", "8251", "8530", "85230"
};
static char *parity[] = { "space", "mark", "odd", "even" };
static char *stop[] = { "1", "1.5", "2" };
static char *flow[] = {
    "XON/XOFF xmit", "XON/XOFF rcv", "hw xmit", "hw rcv", "transparent"
};
static void print_serial(cistpl_funce_t *funce)
{
    cistpl_serial_t *s;
    cistpl_data_serv_t *ds;
    cistpl_fax_serv_t *fs;
    cistpl_modem_cap_t *cp;
    int i, j;
    
    switch (funce->type & 0x0f) {
    case CISTPL_FUNCE_SERIAL_IF:
    case CISTPL_FUNCE_SERIAL_IF_DATA:
    case CISTPL_FUNCE_SERIAL_IF_FAX:
    case CISTPL_FUNCE_SERIAL_IF_VOICE:
	s = (cistpl_serial_t *)(funce->data);
	printf("%sserial_interface", indent);
	if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_IF_DATA)
	    printf("_data");
	else if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_IF_FAX)
	    printf("_fax");
	else if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_IF_VOICE)
	    printf("_voice");
	printf("\n%s  uart %s", indent,
	       (s->uart_type < 6) ? uart[s->uart_type] : "reserved");
	if (s->uart_cap_0) {
	    printf(" [");
	    for (i = 0; i < 4; i++)
	        if (s->uart_cap_0 & (1<<i))
		    printf("%s%s", parity[i],
			   (s->uart_cap_0 >= (2<<i)) ? "/" : "]");
	}
	if (s->uart_cap_1) {
	    int m = s->uart_cap_1 & 0x0f;
	    int n = s->uart_cap_1 >> 4;
	    printf(" [");
	    for (i = 0; i < 4; i++)
		if (m & (1<<i))
		    printf("%d%s", i+5, (m >= (2<<i)) ? "/" : "");
	    printf("] [");
	    for (i = 0; i < 3; i++)
	        if (n & (1<<i))
		    printf("%s%s", stop[i], (n >= (2<<i)) ? "/" : "]");
	}
	printf("\n");
	break;
    case CISTPL_FUNCE_SERIAL_CAP:
    case CISTPL_FUNCE_SERIAL_CAP_DATA:
    case CISTPL_FUNCE_SERIAL_CAP_FAX:
    case CISTPL_FUNCE_SERIAL_CAP_VOICE:
	cp = (cistpl_modem_cap_t *)(funce->data);
	printf("%sserial_modem_cap", indent);
	if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_CAP_DATA)
	    printf("_data");
	else if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_CAP_FAX)
	    printf("_fax");
	else if ((funce->type & 0x0f) == CISTPL_FUNCE_SERIAL_CAP_VOICE)
	    printf("_voice");
	if (cp->flow) {
	    printf("\n%s  flow", indent);
	    for (i = 0; i < 5; i++)
		if (cp->flow & (1<<i))
		    printf(" [%s]", flow[i]);
	}
	printf("\n%s  cmd_buf %d rcv_buf %d xmit_buf %d\n",
	       indent, 4*(cp->cmd_buf+1),
	       cp->rcv_buf_0+(cp->rcv_buf_1<<8)+(cp->rcv_buf_2<<16),
	       cp->xmit_buf_0+(cp->xmit_buf_1<<8)+(cp->xmit_buf_2<<16));
	break;
    case CISTPL_FUNCE_SERIAL_SERV_DATA:
	ds = (cistpl_data_serv_t *)(funce->data);
	printf("%sserial_data_services\n", indent);
	printf("%s  data_rate %d\n", indent,
	       75*((ds->max_data_0<<8) + ds->max_data_1));
	printf("%s  modulation", indent);
	for (i = j = 0; i < 16; i++)
	    if (((ds->modulation_1<<8) + ds->modulation_0) & (1<<i)) {
		if (++j % 6 == 0)
		    printf("\n%s   ", indent);
		printf(" [%s]", data_mod[i]);
	    }
	printf("\n");
	if (ds->error_control) {
	    printf("%s  error_control", indent);
	    if (ds->error_control & CISTPL_SERIAL_ERR_MNP2_4)
		printf(" [MNP2-4]");
	    if (ds->error_control & CISTPL_SERIAL_ERR_V42_LAPM)
		printf(" [V.42/LAPM]");
	    printf("\n");
	}
	if (ds->compression) {
	    printf("%s  compression", indent);
	    if (ds->compression & CISTPL_SERIAL_CMPR_V42BIS)
		printf(" [V.42bis]");
	    if (ds->compression & CISTPL_SERIAL_CMPR_MNP5)
		printf(" [MNP5]");
	    printf("\n");
	}
	if (ds->cmd_protocol) {
	    printf("%s  cmd_protocol", indent);
	    for (i = 0; i < 7; i++)
		if (ds->cmd_protocol & (1<<i))
		    printf(" [%s]", cmd_protocol[i]);
	    printf("\n");
	}
	break;
	
    case CISTPL_FUNCE_SERIAL_SERV_FAX:
	fs = (cistpl_fax_serv_t *)(funce->data);
	printf("%sserial_fax_services [class=%d]\n",
	       indent, funce->type>>4);
	printf("%s  data_rate %d\n", indent,
	       75*((fs->max_data_0<<8) + fs->max_data_1));
	printf("%s  modulation", indent);
	for (i = 0; i < 8; i++)
	    if (fs->modulation & (1<<i))
		printf(" [%s]", fax_mod[i]);
	printf("\n");
	if (fs->features_0) {
	    printf("%s  features", indent);
	    for (i = 0; i < 8; i++)
		if (fs->features_0 & (1<<i))
		    printf(" [%s]", fax_features[i]);
	    printf("\n");
	}
	break;
    }
}

/*====================================================================*/

static void print_fixed(cistpl_funce_t *funce)
{
    cistpl_ide_interface_t *i;
    cistpl_ide_feature_t *f;
    
    switch (funce->type) {
    case CISTPL_FUNCE_IDE_IFACE:
	i = (cistpl_ide_interface_t *)(funce->data);
	printf("%sdisk_interface ", indent);
	if (i->interface == CISTPL_IDE_INTERFACE)
	    printf("[ide]\n");
	else
	    printf("[undefined]\n");
	break;
    case CISTPL_FUNCE_IDE_MASTER:
    case CISTPL_FUNCE_IDE_SLAVE:
	f = (cistpl_ide_feature_t *)(funce->data);
	printf("%sdisk_features", indent);
	if (f->feature1 & CISTPL_IDE_SILICON)
	    printf(" [silicon]");
	else
	    printf(" [rotating]");
	if (f->feature1 & CISTPL_IDE_UNIQUE)
	    printf(" [unique]");
	if (f->feature1 & CISTPL_IDE_DUAL)
	    printf(" [dual]");
	else
	    printf(" [single]");
	if (f->feature1 && f->feature2)
	    printf("\n%s ", indent);
	if (f->feature2 & CISTPL_IDE_HAS_SLEEP)
	    printf(" [sleep]");
	if (f->feature2 & CISTPL_IDE_HAS_STANDBY)
	    printf(" [standby]");
	if (f->feature2 & CISTPL_IDE_HAS_IDLE)
	    printf(" [idle]");
	if (f->feature2 & CISTPL_IDE_LOW_POWER)
	    printf(" [low power]");
	if (f->feature2 & CISTPL_IDE_REG_INHIBIT)
	    printf(" [reg inhibit]");
	if (f->feature2 & CISTPL_IDE_HAS_INDEX)
	    printf(" [index]");
	if (f->feature2 & CISTPL_IDE_IOIS16)
	    printf(" [iois16]");
	putchar('\n');
	break;
    }
}

/*====================================================================*/

static const char *tech[] = {
    "undefined", "ARCnet", "ethernet", "token_ring", "localtalk",
    "FDDI/CDDI", "ATM", "wireless"
};

static const char *media[] = {
    "undefined", "unshielded_twisted_pair", "shielded_twisted_pair",
    "thin_coax", "thick_coax", "fiber", "900_MHz", "2.4_GHz",
    "5.4_GHz", "diffuse_infrared", "point_to_point_infrared"
};

static void print_network(cistpl_funce_t *funce)
{
    cistpl_lan_tech_t *t;
    cistpl_lan_speed_t *s;
    cistpl_lan_media_t *m;
    cistpl_lan_node_id_t *n;
    cistpl_lan_connector_t *c;
    int i;
    
    switch (funce->type) {
    case CISTPL_FUNCE_LAN_TECH:
	t = (cistpl_lan_tech_t *)(funce->data);
	printf("%slan_technology %s\n", indent, tech[t->tech]);
	break;
    case CISTPL_FUNCE_LAN_SPEED:
	s = (cistpl_lan_speed_t *)(funce->data);
	printf("%slan_speed ", indent);
	print_speed(s->speed);
	putchar('\n');
	break;
    case CISTPL_FUNCE_LAN_MEDIA:
	m = (cistpl_lan_media_t *)(funce->data);
	printf("%slan_media %s\n", indent, media[m->media]);
	break;
    case CISTPL_FUNCE_LAN_NODE_ID:
	n = (cistpl_lan_node_id_t *)(funce->data);
	printf("%slan_node_id", indent);
	for (i = 0; i < n->nb; i++)
	    printf(" %02x", n->id[i]);
	putchar('\n');
	break;
    case CISTPL_FUNCE_LAN_CONNECTOR:
	c = (cistpl_lan_connector_t *)(funce->data);
	printf("%slan_connector ", indent);
	if (c->code == 0)
	    printf("Open connector standard\n");
	else
	    printf("Closed connector standard\n");
	break;
    }
}

/*====================================================================*/

static void print_vers_1(cistpl_vers_1_t *v1)
{
    int i, n;
    char s[32];
    sprintf(s, "%svers_1 %d.%d", indent, v1->major, v1->minor);
    printf("%s", s);
    n = strlen(s);
    for (i = 0; i < v1->ns; i++) {
	if (n + strlen(v1->str + v1->ofs[i]) + 4 > 72) {
	    n = strlen(indent) + 2;
	    printf(",\n%s  ", indent);
	} else {
	    printf(", ");
	    n += 2;
	}
	printf("\"%s\"", v1->str + v1->ofs[i]);
	n += strlen(v1->str + v1->ofs[i]) + 2;
    }
    putchar('\n');
}

/*====================================================================*/

static void print_vers_2(cistpl_vers_2_t *v2)
{
    printf("%sversion 0x%2.2x, compliance 0x%2.2x, dindex 0x%4.4x\n",
	   indent, v2->vers, v2->comply, v2->dindex);
    printf("%s  vspec8 0x%2.2x, vspec9 0x%2.2x, nhdr %d\n",
	   indent, v2->vspec8, v2->vspec9, v2->nhdr);
    printf("%s  vendor \"%s\"\n", indent, v2->str+v2->vendor);
    printf("%s  info \"%s\"\n", indent, v2->str+v2->info);
}

/*====================================================================*/

#ifdef CISTPL_FORMAT_DISK
static void print_format(cistpl_format_t *fmt)
{
    if (fmt->type == CISTPL_FORMAT_DISK)
	printf("%s  [disk]", indent);
    else if (fmt->type == CISTPL_FORMAT_MEM)
	printf("%s  [memory]", indent);
    else
	printf("%s  [type 0x%02x]\n", indent, fmt->type);
    if (fmt->edc == CISTPL_EDC_NONE)
	printf(" [no edc]");
    else if (fmt->edc == CISTPL_EDC_CKSUM)
	printf(" [cksum]");
    else if (fmt->edc == CISTPL_EDC_CRC)
	printf(" [crc]");
    else if (fmt->edc == CISTPL_EDC_PCC)
	printf(" [pcc]");
    else
	printf(" [edc 0x%02x]", fmt->edc);
    printf(" offset 0x%04x length ", fmt->offset);
    print_size(fmt->length);
    putchar('\n');
}
#endif

/*====================================================================*/

static void print_config(int code, cistpl_config_t *cfg)
{
    printf("%sconfig%s base 0x%4.4x", indent,
	   (code == CISTPL_CONFIG_CB) ? "_cb" : "",
	   cfg->base);
    if (code == CISTPL_CONFIG)
	printf(" mask 0x%4.4x", cfg->rmask[0]);
    printf(" last_index 0x%2.2x\n", cfg->last_idx);
    if (verbose && cfg->subtuples)
	printf("%s  %d bytes in subtuples\n", indent, cfg->subtuples);
}

/*====================================================================*/

static int nfn = 0, cur = 0;

static void print_parse(tuple_parse_t *tup)
{
    static int func = 0;
    int i;
    
    switch (tup->tuple.TupleCode) {
    case CISTPL_DEVICE:
    case CISTPL_DEVICE_A:
	if (tup->tuple.TupleCode == CISTPL_DEVICE)
	    printf("%sdev_info\n", indent);
	else
	    printf("%sattr_dev_info\n", indent);
	print_device(&tup->parse.device);
	break;
    case CISTPL_CHECKSUM:
	printf("%schecksum 0x%04x-0x%04x = 0x%02x\n",
	       indent, tup->parse.checksum.addr,
	       tup->parse.checksum.addr+tup->parse.checksum.len-1,
	       tup->parse.checksum.sum);
	break;
    case CISTPL_LONGLINK_A:
	if (verbose)
	    printf("%slong_link_attr 0x%04x\n", indent,
		   tup->parse.longlink.addr);
	break;
    case CISTPL_LONGLINK_C:
	if (verbose)
	    printf("%slong_link 0x%04x\n", indent,
		   tup->parse.longlink.addr);
	break;
    case CISTPL_LONGLINK_MFC:
	if (verbose) {
	    printf("%smfc_long_link\n", indent);
	    for (i = 0; i < tup->parse.longlink_mfc.nfn; i++)
		printf("%s function %d: %s 0x%04x\n", indent, i,
		       tup->parse.longlink_mfc.fn[i].space ? "common" : "attr",
		       tup->parse.longlink_mfc.fn[i].addr);
	} else {
	    printf("%smfc {\n", indent);
	    nfn = tup->parse.longlink_mfc.nfn;
	    cur = 0;
	    strcat(indent, "  ");
	}
	break;
    case CISTPL_NO_LINK:
	if (verbose)
	    printf("%sno_long_link\n", indent);
	break;
#ifdef CISTPL_INDIRECT
    case CISTPL_INDIRECT:
	if (verbose)
	    printf("%sindirect_access\n", indent);
	break;
#endif
    case CISTPL_LINKTARGET:
	if (verbose)
	    printf("%slink_target\n", indent);
	else {
	    if (cur++) printf("%s}, {\n", indent+2);
	}
	break;
    case CISTPL_VERS_1:
	print_vers_1(&tup->parse.version_1);
	break;
    case CISTPL_ALTSTR:
	break;
    case CISTPL_JEDEC_A:
    case CISTPL_JEDEC_C:
	if (tup->tuple.TupleCode == CISTPL_JEDEC_C)
	    printf("%scommon_jedec", indent);
	else
	    printf("%sattr_jedec", indent);
	print_jedec(&tup->parse.jedec);
	break;
    case CISTPL_DEVICE_GEO:
    case CISTPL_DEVICE_GEO_A:
	if (tup->tuple.TupleCode == CISTPL_DEVICE_GEO)
	    printf("%scommon_geometry\n", indent);
	else
	    printf("%sattr_geometry\n", indent);
	print_device_geo(&tup->parse.device_geo);
	break;
    case CISTPL_MANFID:
	printf("%smanfid 0x%4.4x, 0x%4.4x\n", indent,
	       tup->parse.manfid.manf, tup->parse.manfid.card);
	break;
    case CISTPL_FUNCID:
	print_funcid(&tup->parse.funcid);
	func = tup->parse.funcid.func;
	break;
    case CISTPL_FUNCE:
	switch (func) {
	case CISTPL_FUNCID_SERIAL:
	    print_serial(&tup->parse.funce);
	    break;
	case CISTPL_FUNCID_FIXED:
	    print_fixed(&tup->parse.funce);
	    break;
	case CISTPL_FUNCID_NETWORK:
	    print_network(&tup->parse.funce);
	    break;
	}
	break;
    case CISTPL_BAR:
	printf("%sBAR %d size ", indent,
	       tup->parse.bar.attr & CISTPL_BAR_SPACE);
	print_size(tup->parse.bar.size);
	if (tup->parse.bar.attr & CISTPL_BAR_SPACE_IO)
	    printf(" [io]");
	else
	    printf(" [mem]");
	if (tup->parse.bar.attr & CISTPL_BAR_PREFETCH)
	    printf(" [prefetch]");
	if (tup->parse.bar.attr & CISTPL_BAR_CACHEABLE)
	    printf(" [cacheable]");
	if (tup->parse.bar.attr & CISTPL_BAR_1MEG_MAP)
	    printf(" [<1mb]");
	putchar('\n');
	break;
    case CISTPL_CONFIG:
    case CISTPL_CONFIG_CB:
	print_config(tup->tuple.TupleCode, &tup->parse.config);
	break;
    case CISTPL_CFTABLE_ENTRY:
	print_cftable_entry(&tup->parse.cftable_entry);
	break;
    case CISTPL_CFTABLE_ENTRY_CB:
	print_cftable_entry_cb(&tup->parse.cftable_entry_cb);
	break;
    case CISTPL_VERS_2:
	print_vers_2(&tup->parse.vers_2);
	break;
    case CISTPL_ORG:
	print_org(&tup->parse.org);
	break;
#ifdef CISTPL_FORMAT_DISK
    case CISTPL_FORMAT:
    case CISTPL_FORMAT_A:
	if (tup->tuple.TupleCode == CISTPL_FORMAT)
	    printf("%scommon_format\n", indent);
	else
	    printf("%sattr_format\n", indent);
	print_format(&tup->parse.format);
#endif
    }
}

/*====================================================================*/

static int get_tuple_buf(int fd, ds_ioctl_arg_t *arg, int first)
{
    u_int ofs;
    static int nb = 0;
    static u_char buf[1024];
    
    if (first) {
	nb = read(fd, buf, sizeof(buf));
	arg->tuple.TupleLink = arg->tuple.CISOffset = 0;
    }
    ofs = arg->tuple.CISOffset + arg->tuple.TupleLink;
    if (ofs >= nb) return -1;
    arg->tuple.TupleCode = buf[ofs++];
    arg->tuple.TupleDataLen = arg->tuple.TupleLink = buf[ofs++];
    arg->tuple.CISOffset = ofs;
    memcpy(arg->tuple_parse.data, buf+ofs, arg->tuple.TupleLink);
    return 0;
}

static int get_tuple(int fd, ds_ioctl_arg_t *arg, int first)
{
    int cmd = (first) ? DS_GET_FIRST_TUPLE : DS_GET_NEXT_TUPLE;
    if (ioctl(fd, cmd, arg) != 0) {
	if (errno == ENODEV)
	    printf("%sno card\n", indent);
	else if (errno != ENODATA)
	    printf("%sget tuple: %s\n", indent, strerror(errno));
	return -1;
    }
    if (ioctl(fd, DS_GET_TUPLE_DATA, arg) != 0) {
	printf("%sget tuple data: %s\n", indent, strerror(errno));
	return -1;
    }
    return 0;
}

/*====================================================================*/

#define MAX_SOCKS 8

int main(int argc, char *argv[])
{
    int i, fd, pfd = -1;
    ds_ioctl_arg_t arg;
    int optch, errflg, first;
    int force = 0;
    char *infile = NULL;

    errflg = 0;
    while ((optch = getopt(argc, argv, "fvi:")) != -1) {
	switch (optch) {
	case 'f':
	    force = 1; break;
	case 'v':
	    verbose = 1; break;
	case 'i':
	    infile = strdup(optarg); break;
	default:
	    errflg = 1; break;
	}
    }
    if (errflg || (optind < argc)) {
	fprintf(stderr, "usage: %s [-v] [-f] [-i infile]\n", argv[0]);
	exit(EXIT_FAILURE);
    }

#if (!defined(__BEOS__) && !defined(__HAIKU__))
    major = lookup_dev("pcmcia");
    if (major < 0) {
	fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	exit(EXIT_FAILURE);
    }
#endif
    
    for (i = 0; (i < MAX_SOCKS) && !(i && infile); i++) {
	nfn = cur = 0;
	if (infile) {
	    indent[0] = '\0';
	    fd = open(infile, O_RDONLY);
	    if (fd < 0) {
		perror("open()");
		return -1;
	    }
	    pfd = open_sock(0);
	} else {
	    strcpy(indent, "  ");
	    fd = pfd = open_sock(i);
	}
	if (pfd < 0)
	    break;
	if (!verbose && (i > 0)) putchar('\n');
	if (!infile) printf("Socket %d:\n", i);
	
	if (!force && !infile) {
	    if (ioctl(fd, DS_VALIDATE_CIS, &arg) != 0) {
		printf("%svalidate CIS: %s\n", indent, strerror(errno));
		continue;
	    }
	    if (arg.cisinfo.Chains == 0) {
		printf("%sno CIS present\n", indent);
		continue;
	    }
	}
	
	arg.tuple.TupleDataMax = sizeof(arg.tuple_parse.data);
	arg.tuple.Attributes = TUPLE_RETURN_LINK | TUPLE_RETURN_COMMON;
	arg.tuple.DesiredTuple = RETURN_FIRST_TUPLE;
	arg.tuple.TupleOffset = 0;

	for (first = 1; ; first = 0) {
	    if (infile) {
		if (get_tuple_buf(fd, &arg, first) != 0) break;
	    } else {
		if (get_tuple(fd, &arg, first) != 0) break;
	    }
	    if (verbose) print_tuple(&arg.tuple_parse);
	    if (ioctl(pfd, DS_PARSE_TUPLE, &arg) == 0)
		print_parse(&arg.tuple_parse);
	    else if (errno != ENOSYS)
		printf("%sparse error: %s\n", indent,
		       strerror(errno));
	    if (verbose) putchar('\n');
	    if (arg.tuple.TupleCode == CISTPL_END)
		break;
	}
	
	if (!verbose && (nfn > 0))
	    printf("%s}\n", indent+2);
    }
    if ((i == 0) && (pfd < 0)) {
	perror("open()");
	return -1;
    }
    
    return 0;
}
