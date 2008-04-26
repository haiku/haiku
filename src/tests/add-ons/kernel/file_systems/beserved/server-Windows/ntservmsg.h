//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: EVMSG_INSTALLED
//
// MessageText:
//
//  The %1 service was installed.
//
#define EVMSG_INSTALLED                  0x00000064L

//
// MessageId: EVMSG_REMOVED
//
// MessageText:
//
//  The %1 service was removed.
//
#define EVMSG_REMOVED                    0x00000065L

//
// MessageId: EVMSG_NOTREMOVED
//
// MessageText:
//
//  The %1 service could not be removed.
//
#define EVMSG_NOTREMOVED                 0x00000066L

//
// MessageId: EVMSG_CTRLHANDLERNOTINSTALLED
//
// MessageText:
//
//  The control handler could not be installed.
//
#define EVMSG_CTRLHANDLERNOTINSTALLED    0x00000067L

//
// MessageId: EVMSG_FAILEDINIT
//
// MessageText:
//
//  The initialization process failed.
//
#define EVMSG_FAILEDINIT                 0x00000068L

//
// MessageId: EVMSG_STARTED
//
// MessageText:
//
//  The service was started.
//
#define EVMSG_STARTED                    0x00000069L

//
// MessageId: EVMSG_BADREQUEST
//
// MessageText:
//
//  The service received an unsupported request.
//
#define EVMSG_BADREQUEST                 0x0000006AL

//
// MessageId: EVMSG_DEBUG
//
// MessageText:
//
//  Debug: %1
//
#define EVMSG_DEBUG                      0x0000006BL

//
// MessageId: EVMSG_STOPPED
//
// MessageText:
//
//  The service was stopped.
//
#define EVMSG_STOPPED                    0x0000006CL

//
// MessageId: EVMSG_INVALID_SHARE_PATH
//
// MessageText:
//
//  The path '%1' could not be resolved and is therefore not exported.
//
#define EVMSG_INVALID_SHARE_PATH         0x0000006DL

//
// MessageId: EVMSG_SOCKET_INIT_FAILED
//
// MessageText:
//
//  The Windows sockets system could not be initialized.
//
#define EVMSG_SOCKET_INIT_FAILED         0x0000006EL

//
// MessageId: EVMSG_TOO_MANY_SHARES
//
// MessageText:
//
//  There are too many shared folders.  Shared folder '%1' cannot be exported.
//
#define EVMSG_TOO_MANY_SHARES            0x0000006FL

//
// MessageId: EVMSG_DUPLICATE_SHARE
//
// MessageText:
//
//  The shared folder '%1' is multiply defined.
//
#define EVMSG_DUPLICATE_SHARE            0x00000070L

