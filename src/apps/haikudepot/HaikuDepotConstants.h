/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


enum {
	MSG_MAIN_WINDOW_CLOSED		= 'mwcl',
	MSG_PACKAGE_SELECTED		= 'pkgs',
	MSG_PACKAGE_WORKER_BUSY		= 'pkwb',
	MSG_PACKAGE_WORKER_IDLE		= 'pkwi',
	MSG_ADD_VISIBLE_PACKAGES	= 'avpk',
	MSG_UPDATE_SELECTED_PACKAGE	= 'uspk',
	MSG_CLIENT_TOO_OLD			= 'oldc',
};


#define HD_ERROR_BASE					(B_ERRORS_END + 1)
#define HD_NETWORK_INACCESSIBLE			(HD_ERROR_BASE + 1)
#define HD_CLIENT_TOO_OLD				(HD_ERROR_BASE + 2)
#define HD_ERR_NOT_MODIFIED				(HD_ERROR_BASE + 3)
#define HD_ERR_NO_DATA					(HD_ERROR_BASE + 4)