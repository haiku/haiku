/* if_types.h 
 *
 * <URL:http://www.iana.org/assignments/ianaiftype-mib>
 */

#ifndef NET_IF_TYPES_H
#define NET_IF_TYPES_H

/* We just list the ones here we actually use... */

#define IFT_ETHER         0x06
#define IFT_PPP           0x17
#define IFT_LOOP          0x18
#define IFT_SLIP          0x1c
#define IFT_RS232         0x21
#define IFT_PARA          0x22 /* Parallel port! ?? */
#define IFT_ATM           0x25
#define IFT_MODEM         0x31 /* Just a simple generic modem */
#define IFT_FASTETHER     0x32 /* 100BaseT ethernet */
#define IFT_ISDN          0x3f /* ISDN / X.25 */

#endif /* NET_IF_TYPES_H */

