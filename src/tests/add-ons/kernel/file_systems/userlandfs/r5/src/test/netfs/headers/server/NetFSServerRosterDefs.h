// NetFSServerRosterDefs.h

#ifndef NET_FS_SERVER_ROSTER_DEFS_H
#define NET_FS_SERVER_ROSTER_DEFS_H

extern const char* kNetFSServerSignature;

// message what field values
enum {
	NETFS_REQUEST_GET_MESSENGER			= 'nfgm',

	NETFS_REQUEST_ADD_USER				= 'nfau',
	NETFS_REQUEST_REMOVE_USER			= 'nfru',
	NETFS_REQUEST_GET_USERS				= 'nfgu',
	NETFS_REQUEST_GET_USER_STATISTICS	= 'nfus',

	NETFS_REQUEST_ADD_SHARE				= 'nfas',
	NETFS_REQUEST_REMOVE_SHARE			= 'nfrs',
	NETFS_REQUEST_GET_SHARES			= 'nfgs',
	NETFS_REQUEST_GET_SHARE_USERS		= 'nfsu',
	NETFS_REQUEST_GET_SHARE_STATISTICS	= 'nfss',

	NETFS_REQUEST_SET_USER_PERMISSIONS	= 'nfsp',
	NETFS_REQUEST_GET_USER_PERMISSIONS	= 'nfgp',

	NETFS_REQUEST_SAVE_SETTINGS			= 'nfse',
};

/*
	Protocol
	========

	Common
	------

	reply:
		"error":		int32


	NETFS_REQUEST_GET_MESSENGER
	---------------------------

	reply:
		"messenger":	messenger


	NETFS_REQUEST_ADD_USER
	----------------------

	request:
		"user":			string
		[ "password":	string ]


	NETFS_REQUEST_REMOVE_USER
	-------------------------

	request:
		"user":			string


	NETFS_REQUEST_GET_USERS
	-----------------------

	reply:
		"users":		message ( "users":	string[] )


	NETFS_REQUEST_GET_USER_STATISTICS
	---------------------------------

	request:
		"user":			string

	reply:
		"statistics":	message ( not defined yet )


	NETFS_REQUEST_ADD_SHARE
	-----------------------

	request:
		"share":		string
		"path":			string


	NETFS_REQUEST_REMOVE_SHARE
	--------------------------

	request:
		"share":		string


	NETFS_REQUEST_GET_SHARES
	------------------------

	reply:
		"shares":		message ( "shares":	string[]
								  "paths":	string[] )


	NETFS_REQUEST_GET_SHARE_USERS
	-----------------------------

	request:
		"share":		string

	reply:
		"users":		message ( "users":	string[] )


	NETFS_REQUEST_GET_SHARE_STATISTICS
	----------------------------------

	request:
		"share":		string

	reply:
		"statistics":	message ( not defined yet )


	NETFS_REQUEST_SET_USER_PERMISSIONS
	----------------------------------

	request:
		"share":		string
		"user":			string
		"permissions":	int32


	NETFS_REQUEST_GET_USER_PERMISSIONS
	----------------------------------

	request:
		"share":		string
		"user":			string

	reply:
		"permissions":	int32


	NETFS_REQUEST_SAVE_SETTINGS
	---------------------------

	request: <empty>

*/

#endif	// NET_FS_SERVER_ROSTER_DEFS_H
