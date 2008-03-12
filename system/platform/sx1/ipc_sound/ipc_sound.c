/* ipc_sound.c
*
*  SX1 sound server
*
* handles messages sent to modem on channel 6 (sound)
* receives messages from kernel via kernel connector and from userspace via local socket
*
* Copyright 2007,2008 by Vladimir Ananiev (Vovan888 at gmail com )
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
//DBGMSG
#include <syslog.h>

#include <arch/sx1/ipc_sound.h>
#include <ipc/colosseum.h>
#include <ipc/shareddata.h>
#include <debug.h>

// timeout in seconds
#define IPC_TIMEOUT 5
// number of sockets to poll
#define SOCK_NUM	2

#define MODEMDEVICE "/dev/mux5"
#define MAXMSG	64

#define NETLINK_CONNECTOR   11
#define SOL_NETLINK 270
#define NETLINK_ADD_MEMBERSHIP  1

static volatile int terminate = 0;
int _priority;

/* IPC interface */
struct SharedSystem *shdata;	/* shared memory segment */
static int ipc_fd;		/* IPC file descriptor */

static int fd_mux;		// file descriptor for /dev/mux6, should be opened blocking
//-----------------------------------------------------------------
int sound_init_serial(void)
{
	struct termios options;

	fd_mux = open(MODEMDEVICE, O_RDWR | O_NONBLOCK);
	if (fd_mux < 0) {
		perror("ipc_sound: open /dev/mux5 failed!");
		exit(-1);
	}
// set port settings
	cfmakeraw(&options);
	options.c_iflag = IGNBRK;
	// Enable the receiver and set local mode and 8N1
	options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL);
	options.c_cflag |= CRTSCTS;	// enable hardware flow control (CNEW_RTCCTS)
	options.c_cflag |= B38400;	// Set speed
	options.c_lflag = 0;	// set raw input
	options.c_oflag = 0;	// set raw output
// Set the new options for the port...
	tcsetattr(fd_mux, TCSANOW, &options);
	return 0;
}

//-----------------------------------------------------------------
// waits for reading length bytes from port. with timeout in seconds
//
// frame - buffer to store data
// length - estimated frame length
// timeout - timeout reading data in seconds
// returns:  number of actually read bytes, or -1 if error
int Read(char *frame, int length, int readtimeout)
{
	fd_set all_set, read_set;
	struct timeval timeoutz;
	int act_read = 0, retval;

	if ((frame == NULL) || (length <= 0))
		return -1;
	/* Initialize the timeout data structure. */
	timeoutz.tv_sec = readtimeout;
	timeoutz.tv_usec = 0;
	/* Initialize the file descriptor set. */
	FD_ZERO(&all_set);
	FD_SET(fd_mux, &all_set);
	do {
		read_set = all_set;
		if ((retval =
		     select(FD_SETSIZE, &read_set, NULL, NULL,
			    &timeoutz)) < 0) {
			DBGMSG("read select error = %d", errno);
			return -1;
//                      exit (EXIT_FAILURE);
		}
		if (retval == 0) {
			DBGMSG("read timeout!");
			return -1;
		}

		if (FD_ISSET(fd_mux, &read_set)) {
			do {
				retval =
				    read(fd_mux, frame + act_read,
					 length - act_read);
				if (retval >= 0)
					act_read += retval;
				else
					return -1;
			} while (act_read < length);
			return act_read;
		}
	} while (1);
}

//-----------------------------------------------------------------
// write frame to serial port fd_mux
//
// frame - buffer to store data
// length - estimated frame length
int Write(char *frame, int length)
{
	int written = 0, retval;
	if ((frame == NULL) || (length <= 0))
		return -1;
	do {
		retval = write(fd_mux, frame + written, length - written);
		if (retval > 0)
			written += retval;
		else
			return -1;
	} while (written < length);
	return written;
}

//-----------------------------------------------------------------
// RDsyReqHandler::Request(TDesC8 const &, TDes8 &)
// it is here - (maybe) sub_5069F4FC
// check err here
int Request(char *request_frame, char *reply_frame)
{
	unsigned short length;
	int len_write, len_read;
	char err;

	length = 6 + *(unsigned short *)(request_frame + 4);
	len_write = Write(request_frame, length);
	DBGMSG("written to serial : %d", len_write);
	if (len_write != length)
		return -1;	// Error

// 5069F7AC             ; CDsyRequestDispatcher::Run(void)
	len_read = Read(reply_frame, 6, IPC_TIMEOUT);	// read header
	DBGMSG("read from serial port : %d", len_read);
	if (len_read != 6)
		return -1;	// Error, we got less then 6 bytes - minimum IPC message
	length = *(unsigned short *)(reply_frame + 4);
	if (length) {
		len_read = Read(reply_frame + 6, length, IPC_TIMEOUT);	// read the rest of data (if there)
		if (len_read != length)
			return -1;	// Error, we got not enough data
		DBGMSG("+=read from serial port : %d", len_read);
	}
	if (reply_frame[0] != IPC_DEST_OMAP)
		return -1;	// Error - packet is not for this server
	err = reply_frame[3];	// check error flag
	if ((err != 0x00))
		return -1;	// Error - frame with error flag set
	return 0;
}

//-----------------------------------------------------------------
// CIpcHelper::PutHeaderAud(TDes8 &, t_AUD_MsgType, t_AUD_Group, unsigned char, unsigned short, unsigned char)
void PutHeaderAud(char *buf, char AUD_Group, unsigned char cmd,
		  unsigned short length)
{
	buf[0] = 0;		//AUD_MsgType;
	buf[1] = AUD_Group;
	buf[2] = cmd;
	buf[3] = 0;		// err;
	*(unsigned short *)(buf + 4) = length - 6;
}

//-----------------------------------------------------------------
// IpcCom::MdaDacOpenDefault(unsigned short, unsigned short)
// r1 -  0=
int MdaDacOpenDefault(unsigned short freq, unsigned short r2)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_OPEN_DEFAULT, 10);
	*(unsigned short *)(tx_buf + 6) = freq;
	*(unsigned short *)(tx_buf + 8) = r2;
	return Request(tx_buf, rx_buf);
}

//IpcCom::MdaDacOpenRing(unsigned short, unsigned short)
int MdaDacOpenRing(unsigned short freq, unsigned short r2)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_OPEN_RING, 10);
	*(unsigned short *)(tx_buf + 6) = freq;
	*(unsigned short *)(tx_buf + 8) = r2;
	return Request(tx_buf, rx_buf);
}

// IpcCom::MdaVolumeUpdate(unsigned short)
int MdaVolumeUpdate(unsigned short volume)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_VOLUME_UPDATE, 8);
	*(unsigned short *)(tx_buf + 6) = volume;
	return Request(tx_buf, rx_buf);
}

// IpcCom::MdaDacClose(void)
int MdaDacClose(void)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}

//  IpcCom::MdaSetAudioDevice(unsigned short)
int MdaSetAudioDevice(unsigned short device)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_SETAUDIODEVICE, 8);
	*(unsigned short *)(tx_buf + 6) = device;
	return Request(tx_buf, rx_buf);
}

// IpcCom::MdaPcmPlay(unsigned short)
// 0 -
// 1 -
int MdaPcmPlay(unsigned short r1)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_PCM, PCM_PLAY, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}

//  IpcCom::MdaPcmRecord(void)
int MdaPcmRecord(void)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_PCM, PCM_RECORD, 6);
	return Request(tx_buf, rx_buf);
}

//  IpcCom::MdaPcmClose(void)
int MdaPcmClose(void)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_PCM, PCM_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}

//IpcCom::MdaPlayTone(unsigned short)
int MdaPlayTone(unsigned short r1)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_PLAYTONE, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}

// IpcCom::MdaFMRadioOpen(unsigned short)
int MdaFMRadioOpen(unsigned short r1)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_FMRADIO_OPEN, 8);
	*(unsigned short *)(tx_buf + 6) = r1;
	return Request(tx_buf, rx_buf);
}

// IpcCom::MdaFMRadioClose(unsigned short)
int MdaFMRadioClose(void)
{
	char tx_buf[16];	// data to send
	char rx_buf[16];	// received
	PutHeaderAud(tx_buf, IPC_GROUP_DAC, DAC_FMRADIO_CLOSE, 6);
	return Request(tx_buf, rx_buf);
}

//-----------------------------------------------------------------
// CMdaRadio::SetVolume(int)   // vol = 0..9, 0x0A - close FMradio
void CMdaRadio_SetVolume(int volume)
{
	if ((volume >= 0) && (volume < 10))
		MdaFMRadioOpen(volume);
	else
		MdaFMRadioClose();
}

// CMdaSoundPlayer__SetVolume_int_
void CMdaSoundPlayer_SetVolume(int volume)
{
	if ((volume > 0) && (volume <= 10))
		MdaVolumeUpdate(volume - 1);
}

//  CMdaSoundPlayer__StartPlayingL_int_
void CMdaSoundPlayer_StartPlaying(int c1)
{
}

//-----------------------------------------------------------------
// Decode message
int decode_message(unsigned char *msg)
{
	unsigned short cmd, arg1, arg2;
	int ret = -1;
	cmd = *(unsigned short *)(msg + 0);
	arg1 = *(unsigned short *)(msg + 2);
	arg2 = *(unsigned short *)(msg + 4);

	DBGMSG("ipc_sound: got message: %02X %02X %02X", cmd, arg1, arg2);

	switch (cmd) {
	case 0:
		ret = MdaVolumeUpdate(arg1);
		break;
	case 1:
		ret = MdaSetAudioDevice(arg1);
		break;
	case 2:
		ret = MdaDacOpenRing(arg1, arg2);
		break;
	case 3:
		ret = MdaDacOpenDefault(arg1, arg2);
		break;
	case 4:
		ret = MdaDacClose();
		break;
	case 5:
		ret = MdaFMRadioOpen(arg1);
		break;
	case 6:
		ret = MdaFMRadioClose();
		break;
	case 7:
		ret = MdaPlayTone(arg1);
		break;

	case 8:
		ret = MdaPcmPlay(arg1);
		break;
	case 9:
		ret = MdaPcmRecord();
		break;
	case 0x0A:
		ret = MdaPcmClose();
		break;
	}
	if (ret)
		DBGMSG("ipc_sound: error decoding message= %d", ret);

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
		DBGMSG("Failed to send to netlink :[%d].", errno);

	return err;
}

/* processes messages from kernel */
int process_netlink(int fd)
{
	struct nlmsghdr *reply;
	char buf[1024];
	struct cn_msg *data = NULL;
	int len;

	len = recv(fd, buf, sizeof(buf), 0);
	if (len == -1) {
		DBGMSG("recv buf");
		close(fd);
		return -1;
	}

	reply = (struct nlmsghdr *)buf;

	switch (reply->nlmsg_type) {
	case NLMSG_ERROR:
		DBGMSG("Error message received.");
		break;
	case NLMSG_DONE:
		data = (struct cn_msg *)NLMSG_DATA(reply);
		break;
	}
	if (data && (data->len == 6)) {
		DBGMSG("ipc_sound: message from kernel");
		return decode_message(data->data);
	} else {
		DBGMSG("ipc_sound: message size is not 6");
		return -1;
	}
}

/* processes messages from user programs via TCP socket */
int process_client(int fd)
{
	unsigned char buffer[MAXMSG];
	int nbytes;
//      int length;     // length of message

	nbytes = read(fd, buffer, MAXMSG);
//      nbytes = recvfrom(fd, buffer, MAXMSG, 0, NULL, NULL);
	if (nbytes <= 0) {	/* Read error or end-of-file (connection closed) */
		DBGMSG("recvfrom");
		return -1;
	} else {		/* Data read. */
		if (nbytes == 6) {
			DBGMSG("ipc_sound: message from client");
			return decode_message(buffer);
		} else {
			DBGMSG("ipc_sound: message size is not 6");
			return -1;
		}
	}
}

/*-----------------------------------------------------------------*/
/* Handle IPC message
*/
int ipc_handle(int fd)
{
	int ack = 0, size = 64;
	unsigned short src = 0;
	unsigned char msg_buf[64];

	DBGMSG("ipc_sound: ipc_handle\n");

	if ((ack = ClGetMessage(&msg_buf, &size, &src)) < 0)
		return ack;

	if (ack == CL_CLIENT_BROADCAST) {
		/* handle broadcast message */
	}

	/*      if (IS_GROUP_MSG(src))
	   ipc_group_message(src, msg_buf); */

	return 0;
}

/*
 * Function responsible by all signal handlers treatment
 * any new signal must be added here
 */
void signal_treatment(int param)
{
	switch (param) {
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
		terminate = 1;
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
int	sound_init()
{
	int cl_flags;
	/* TODO handle errors here */

	sound_init_serial();

	ipc_fd = ClRegister("sx1_sound", &cl_flags);

	shdata = ShmMap(SHARED_SYSTEM);

	/* Subscribe to different groups */
	 /*TODO*/
	    /* ClSubscribeToGroup(MSG_GROUP_PHONE); */
	    return 0;

}

//-----------------------------------------------------------------
int main(int argc, char *argv[])
{
	int sock_nl, sock_loc;	// File handlers
	int res;		// temp vars

	struct sockaddr_nl sa_netlink;

	int on;
	fd_set active_fd_set, read_fd_set;
	struct timeval timeout;

#ifndef DEBUG
	res = daemon(0, 0);
#endif
	/* SIGNALS treatment */
	signal(SIGHUP, signal_treatment);
	signal(SIGPIPE, signal_treatment);
	signal(SIGKILL, signal_treatment);
	signal(SIGINT, signal_treatment);
	signal(SIGUSR1, signal_treatment);
	signal(SIGTERM, signal_treatment);

	INITSYSLOG(argv[0]);

	sound_init();

//------------------------
	/* -- Netlink socket -- */
	sock_nl = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (sock_nl == -1) {
		DBGMSG("netlink socket");
		return -1;
	}

	sa_netlink.nl_family = AF_NETLINK;
	sa_netlink.nl_groups = CN_IDX_SX1SND;
	sa_netlink.nl_pid = getpid();

	if (bind(sock_nl, (struct sockaddr *)&sa_netlink, sizeof(sa_netlink))) {
		DBGMSG("netlink bind");
		close(sock_nl);
		return -1;
	}
	on = sa_netlink.nl_groups;
	setsockopt(sock_nl, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &on,
		   sizeof(on));
//	fcntl(sock_nl, F_SETFL, O_NONBLOCK);

//------------------------

/* Initialize the timeout data structure. */
	// TODO not used now
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
/* Initialize the file descriptor set. */
	FD_ZERO(&active_fd_set);
	FD_SET(sock_nl, &active_fd_set);
	FD_SET(ipc_fd, &active_fd_set);

/*  ---  Socket processing loop ---*/
	while (!terminate) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			DBGMSG("select error = %d", errno);
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(sock_nl, &read_fd_set)) {
			/* Message from kernel */
			if (process_netlink(sock_nl)) {
				DBGMSG("error in process_netlink");
			}
		}
		/* Service IPC message */
		if (FD_ISSET(ipc_fd, &read_fd_set)) {
			if (ipc_handle(ipc_fd) < 0) {
				DBGMSG("error in ipc_handle");
			}
		}
	}
	closelog();
	close(fd_mux);
	close(sock_nl);
	close(sock_loc);
	return 0;
}
