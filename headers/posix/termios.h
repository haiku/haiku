#ifndef _TERMIOS_H_
#define _TERMIOS_H_

/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <be_setup.h>
#include <sys/types.h>
#include <unistd.h>


#define NCC	  11			/* number of control characters */
#define NCCS  NCC

/* control characters */
#define	VINTR	0
#define	VQUIT	1
#define	VERASE	2
#define	VKILL	3
#define	VEOF	4
#define	VEOL	5
#define	VMIN	4
#define	VTIME	5
#define	VEOL2	6
#define	VSWTCH	7
#define VSTART  8
#define VSTOP   9
#define VSUSP   10


#define termio termios   /* for old non-posix code */
	
typedef unsigned long tcflag_t;
typedef unsigned char speed_t;
typedef unsigned char cc_t;

struct	termios {
	tcflag_t	c_iflag;	/* input modes */
	tcflag_t	c_oflag;	/* output modes */
	tcflag_t	c_cflag;	/* control modes */
	tcflag_t	c_lflag;	/* local modes */
	char		c_line;		/* line discipline */
	speed_t		c_ixxxxx;	/* (unused) */
	speed_t		c_oxxxxx;	/* (unused) */
	cc_t		c_cc[NCC];	/* control chars */
};


struct winsize {                    /* for the TIOCGWINSZ ioctl */
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};


/*
 * c_iflag - input control modes
 */
#define	IGNBRK		0x01		/* ignore breaks */
#define	BRKINT		0x02		/* break sends interrupt */
#define	IGNPAR		0x04		/* ignore chars with parity errors */
#define	PARMRK		0x08		/* marks parity errors */
#define	INPCK		0x10		/* enable input parity checking */
#define	ISTRIP		0x20		/* strip high bit from characters */
#define	INLCR		0x40		/* maps newline to CR on input */
#define	IGNCR		0x80		/* ignores carriage returns */
#define	ICRNL		0x100		/* maps CR to newlines on input */
#define	IUCLC		0x200		/* maps all upper case to lower */
#define	IXON		0x400		/* enables input SW flow control */
#define	IXANY		0x800		/* any character will restart input */
#define	IXOFF		0x1000		/* enables output SW flow control */	


/*
 * c_oflag - output control modes
 */
#define	OPOST		0x01		/* enable postprocessing of output */
#define	OLCUC		0x02		/* maps lowercase to uppercase */
#define	ONLCR		0x04		/* maps NL to CR-NL on output */
#define	OCRNL		0x08		/* maps CR to NL on output */
#define	ONOCR		0x10		/* no CR output when at column 0 */
#define	ONLRET		0x20		/* newline performs CR function */
#define	OFILL		0x40		/* uses fill characters for delays */
#define	OFDEL		0x80		/* Fills are DEL, otherwise NUL */
#define	NLDLY		0x100		/* Newline delays: */
#define	NL0			0x000
#define	NL1			0x100
#define	CRDLY		0x600		/* Carriage return delays: */
#define	CR0			0x000
#define	CR1			0x200
#define	CR2			0x400
#define	CR3			0x600
#define	TABDLY		0x1800		/* Tab delays: */
#define	TAB0		0x0000
#define	TAB1		0x0800
#define	TAB2		0x1000
#define	TAB3		0x1800
#define	BSDLY		0x2000		/* Backspace delays: */
#define	BS0			0x0000
#define	BS1			0x2000
#define	VTDLY		0x4000		/* Vertical tab delays: */
#define	VT0			0x0000
#define	VT1			0x4000
#define	FFDLY		0x8000		/* Form feed delays: */
#define	FF0			0x0000
#define	FF1			0x8000

/*
 * c_cflag - control modes
 */
#define	CBAUD		0x1F			/* line speed definitions */

#define	B0			0x00
#define	B50			0x01
#define	B75			0x02
#define	B110		0x03
#define	B134		0x04
#define	B150		0x05
#define	B200		0x06
#define	B300		0x07
#define	B600		0x08
#define	B1200		0x09
#define	B1800		0x0A
#define	B2400		0x0B
#define	B4800		0x0C
#define	B9600		0x0D
#define	B19200		0x0E
#define	B38400		0x0F
#define B57600		0x10
#define B115200		0x11
#define B230400		0x12
#define B31250		0x13			/* for MIDI */

#define	CSIZE		0x20			/* character size */
#define	CS5			0x00			/* only 7 and 8 bits supported */
#define	CS6			0x00
#define	CS7			0x00
#define	CS8			0x20
#define	CSTOPB		0x40			/* send 2 stop bits, not 1 */
#define	CREAD		0x80			/* enables receiver */
#define	PARENB		0x100			/* xmit parity enable */
#define	PARODD		0x200			/* odd parity, else even */
#define	HUPCL		0x400			/* hangs up on last close */
#define	CLOCAL		0x800			/* indicates local line */
#define	XLOBLK		0x1000			/* block layer output ?*/
#define	CTSFLOW		0x2000			/* enable CTS flow */
#define	RTSFLOW		0x4000			/* enable RTS flow */
#define	CRTSCTS		(RTSFLOW | CTSFLOW)

/*
 * c_lflag - local modes
 */
#define ISIG		(0x01)			/* enable signals */
#define ICANON		(0x02)			/* Canonical input */
#define XCASE		(0x04)			/* Canonical u/l case */
#define ECHO		(0x08)			/* Enable echo */
#define ECHOE		(0x10)			/* Echo erase as bs-sp-bs */
#define ECHOK		(0x20)			/* Echo nl after kill */
#define ECHONL		(0x40)			/* Echo nl */
#define NOFLSH		(0x80)			/* Disable flush after int or quit */
#define TOSTOP      (0x100)         /* stop bg processes that write to tty */
#define IEXTEN      (0x200)         /* implementation defined extensions */

/*
 * Event codes.  Returned from TCWAITEVENT
 */
#define EV_RING			0x0001
#define EV_BREAK		0x0002
#define EV_CARRIER		0x0004
#define EV_CARRIERLOST	0x0008

/*
 * These are ioctl identifiers to control the serial driver.
 */
#define TCGETA			(0x8000)
#define TCSETA			(TCGETA+1)
#define TCSETAF			(TCGETA+2)
#define TCSETAW			(TCGETA+3)
#define TCWAITEVENT		(TCGETA+4)
#define TCSBRK			(TCGETA+5)
#define TCFLSH			(TCGETA+6)
#define TCXONC			(TCGETA+7)
#define TCQUERYCONNECTED (TCGETA+8)
#define TCGETBITS		(TCGETA+9)
#define	TCSETDTR		(TCGETA+10)
#define TCSETRTS		(TCGETA+11)
#define TIOCGWINSZ		(TCGETA+12)		/* pass in a struct winsize */
#define TIOCSWINSZ		(TCGETA+13)		/* pass in a struct winsize */
#define TCVTIME			(TCGETA+14)		/* pass in bigtime_t, old val saved */

/*
 * Bits for the TCGETBITS control
 */
#define	TCGB_CTS		0x01
#define TCGB_DSR		0x02
#define TCGB_RI			0x04
#define TCGB_DCD		0x08


__extern_c_start

#define  tcgetattr(f, t) ioctl(f, TCGETA, (char *)t)

extern speed_t	cfgetispeed( const struct termios *);
extern speed_t	cfgetospeed( const struct termios *);
extern int		cfsetispeed( struct termios *, speed_t);
extern int		cfsetospeed( struct termios *, speed_t);
extern int		tcsetattr( int fd, int opt, const struct termios *tp);

/* 
 * Options to tcsetattr()
 */
#define TCSANOW    0x01		/* make change immediate */
#define TCSADRAIN  0x02		/* drain output, then change */
#define TCSAFLUSH  0x04		/* drain output, flush input */


int   tcsendbreak(int fd, int duration);
int   tcdrain(int fd);
int   tcflow(int fd, int action);

/*
 * Actions for tcflow()
 */
#define TCOOFF 0x01		
#define TCOON  0x02
#define TCIOFF 0x04
#define TCION  0x08


int   tcflush(int fd, int queue_selector);

/* 
 * Values for "queue_selector" in tcflush()
 */
#define TCIFLUSH  0x01		
#define TCOFLUSH  0x02
#define TCIOFLUSH (TCIFLUSH | TCOFLUSH)

int   tcsetpgrp(int fd, pid_t pgrpid);
pid_t tcgetpgrp(int fd);

__extern_c_end

#endif /* _TERMIOS_H_ */
