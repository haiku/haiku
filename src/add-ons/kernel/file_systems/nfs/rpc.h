#ifndef _RPC_H

#define _RPC_H

const int32 RPC_VERSION = 2;

typedef enum rpc_auth_flavor 
{
	RPC_AUTH_NONE       = 0, 
	RPC_AUTH_SYS        = 1, 
	RPC_AUTH_SHORT      = 2 
} rpc_auth_flavor; 
      
typedef enum rpc_msg_type 
{ 
	RPC_CALL  = 0, 
	RPC_REPLY = 1 
} rpc_msg_type; 
      
typedef enum rpc_reply_stat 
{ 
	RPC_MSG_ACCEPTED = 0, 
	RPC_MSG_DENIED   = 1 
} rpc_reply_stat;
      
typedef enum rpc_accept_stat 
{ 
	RPC_SUCCESS       = 0, /* RPC executed successfully             */ 
	RPC_PROG_UNAVAIL  = 1, /* remote hasn't exported program        */ 
	RPC_PROG_MISMATCH = 2, /* remote can't support version #        */ 
	RPC_PROC_UNAVAIL  = 3, /* program can't support procedure       */ 
	RPC_GARBAGE_ARGS  = 4, /* procedure can't decode params         */ 
	RPC_SYSTEM_ERR    = 5  /* errors like memory allocation failure */ 
} rpc_accept_stat; 

typedef enum rpc_reject_stat 
{ 
	RPC_RPC_MISMATCH = 0, /* RPC version number != 2          */ 
	RPC_AUTH_ERROR = 1    /* remote can't authenticate caller */ 
} rpc_reject_stat; 

typedef enum rpc_auth_stat 
{ 
	RPC_AUTH_OK           = 0,  /* success                          */ 
	/* 
	 * failed at remote end 
	 */ 
	RPC_AUTH_BADCRED      = 1,  /* bad credential (seal broken)     */ 
	RPC_AUTH_REJECTEDCRED = 2,  /* client must begin new session    */ 
	RPC_AUTH_BADVERF      = 3,  /* bad verifier (seal broken)       */ 
	RPC_AUTH_REJECTEDVERF = 4,  /* verifier expired or replayed     */ 
	RPC_AUTH_TOOWEAK      = 5,  /* rejected for security reasons    */ 
	/* 
	 * failed locally 
	 */ 
	RPC_AUTH_INVALIDRESP  = 6,  /* bogus response verifier          */ 
	RPC_AUTH_FAILED       = 7   /* reason unknown                   */ 
} rpc_auth_stat; 

#endif
