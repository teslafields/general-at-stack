/* PPP exit codes */
#define SUCCESS     0  // Pppd has detached, or otherwise the connection was successfully established and terminated at the peer's request.
#define FATAL       1  // An immediately fatal error of some kind occurred, such as an essential system call failing, or running out of virtual memory.
#define OPTION      2  // An error was detected in processing the options given, such as two mutually exclusive options being used.
#define PERMISSION  3  // Pppd is not setuid-root and the invoking user is not root.
#define DRIVER      4  // The kernel does not support PPP, for example, the PPP kernel driver is not included or cannot be loaded.
#define SIGNAL      5  // Pppd terminated because it was sent a SIGINT, SIGTERM or SIGHUP signal.
#define LOCKPORT    6  // The serial port could not be locked.
#define OPENPORT    7  // The serial port could not be opened.
#define CONNECTION  8  // The connect script failed (returned a non-zero exit status).
#define COMMAND     9  // The command specified as the argument to the pty option could not be run.
#define NEGOTIATION 10  // The PPP negotiation failed, that is, it didn't reach the point where at least one network protocol (e.g. IP) was running.
#define PREAUTH     11  // The peer system failed (or refused) to authenticate itself.
#define IDLE        12  // The link was established successfully and terminated because it was idle.
#define TIMEOUT     13  // The link was established successfully and terminated because the connect time limit was reached.
#define INCALL      14  // Callback was negotiated and an incoming call should arrive shortly.
#define ECHOREQUEST 15  // The link was terminated because the peer is not responding to echo requests.
#define HANGUP      16  // The link was terminated by the modem hanging up.
#define LOOPBACK    17  // The PPP negotiation failed because serial loopback was detected.
#define INIT        18  // The init script failed (returned a non-zero exit status).
#define POSAUTH     19  // We failed to authenticate ourselves to the peer.

#define PPP_RESOLV "/etc/ppp/resolv.conf"

