#ifndef _SYS_SOCKIO_H
#define _SYS_SOCKIO_H

#include <sys/ioccom.h>

/* Socket ioctl's. */ 
#define SIOCSHIWAT      _IOW('s',  0, int)             /* set high watermark */
#define SIOCGHIWAT      _IOR('s',  1, int)             /* get high watermark */
#define SIOCSLOWAT      _IOW('s',  2, int)             /* set low watermark */
#define SIOCGLOWAT      _IOR('s',  3, int)             /* get low watermark */
#define SIOCATMARK      _IOR('s',  7, int)             /* at oob mark? */

#define SIOCADDRT       _IOW('r', 10, struct ortentry) /* add route */
#define SIOCDELRT       _IOW('r', 11, struct ortentry) /* delete route */

#define SIOCSIFADDR     _IOW('i', 12, struct ifreq)    /* set ifnet address */
#define OSIOCGIFADDR    _IOWR('i', 13, struct ifreq)    /* get ifnet address */
#define SIOCGIFADDR     _IOWR('i', 33, struct ifreq)    /* get ifnet address */
#define SIOCSIFDSTADDR  _IOW('i', 14, struct ifreq)    /* set p-p address */
#define OSIOCGIFDSTADDR _IOWR('i', 15, struct ifreq)    /* get p-p address */
#define SIOCGIFDSTADDR  _IOWR('i', 34, struct ifreq)    /* get p-p address */
#define SIOCSIFFLAGS    _IOW('i', 16, struct ifreq)    /* set ifnet flags */
#define SIOCGIFFLAGS    _IOWR('i', 17, struct ifreq)    /* get ifnet flags */
#define OSIOCGIFBRDADDR _IOWR('i', 18, struct ifreq)    /* get broadcast addr */
#define SIOCGIFBRDADDR  _IOWR('i', 35, struct ifreq)    /* get broadcast addr */
#define SIOCSIFBRDADDR  _IOW('i', 19, struct ifreq)    /* set broadcast addr */
#define OSIOCGIFCONF    _IOWR('i', 20, struct ifconf)   /* get ifnet list */
#define SIOCGIFCONF     _IOWR('i', 36, struct ifconf)   /* get ifnet list */
#define OSIOCGIFNETMASK _IOWR('i', 21, struct ifreq)    /* get net addr mask */
#define SIOCGIFNETMASK  _IOWR('i', 37, struct ifreq)    /* get net addr mask */
#define SIOCSIFNETMASK  _IOW('i', 22, struct ifreq)    /* set net addr mask */
#define SIOCGIFMETRIC   _IOWR('i', 23, struct ifreq)    /* get IF metric */
#define SIOCSIFMETRIC   _IOW('i', 24, struct ifreq)    /* set IF metric */
#define SIOCDIFADDR     _IOW('i', 25, struct ifreq)    /* delete IF addr */
#define SIOCAIFADDR      _IOW('i', 26, struct ifaliasreq)/* add/chg IF alias */
#define SIOCGIFDATA     _IOWR('i', 27, struct ifreq)    /* get if_data */

#define SIOCGIFMTU      _IOWR('i', 126, struct ifreq)   /* get ifnet MTU */
#define SIOCSIFMTU      _IOW('i', 127, struct ifreq)    /* set ifnet MTU */

#define SIOCADDMULTI    _IOW('i', 49, struct ifreq)    /* add m'cast addr */
#define SIOCDELMULTI    _IOW('i', 50, struct ifreq)    /* del m'cast addr */

#endif /* SYS_NET_IOCTL_H */

