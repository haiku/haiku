#ifndef SERVICES_DAEMON_APP_H
#define SERVICES_DAEMON_APP_H

#define B_SERVICES_DAEMON_SIGNATURE "application/x-vnd.Haiku-ServicesDaemon"

// Send this message to the daemon if you would like to have your program
// restarted. The message is expected to have an attached string containing
// the signature of your app. Once sent to the daemon, it will wait until
// your app quits before relaunching it.
#define B_SERVICES_DAEMON_RESTART 'SDRS'

#endif
