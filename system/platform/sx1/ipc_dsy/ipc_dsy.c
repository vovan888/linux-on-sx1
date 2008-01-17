/* ipc_dsy.c
*
* SX1 indication server
*
* by Vladimir Ananiev (Vovan888 at gmail com )
*
*
* handles messages from modem. on channel 5 (AMUX::105)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <asm/types.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/select.h>
//#include <linux/netlink.h>
//#include <linux/rtnetlink.h>
//#include <linux/connector.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
//syslog
#include <syslog.h>

#include <arch/sx1/ipc_dsy.h>

//#define DEBUG 1

// for debugging
#ifdef DEBUG
#  define SYSLOG(fmt, args...) syslog(LOG_DEBUG, fmt, ## args)
#else
#  define SYSLOG(fmt, args...) /* not debugging: nothing */
#endif

// timeout in seconds
#define IPC_TIMEOUT 3
#define uint	unsigned int
#define MODEMDEVICE "/dev/mux4"
#define LSOCKET	"/tmp/sx_dsy"	// local socket to listen on
#define MAXMSG	64

//static int _debug = 0;	// 1 - print debug messages, do not daemonize
static int wait_for_daemon_status = 0;
static volatile int terminate = 0;
static pid_t the_pid;
int _priority;

static int	fd_mux; // file descriptor for /dev/mux4, should be opened blocking

// array of socket descriptors of connected clients
#define	MAX_CLIENTS	 8
static int	clients_number = 0;
static int	clients[MAX_CLIENTS];

//-----------------------------------------------------------------
int InitPort(void)
{
	struct termios options;

	fd_mux = open (MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd_mux < 0)
	{
		perror ("Error: open /dev/mux4 failed!");
		exit (-1);
	}
// set port settings
	cfmakeraw(&options);
	options.c_iflag = IGNBRK;
	options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL); // Enable the receiver and set local mode and 8N1
	options.c_cflag |= CRTSCTS; // enable hardware flow control (CNEW_RTCCTS)
	options.c_cflag |= B38400; // Set speed
	options.c_lflag = 0; // set raw input
	options.c_oflag = 0; // set raw output
// Set the new options for the port...
	tcsetattr (fd_mux, TCSANOW, &options);
	return 0;
}
//-----------------------------------------------------------------
// waits for reading length bytes from port. with timeout
int Read(char * frame, int length, int readtimeout)
{
	fd_set all_set,read_set;
	struct timeval timeoutz;
	int	act_read = 0, retval;

	if((frame==NULL) || (length <= 0))
		return -1;

	/* Initialize the timeout data structure. */
	timeoutz.tv_sec = readtimeout;
	timeoutz.tv_usec = 0;

	/* Initialize the file descriptor set. */
	FD_ZERO (&all_set);
	FD_SET (fd_mux,  &all_set);

	do {
		read_set = all_set;
		if ((retval = select (FD_SETSIZE, &read_set, NULL, NULL, &timeoutz)) < 0) {
			syslog(LOG_DEBUG,"read select error = %d",errno);
			return -1;
//			exit (EXIT_FAILURE);
		}

		if(retval == 0) {
			syslog(LOG_DEBUG,"read timeout!");
			return -1;
		}

		if (FD_ISSET (fd_mux, &read_set)) {
			do {
				retval = read(fd_mux, frame + act_read, length - act_read);
				if(retval >= 0)
					act_read += retval;
				else
					return -1;
			} while(act_read < length);
			return act_read;
		}
	}while(1);
}
//-----------------------------------------------------------------
// write frame to serial port fd_mux
int	Write(char * frame, int length)
{
	int written = 0, retval;
	if((frame==NULL) || (length <= 0))
		return -1;
	do {
		retval = write(fd_mux, frame + written, length - written);
		if(retval > 0)
			written += retval;
		else
			return -1;
	} while(written < length);
	return written;
}
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// 5069C80C             ; IpcHelper::PutHeader
void PutHeader(char * buf, char IPC_Group, unsigned char cmd, unsigned short length)
{
	buf[0] = IPC_DEST_ACCESS;
	buf[1] = IPC_Group;
	buf[2] = cmd;
	buf[3] = 0x30;
	*(unsigned short *)(buf + 4) = length - 6;
}

// CDsyIndicationHandler::HandleRagbag(TIpcMsgHdr *, TPtr8 &)
int HandleRagbag(char * buf, char * res)
{
	char cmd;
	char c1, c2;
	unsigned short data16;
	int		data32;

	cmd = buf[2];
	switch(cmd) {
		case 0: // CDsyIndicationHandler::NotifyAudioLinkOpenReq(TPtr8 &)
				// CBtAudioStateMachine::ModemAudioOpenReq(void);
			break;
		case 1: // CDsyIndicationHandler::NotifyAudioLinkCloseReq(TPtr8 &)
				// CBtAudioStateMachine::ModemAudioCloseReq(void)
			break;
		case 5: // CDsyIndicationHandler::SetLightSensorSettings(TPtr8 &)
				data16 = *(unsigned short *)(buf+6); // unused ???
			break;
		case 9: // CDsyIndicationHandler::SetTempSensorValues(TPtr8 &)
				c1 = buf[6]; // unused ???
				c2 = buf[7]; // unused ???
			break;
		case 6: // CDsyIndicationHandler::NotifyEmailMessage(TPtr8 &)
			data32 = *(unsigned char *)(buf+6);
			// CDosEventManager::EmailMessage(int data32)
			break;
		case 7: //CDsyIndicationHandler::NotifyFaxMessage(TPtr8 &)
			data32 = *(unsigned char *)(buf+6);
			//  CDosEventManager::FaxMessage(int data32)
			break;
		case 8: // CDsyIndicationHandler::NotifyVoiceMailStatus(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDosEventManager::VoiceMailStatus(TSAVoiceMailStatus)  data16
			break;
		case 11: // CDsyIndicationHandler::NotifyCallsForwardingStatus(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDosEventManager::CallsForwardingStatus(TSACallsForwardingStatus)  data16
			break;
		case 0x13: // CDsyIndicationHandler::BacklightTempHandle(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// RBusLogicalChannel::DoControl(int, void *) 8, &data16
			break;
		case 0x14: // CDsyIndicationHandler::NotifyCarKitIgnition(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDsyFactory::CarKitIgnitionInd(unsigned short)  data16
			break;
		default:
			return -1;
	}
	return 0;
}
// CDsyIndicationHandler::HandleSim(TIpcMsgHdr *, TPtr8 &)
int HandleSim(char * buf, char * res)
{
	char cmd;
	unsigned short data16;
	int		data32;

	cmd = buf[2];
	switch(cmd) {
		case 0: // CDsyIndicationHandler::NotifySimState(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDosEventManager::SimState(TDosSimState)  data16
				if (data16 == 7){
					// CDsyFactory::SetSimChangedFlag(CDsyFactory::TSimChangedFlag, int)  0, 1
				}
			break;
		case 2: // CDsyIndicationHandler::NotifySimChanged(TPtr8 &)
			data32 = *(unsigned char *)(buf+6);
				// CDosEventManager::SimChanged(int)  data32
			break;
		case 3: // CDsyIndicationHandler::NotifyCurrentOwnedSimStatus(TPtr8 &)
			data32 = *(unsigned char *)(buf+6);
				// CDosEventManager::CurrentSimOwnedStatus(TSACurrentSimOwnedSimStatus) data32
			break;
		case 5: // CDsyIndicationHandler::NotifySimLockStatus(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDosEventManager::SimLockStatus(TSASimLockStatus)  1
			break;
		default:
			return -1;
	}
	return 0;
}
// CDsyIndicationHandler::HandleBattery(TIpcMsgHdr *, TPtr8 &)
int HandleBattery(char * buf, char * res)
{
	char cmd;
	char c1;
	unsigned short data16;

	cmd = buf[2]
	switch( cmd ) {
		case 0: // CDsyIndicationHandler::NotifyChargingState(TPtr8 &)
			data16 = *(unsigned short *)(buf+6);
			// CDosEventManager::ChargingState(TDosChargingState)  data16
			break;
		case 1: // CDsyIndicationHandler::NotifyBatteryStatus(TPtr8 &)
			data16 = *(unsigned short *)(buf+6);
			// CDosEventManager::BatteryStatus(TDosBatteryStatus)  c1
			break;
		case 2: // CCDsyIndicationHandler::NotifyBatteryBars(TPtr8 &)
			c1 = *(unsigned char *)(buf+6);
			//CDosEventManager::BatteryBars(int)  c1
			break;
		case 3: // CDsyIndicationHandler::NotifyBatteryLowWarning(TPtr8 &)
			c1 = *(unsigned char *)(buf+6);
			// CDosEventManager::BatteryLowWarning(int)  c1
			break;
		default:
			return -1;
	}

	return 0;
}
// 50699AC0             ; CDsyIndicationHandler::HandleIndication(TIpcMsgHdr *, TPtr8 &, TPtr8 &)
// decodes incoming package and puts response into res
int HandleIndication(char * buf, char * res)
{
	char IPC_Group = buf[1];
	char cmd;
	char err = 0;
	unsigned short data16;

	if(IPC_Group > 10)
		return -2;	// error
	err = buf[3];
	if( (err != 0x30) && (err != 0xFF) )
		return -1;
	switch(IPC_Group) {
		case IPC_GROUP_RAGBAG: // CDsyIndicationHandler::HandleRagbag(TIpcMsgHdr *, TPtr8 &)
			return HandleRagbag(buf, res);
		case IPC_GROUP_SIM: // CDsyIndicationHandler::HandleSim(TIpcMsgHdr *, TPtr8 &)
			return HandleSim(buf, res);
		case IPC_GROUP_BAT: // CDsyIndicationHandler::HandleBattery(TIpcMsgHdr *, TPtr8 &)
			return HandleBattery(buf, res);
		case IPC_GROUP_RSSI: // CDsyIndicationHandler::HandleRssi(TIpcMsgHdr *, TPtr8 &)
			cmd = buf[2];
			if(cmd == 0) { // CDsyIndicationHandler::NotifyNetworkBars(TPtr8 &)
				data16 = *(unsigned short *)(buf+6);
				// CDosEventManager::NetworkBars(int)  data16
				return 0;
			}
			return -1;
		case IPC_GROUP_POWERUP: // CDsyIndicationHandler::HandlePowerup(TIpcMsgHdr *, TPtr8 &)
			cmd = buf[2];
			if(cmd == 1) { // CDsyIndicationHandler::SendPingResponse(void)
				return 0;
			}
			return -1;
		case 0x0A: //  CDsyIndicationHandler::HandlePowerdown(TIpcMsgHdr *, TPtr8 &)
			cmd = buf[2];
			if(cmd == 1) { // CDsyIndicationHandler::NotifyShutdown(void)
				// many things...
				// many things...
				return 0;
			}
			return -1;
		default:
			return -1;
	}
}
//-----------------------------------------------------------------
/* Send ipc message from modem to all connected clients (spam :) ) */
void	send_spam(char * modem_message)
{
	int	i, written;
	int	length = 6 + *(unsigned short *)(modem_message + 4);

	if(!clients_number)
		return;
	SYSLOG("sending to clients");
	for(i = 0; i < clients_number; i++) {
		if( clients[i] > 0) {
			/* send 4 bytes with data length */
			written = write( clients[i], &length, sizeof(length) );
			if(written == -1) {
				/* client closed connection */
				clients[i] = 0;
				continue;
			}
			/* send actual message data */
			written = write( clients[i], modem_message, length);
			if(written == -1) {
				clients[i] = 0;
				continue;
			}
			SYSLOG("sent to client");
		}
	}
}
//-----------------------------------------------------------------
/* processes messages from modem on serial port */
int	process_modem(int fd)
{
	char buffer_in[MAXMSG];
	char buffer_out[MAXMSG];
	int err, group, cmd;
	int length; // message length
	int len_read, len_write;

	SYSLOG("from modem");
	/* read first 6 bytes of message (header) */
	len_read = Read(buffer_in, 6, IPC_TIMEOUT);
	if(len_read != 6)
		return -1;// Error

	length = *(unsigned short *)(buffer_in + 4);
	if(length) {
		/* read the rest of data (if exists) */
		len_read = Read(buffer_in + 6, length, IPC_TIMEOUT);
		if(len_read != length)
			return -1;// Error
	}
	group = buffer_in[1];
	cmd = buffer_in[2];
	SYSLOG("got message: %02X %02X %02X",group, cmd, buffer_in[6]);

	if (buffer_in[0] != IPC_DEST_DSY )
		return -1; /* Error - packet is not for us */

	err = buffer_in[3];
	if ((err != 0x30) && (err != 0xFF) )
		return -1; /* Error - wrong packet */

	if( HandleIndication(buffer_in, buffer_out) == 0 ) {
//	if( 1 ) {
		/* send  multicast message to clients */
		send_spam( buffer_in );
		/* send ACK response to modem */
		PutHeader( buffer_out, buffer_in[1], buffer_in[2], 6 );
		length = 6;	// + *(unsigned short *)(buffer_out + 4);
		len_write = Write(buffer_out, length);
		if(len_write != length)
			return -1;
	}
	else
		return -1;

	return 0;
}
/* shows how to use this program
 */
void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);
	fprintf(stderr,"options:\n");
	fprintf(stderr," -d : Debug mode, don't fork\n");
	fprintf(stderr," -w : Wait for deamon startup success/failure\n");
	fprintf(stderr," -h : Show this help message\n");
}

/* Wait for child process to kill the parent.
 */
void parent_signal_treatment(int param) {
	fprintf(stderr, "ipc_dsy started\n");
	exit(0);
}

/*
 * Daemonize process, this process  create the daemon
 */
int daemonize(void)
{
#ifndef DEBUG
	signal(SIGHUP, parent_signal_treatment);
	if((the_pid=fork()) < 0) {
		wait_for_daemon_status = 0;
		return(-1);
	} else
		if(the_pid!=0) {
			if (wait_for_daemon_status) {
				wait(NULL);
				fprintf(stderr, "ipc_dsy startup failed. See syslog for details.\n");
				exit(1);
			}
			exit(0);//parent goes bye-bye
		}
	//child continues
		setsid();   //become session leader
	//signal(SIGHUP, SIG_IGN);
		if(wait_for_daemon_status == 0 && (the_pid = fork()) != 0)
			exit(0);
		chdir("/tmp"); //change working directory
		umask(0);// clear our file mode creation mask

// Close out the standard file descriptors
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
#endif
	//daemonize process stop here
	return 0;
}

/**
 * Function responsible by all signal handlers treatment
 * any new signal must be added here
 */
void signal_treatment(int param)
{
	switch(param)
	{
		case SIGPIPE:
			exit(0);
			break;
		case SIGHUP:
			//reread the configuration files
			break;
		case SIGINT:
			//exit(0);
			terminate = 1;
			break;
		case SIGKILL:
			//kill immediatly
			//i'm not sure if i put exit or sustain the terminate attribution
			terminate = 1;
			//exit(0);
			break;
		case SIGUSR1:
			terminate  = 1;
			//sig_term(param);
		case SIGTERM:
			terminate = 1;
			break;
		default:
			exit(0);
			break;
	}
}


//-----------------------------------------------------------------
int main(int argc, char *argv[])
{
	int opt;
	struct sockaddr_un sa_loc;
	struct sockaddr_un sa_loc_client;
	int  sock_loc;	// socket handler
	fd_set active_fd_set, read_fd_set;
	struct timeval timeout;
	char * programName = argv[0];
	pid_t parent_pid;

	while((opt=getopt(argc,argv,"h?"))>0)
	{
		switch(opt)
		{
			case '?' :
			case 'h' :
				usage(programName);
				exit(0);
				break;
            case 'w':
                wait_for_daemon_status = 1;
                break;
			default:
				break;
		}
	}
//------------------------
	InitPort();	// Init serial port
//------------------------
	//DAEMONIZE
	//SHOW TIME
	parent_pid = getpid();
	daemonize();
	//The Hell is from now-one

	/* SIGNALS treatment*/
	signal(SIGHUP, signal_treatment);
	signal(SIGPIPE, signal_treatment);
	signal(SIGKILL, signal_treatment);
	signal(SIGINT, signal_treatment);
	signal(SIGUSR1, signal_treatment);
	signal(SIGTERM, signal_treatment);

	programName = argv[0];
#ifdef DEBUG
	openlog(programName, LOG_NDELAY | LOG_PID | LOG_PERROR  , LOG_LOCAL0);
	_priority = LOG_DEBUG;
#else
	openlog(programName, LOG_NDELAY | LOG_PID , LOG_LOCAL0 );
	_priority = LOG_INFO;
#endif

#ifdef DEBUG
	syslog(LOG_INFO,
		 "You can quit the ipc_dsy daemon with SIGKILL or SIGTERM");
#else
	if (wait_for_daemon_status) {
		kill(parent_pid, SIGHUP);
	}
#endif

//------------------------
	/*  -- Local socket init --  */
	sock_loc = socket (PF_LOCAL, SOCK_STREAM, 0);
	if (sock_loc == -1) {
		perror("socket");
		return -1;
	}
	unlink(LSOCKET); /* remove old socket file */
	sa_loc.sun_family	= AF_LOCAL;
	strncpy(sa_loc.sun_path, LSOCKET ,sizeof(sa_loc.sun_path) - 1);
	if(bind (sock_loc, (struct sockaddr *)&sa_loc, sizeof(sa_loc)))  {
		perror("bind");
		close(sock_loc);
		return -1;
	}
	fcntl(sock_loc, F_SETFL, O_NONBLOCK);
	if (listen (sock_loc, 5) < 0)
	{
		perror ("listen");
		close(sock_loc);
		return -1;
	}

//-----------------------------------------------------
	/* Initialize the timeout data structure. */
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	/* Initialize the file descriptor set. */
	FD_ZERO (&active_fd_set);
	FD_SET (fd_mux  , &active_fd_set);
	FD_SET (sock_loc, &active_fd_set);

	/*  ---  descriptor processing loop ---*/
	while (!terminate)
	{
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			syslog(LOG_DEBUG,"select error = %d",errno);
			exit (EXIT_FAILURE);
		}

		if (FD_ISSET (fd_mux, &read_fd_set)){
		/* Message from modem */
			if(process_modem(fd_mux)){
				syslog(LOG_DEBUG,"error in process_modem");
			}
			//FD_CLR (i, &active_fd_set);
		}
		if (FD_ISSET (sock_loc, &read_fd_set))
		{
			/* Connection request on original socket. */
			int newsock, size;
			size = sizeof (sa_loc_client);
			newsock = accept (sock_loc,	(struct sockaddr *) &sa_loc_client,	&size);
			if (newsock < 0) {
				perror ("accept");
				exit (EXIT_FAILURE);
			}

			// add connected socket to list
			if(clients_number < 7){
				clients[clients_number++] = newsock;
				SYSLOG("client accepted");
				FD_SET (newsock, &active_fd_set);
			}
			else
				SYSLOG("connection refused - max clients connected");

		}
	}
	closelog();
	close(fd_mux);
	close(sock_loc);
	return 0;
}
