#ifndef CONFIG_LIBMICRO_H
#define CONFIG_LIBMICRO_H

#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

#ifndef PF_UNIX
#define PF_UNIX	AF_UNIX
#endif

#endif /* CONFIG_LIBMICRO_H */
