/* 
 * ppp_ctrl.h
 *
 * Definitions required to control PPP from userland
 */
 
#ifndef _NET_PPP_CTRL_H_
#define _NET_PPP_CTRL_H_

#define PPPCTRL_THREADNAME     "PPP_Control_Thread"
#define PPPCTRL_MAXSIZE      100     /* biggest lump of data we accept */

enum {
	PPPCTRL_OK = 0,                /* Operation completed OK */
	PPPCTRL_FAIL,                  /* Operation failed */
	PPPCTRL_CREATE,                /* create a new PPP device */
	PPPCTRL_DELETE,                /* delete a PPP device */
	PPPCTRL_UP,                    /* start a ppp connection */
	PPPCTRL_OPEN,                  /* Open a device */
	PPPCTRL_DOWN,                  /* stop a ppp connection */

};

#endif /* _NET_PPP_CTRL_H_ */
