/* ipc_sound.c
*
*  SX1 sound server
*
* handles messages sent to modem on channel 6 (sound)
* receives messages from kernel via kernel connector and from userspace via local socket
*
* by Vladimir Ananiev (Vovan888 at gmail com )
*
* Some bits of code from gsmMux daemon Copyright (C) 2003 Tuukka Karvonen <tkarvone@iki.fi>
* Netlink connector stuff is based on ucon.c,  Copyright (c) 2004 Evgeniy Polyakov <johnpol@xxxxxxxxxxx>
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
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/connector.h>
#include <linux/tcp.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
//syslog
#include <syslog.h>

#include "ipc_sound.h"

//#define DEBUG 1

// for debugging
#ifdef DEBUG
#  define SYSLOG(fmt, args...) syslog(LOG_DEBUG, fmt, ## args)
#else
#  define SYSLOG(fmt, args...) /* not debugging: nothing */
#endif


// timeout in seconds
#define IPC_TIMEOUT 5
// number of sockets to poll
#define SOCK_NUM	2

#define uint	unsigned int
#define MODEMDEVICE "/dev/mux5"
#define LSOCKET	"/tmp/sx_sound"	// local socket to listen on
#define MAXMSG	64

#define NETLINK_CONNECTOR   11
#define SOL_NETLINK 270
#define NETLINK_ADD_MEMBERSHIP  1

//static int _debug = 0;	// 1 - print debug messages, do not daemonize
static volatile int wait_for_daemon_status = 0;
static volatile int terminate = 0;
static volatile pid_t the_pid;
int _priority;

static int	fd_mux; // file descriptor for /dev/mux6, should be opened blocking
//-----------------------------------------------------------------
int InitPort(void)
{
	struct termios options;

	fd_mux = open (MODEMDEVICE, O_RDWR | O_NONBLOCK);
	if (fd_mux < 0)
	{
		perror ("ipc_sound: open /dev/mux5 failed!");
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
// waits for reading length bytes from port. with timeout in seconds
//
// frame - buffer to store data
// length - estimated frame length
// timeout - timeout reading data in seconds
// returns:  number of actually read bytes, or -1 if error
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
		if(retval == 0){
			syslog(LOG_DEBUG,"read timeout!");
			return -1;
		}

		if (FD_ISSET (fd_mux, &read_set)) {
			do{
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
//
// frame - buffer to store data
// length - estimated frame length
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
// RDsyReqHandler::Request(TDesC8 const &, TDes8 &)
// it is here - (maybe) sub_5069F4FC
// check err here
int Request(char * request_frame, char * reply_frame)
{
	unsigned short length;
	int len_write, len_read;
	char err;

	length = 6 + *(unsigned short *)(request_frame + 4);
	len_write = Write(request_frame, length);
	SYSLOG("written to serial : %d", len_write);
	if(len_write != length)
		return -1;// Error

// 5069F7AC             ; CDsyRequestDispatcher::Run(void)
	len_read = Read(reply_frame, 6, IPC_TIMEOUT); // read header
	SYSLOG("read from serial port : %d", len_read);
	if(len_read != 6)
		return -1;// Error, we got less then 6 bytes - minimum IPC message
	length = *(unsigned short *)(reply_frame + 4);
	if(length){
		len_read = Read(reply_frame + 6, length, IPC_TIMEOUT); // read the rest of data (if there)
		if(len_read != length)
			return -1;// Error, we got not enough data
		SYSLOG("+=read from serial port : %d", len_read);
	}
	if (reply_frame[0] != IPC_DEST_OMAP )
		return -1; // Error - packet is not for this server
	err = reply_frame[3]; // check error flag
	if ((err != 0x00))
		return -1; // Error - frame with error flag set
	return 0;
}
//-----------------------------------------------------------------
// CIpcHelper::PutHeaderAud(TDes8 &, t_AUD_MsgType, t_AUD_Group, unsigned char, unsigned short, unsigned char)
void PutHeaderAud(char * buf, char AUD_Group, unsigned char cmd, unsigned short length)
{
	buf[0] = 0;  //AUD_MsgType;
	buf[1] = AUD_Group;
	buf[2] = cmd;
	buf[3] = 0;// err;
	*(unsigned short *)(buf + 4) = length - 6;
}
//-----------------------------------------------------------------
// IpcCom::MdaDacOpenDefault(unsigned short, unsigned short)
// r1 -  0=
int	MdaDacOpenDefault(unsigned short freq, unsigned short r2)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_OPEN_DEFAULT, 10);
	*(unsigned short *)(tx_buf + 6) = freq;
	*(unsigned short *)(tx_buf + 8) = r2;
	return Request(tx_buf, rx_buf);
}
//IpcCom::MdaDacOpenRing(unsigned short, unsigned short)
int	MdaDacOpenRing(unsigned short freq, unsigned short r2)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_OPEN_RING, 10);
	*(unsigned short *)(tx_buf + 6) = freq;
	*(unsigned short *)(tx_buf + 8) = r2;
	return Request(tx_buf, rx_buf);
}
// IpcCom::MdaVolumeUpdate(unsigned short)
int	MdaVolumeUpdate(unsigned short volume)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_VOLUME_UPDATE, 8);
	*(unsigned short *)(tx_buf + 6) = volume;
	return Request(tx_buf, rx_buf);
}
// IpcCom::MdaDacClose(void)
int	MdaDacClose(void)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}
//  IpcCom::MdaSetAudioDevice(unsigned short)
int	MdaSetAudioDevice(unsigned short device)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_SETAUDIODEVICE, 8);
	*(unsigned short *)(tx_buf + 6) = device;
	return Request(tx_buf, rx_buf);
}
// IpcCom::MdaPcmPlay(unsigned short)
// 0 -
// 1 -
int	MdaPcmPlay(unsigned short r1)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_PCM, PCM_PLAY, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}
//  IpcCom::MdaPcmRecord(void)
int	MdaPcmRecord(void)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf,IPC_GROUP_PCM, PCM_RECORD, 6);
	return Request(tx_buf, rx_buf);
}
//  IpcCom::MdaPcmClose(void)
int	MdaPcmClose(void)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_PCM, PCM_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}
//IpcCom::MdaPlayTone(unsigned short)
int	MdaPlayTone(unsigned short r1)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_PLAYTONE, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}
// IpcCom::MdaFMRadioOpen(unsigned short)
int	MdaFMRadioOpen(unsigned short r1)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_FMRADIO_OPEN, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}
// IpcCom::MdaFMRadioClose(unsigned short)
int	MdaFMRadioClose(void)
{
	char tx_buf[16]; // data to send
	char rx_buf[16]; // received
	PutHeaderAud( tx_buf, IPC_GROUP_DAC, DAC_FMRADIO_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}
//-----------------------------------------------------------------
// CMdaRadio::SetVolume(int)   // vol = 0..9, 0x0A - close FMradio
void CMdaRadio_SetVolume(int volume)
{
	if((volume >= 0) && (volume < 10))
		MdaFMRadioOpen(volume);
	else
		MdaFMRadioClose();
}
// CMdaSoundPlayer__SetVolume_int_
void CMdaSoundPlayer_SetVolume(int volume)
{
	if((volume > 0) && (volume <= 10))
		MdaVolumeUpdate(volume - 1);
}
//  CMdaSoundPlayer__StartPlayingL_int_
void CMdaSoundPlayer_StartPlaying(int c1)
{
}
//-----------------------------------------------------------------
// Decode message
int	decode_message(unsigned char * msg)
{
	unsigned short	cmd,arg1,arg2;
	int	ret = -1;
	cmd = *(unsigned short *)(msg + 0);
	arg1 = *(unsigned short *)(msg + 2);
	arg2 = *(unsigned short *)(msg + 4);

	SYSLOG("ipc_sound: got message: %02X %02X %02X", cmd,arg1,arg2);

	switch(cmd){
		case 0:	ret = MdaVolumeUpdate(arg1); break;
		case 1:	ret = MdaSetAudioDevice(arg1);break;
		case 2:	ret = MdaDacOpenRing(arg1, arg2);break;
		case 3:	ret = MdaDacOpenDefault(arg1, arg2);break;
		case 4:	ret = MdaDacClose();break;
		case 5:	ret = MdaFMRadioOpen(arg1);break;
		case 6:	ret = MdaFMRadioClose();break;
		case 7:	ret = MdaPlayTone(arg1);break;

		case 8:	ret = MdaPcmPlay(arg1);break;
		case 9:	ret = MdaPcmRecord();break;
		case 0x0A:	ret = MdaPcmClose();break;
	}
	if(ret)
		syslog(LOG_DEBUG,"ipc_sound: error decoding message= %d", ret);

	return ret;
}
//-----------------------------------------------------------------
// Netlink stuff
//-----------------------------------------------------------------

//static int need_exit;
static __u32 seq;

static int netlink_send(int s)
{
	struct nlmsghdr *nlh;
	struct cn_msg *msg;
	unsigned int size;
	char buf[128];
	int err;

	memset(buf, 0, sizeof(buf));

	size = NLMSG_SPACE(sizeof(struct cn_msg));

	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_seq = seq++;
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_type = NLMSG_DONE;
	nlh->nlmsg_len = NLMSG_LENGTH(size - sizeof(*nlh));
	nlh->nlmsg_flags = 0;

	msg = NLMSG_DATA(nlh);

	msg->seq = nlh->nlmsg_seq;

	err = send(s, nlh, size, 0);
	if (err == -1)
		syslog(LOG_DEBUG, "Failed to send to netlink :[%d].", errno);

	return err;
}
/* processes messages from kernel */
int	process_netlink(int fd)
{
	struct nlmsghdr *reply;
	char buf[1024];
	struct cn_msg *data = NULL;
	int	len;

	len = recv(fd, buf, sizeof(buf), 0);
	if (len == -1) {
		syslog(LOG_DEBUG,"recv buf");
		close(fd);
		return -1;
	}

	reply = (struct nlmsghdr *)buf;

	switch (reply->nlmsg_type)
	{
		case NLMSG_ERROR:
			syslog(LOG_DEBUG,"Error message received.");	break;
		case NLMSG_DONE:
			data = (struct cn_msg *)NLMSG_DATA(reply); break;
	}
	if(data && (data->len == 6) ){
		SYSLOG("ipc_sound: message from kernel");
		return decode_message(data->data);
	}
	else
	{
		syslog(LOG_DEBUG,"ipc_sound: message size is not 6");
		return -1;
	}
}

/* processes messages from user programs via TCP socket */
int	process_client(int fd)
{
	unsigned char buffer[MAXMSG];
	int nbytes;
//	int length;	// length of message

	nbytes = read (fd, buffer, MAXMSG);
//	nbytes = recvfrom(fd, buffer, MAXMSG, 0, NULL, NULL);
	if (nbytes <= 0)
	{	/* Read error or end-of-file (connection closed) */
		syslog(LOG_DEBUG, "recvfrom");
		return -1;
	}
	else
	{	/* Data read. */
		if(nbytes == 6){
			SYSLOG( "ipc_sound: message from client");
			return decode_message(buffer);
		}
		else
		{
			syslog(LOG_DEBUG,"ipc_sound: message size is not 6");
			return -1;
		}
	}
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
	fprintf(stderr, "ipc_sound started\n");
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
				fprintf(stderr, "ipc_sound: startup failed. See syslog for details.\n");
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
	int sock_nl, sock_loc;	// File handlers
	int i;	// temp vars
	socklen_t size;

	struct sockaddr_nl sa_netlink;
	struct sockaddr_un sa_loc;
	struct sockaddr_un sa_loc_client;
//	socklen_t sa_loc_client_len;

//	time_t tm;
	int on;
	int opt;
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

	// Open syslog
#ifdef DEBUG
	openlog(programName, LOG_NDELAY | LOG_PID | LOG_PERROR  , LOG_LOCAL0);
	_priority = LOG_DEBUG;
#else
	openlog(programName, LOG_NDELAY | LOG_PID , LOG_LOCAL0 );
	_priority = LOG_INFO;
#endif

	// Init serial port
	InitPort();

#ifdef DEBUG
	syslog(LOG_INFO,
			 "You can quit the ipc_sound daemon with SIGKILL or SIGTERM");
#else
	if (wait_for_daemon_status) {
		kill(parent_pid, SIGHUP);
	}
#endif

//------------------------
	/* -- Netlink socket -- */
	sock_nl = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (sock_nl == -1) {
		perror("netlink socket");
		return -1;
	}

	sa_netlink.nl_family = AF_NETLINK;
	sa_netlink.nl_groups = CN_IDX_SX1SND;
	sa_netlink.nl_pid    = getpid();

	if (bind(sock_nl, (struct sockaddr *)&sa_netlink, sizeof(sa_netlink)))
	{
		perror("netlink bind");
		close(sock_nl);
		return -1;
	}
	on = sa_netlink.nl_groups;
	setsockopt(sock_nl, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &on, sizeof(on));
	fcntl(sock_nl, F_SETFL, O_NONBLOCK);

//------------------------
	/*  -- Local socket --  */
	sock_loc = socket (PF_LOCAL, SOCK_STREAM, 0);
	if (sock_loc == -1) {
		perror("local socket");
		return -1;
	}
	unlink(LSOCKET); /* remove old socket file */
	sa_loc.sun_family	= AF_LOCAL;
	strncpy(sa_loc.sun_path, LSOCKET ,sizeof(sa_loc.sun_path) - 1);
	if(bind (sock_loc, (struct sockaddr *)&sa_loc, sizeof(sa_loc)))  {
		perror("local bind");
		close(sock_loc);
		return -1;
	}
	fcntl(sock_loc, F_SETFL, O_NONBLOCK);
	if (listen (sock_loc, 5) < 0)
	{
		perror ("local listen");
		close(sock_loc);
		return -1;
	}

	/* Disable the Nagle (TCP No Delay) algorithm */
	int	ret, flag = 1;
	ret = setsockopt( sock_loc, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
//------------------------

/* Initialize the timeout data structure. */
	// TODO not used now
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
/* Initialize the file descriptor set. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock_nl,  &active_fd_set);
	FD_SET (sock_loc, &active_fd_set);

/*  ---  Socket processing loop ---*/
	while (!terminate)
	{
	/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			syslog(LOG_DEBUG,"select error = %d",errno);
			exit (EXIT_FAILURE);
 		}

		if (FD_ISSET (sock_nl, &read_fd_set)){
		/* Message from kernel */
			if(process_netlink(sock_nl)){
				syslog(LOG_DEBUG,"error in process_netlink");
			}
			//FD_CLR (i, &active_fd_set);
		}
		/* Service all the sockets with input pending. */
		for (i = 2; i < FD_SETSIZE; ++i)
			if (FD_ISSET (i, &read_fd_set))
			{
				if (i == sock_nl)
					continue;	// skip already processed netlink socket
				if (i == sock_loc)
				{
					/* Connection request on original socket. */
					int new;
					size = sizeof (sa_loc_client);
					new = accept (sock_loc,
							(struct sockaddr *) &sa_loc_client,
							&size);
					if (new < 0) {
						perror ("accept");
						exit (EXIT_FAILURE);
					}
					SYSLOG("Server: connect");
					FD_SET (new, &active_fd_set);
				}
				else
				{
					/* Data arriving on an already-connected socket. */
					if(process_client (i) < 0){
						syslog(LOG_DEBUG,"error in process_client");
						close (i);
						FD_CLR (i, &active_fd_set);
					}
				}
			}
//		if (FD_ISSET (sock_loc, &read_fd_set)){
			/* Message from some user program */
//			if(process_client (sock_loc)){
//				syslog(LOG_DEBUG,"error in process_client");
//			}
//			//FD_CLR (sock_loc, &active_fd_set);
//		}
	}
	/**
	 * close devices
	 */
	closelog();
	close(fd_mux);
	close(sock_nl);
	close(sock_loc);
	return 0;
}
