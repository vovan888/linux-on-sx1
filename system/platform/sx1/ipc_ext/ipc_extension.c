/* ipc_extension.c
*
* SX1 extension server
*
* by Vladimir Ananiev (Vovan888 at gmail com )
*
*  handles messages to modem
*  channel 4 (AMUX::104)
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
//#include <asm/types.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/select.h>

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

#include <ipc/colosseum.h>
#include <ipc/shareddata.h>
#include <arch/sx1/ipc_dsy.h>
#include <debug.h>

/* timeout in seconds */
#define IPC_TIMEOUT 3
#define uint	unsigned int
#define MODEMDEVICE "/dev/mux3"
#define MAXMSG	64

static int wait_for_daemon_status = 0;
static volatile int terminate = 0;
static pid_t the_pid;
int _priority;

/* file descriptor for /dev/mux3, should be opened blocking */
static int fd_mux;

/* IPC interface */
struct SharedSystem *shdata;	/* shared memory segment */
static int ipc_fd;		/* IPC file descriptor */

/* RTC interface */
static const char default_rtc[] = "/dev/rtc";
static int rtc_fd;		/* RTC file descriptor */

static struct {
	int pingok;
	int rtccheck;
	int reason;
	int sw_reason;
	int selftest;
	unsigned char hiddenreset;
	unsigned char devmenuenabled;
	unsigned char sim_domestic_lang;
} startup;			// saves current state
/*-----------------------------------------------------------------*/
static int extension_init_serial(void)
{
	struct termios options;

	fd_mux = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd_mux < 0) {
		perror("Error: open " MODEMDEVICE " failed!");
		exit(-1);
	}
	/* set port settings */
	cfmakeraw(&options);
	options.c_iflag = IGNBRK;
	/* Enable the receiver and set local mode and 8N1 */
	options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL);
	options.c_cflag |= CRTSCTS;	/* enable hardware flow control (CNEW_RTCCTS) */
	options.c_cflag |= B38400;	/* Set speed */
	options.c_lflag = 0;	/* set raw input */
	options.c_oflag = 0;	/* set raw output */
	/* Set the new options for the port */
	tcsetattr(fd_mux, TCSANOW, &options);
	return 0;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* waits for reading length bytes from port. with timeout */
static int Read(char *frame, int length, int readtimeout)
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
		retval = select(FD_SETSIZE, &read_set, NULL, NULL, &timeoutz);
		if (retval < 0) {
			syslog(LOG_DEBUG, "read select error = %d", errno);
			return -1;
			/* exit (EXIT_FAILURE); */
		}
		if (retval == 0) {
			syslog(LOG_DEBUG, "read timeout!");
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

/*-----------------------------------------------------------------*/
// write frame to serial port fd_mux
static int Write(char *frame, int length)
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

/*-----------------------------------------------------------------*/
// RDsyReqHandler::Request(TDesC8 const &, TDes8 &)
// it is here - (maybe) sub_5069F4FC
// check err here
static int Request(char *request_frame, char *reply_frame)
{
	unsigned short length;
	int len_write, len_read;
	char err;

	length = 6 + *(unsigned short *)(request_frame + 4);
	len_write = Write(request_frame, length);
	SYSLOG("written to serial : %d", len_write);
	if (len_write != length)
		return -1;

	len_read = Read(reply_frame, 6, IPC_TIMEOUT);	// read header
	SYSLOG("read from serial port : %d", len_read);
	if (len_read != 6)
		return -1;	// Error

	length = *(unsigned short *)(reply_frame + 4);
	if (length) {
		/* read the rest of data (if there) */
		len_read = Read(reply_frame + 6, length, IPC_TIMEOUT);
		SYSLOG("=read from serial port : %d", len_read);
		if (len_read != length)
			return -1;	// Error
	}

	if (reply_frame[0] != IPC_DEST_OMAP)
		return -1;	// Error - packet is not to us

	err = reply_frame[3];
	if ((err != 0x30) && (err != 0xFF))
		return -1;	// Error

	return 0;
}

/*-----------------------------------------------------------------*/
void PutHeader(char *buf, char Group, unsigned char cmd, unsigned short length)
{
	buf[0] = IPC_DEST_MODEM;	/* Message destination */;
	buf[1] = Group;			/* Message group */
	buf[2] = cmd;			/* command */
	buf[3] = 0xFF;			/* error */
	*(unsigned short *)(buf + 4) = length - 6;
}

/*-----------------------------------------------------------------*/
// CDsyFactory::PingIpcL(void)
int PingIpcL(void)
{
	char req[6];
	char reply[6];
	PutHeader(req, IPC_GROUP_POWERUP, PWRUP_PING_REQ, 6);
	return Request(req, reply);
}

/*-----------------------------------------------------------------*/
// for CCMon and DevMenu
int UnpackString(char *buf, char *string)	// string will be in Unicode ?
{
	char err = buf[3];
	unsigned short length;
	if ((err == 0x30) || (err == 0xFF)) {
		length = (unsigned short)*(buf + 4);
		memcpy(string, buf + 6, length);
		return length;
	}
	return -1;
}

/*-----------------------------------------------------------------*/
// 1000 CDsyExtension::CCMonGetStringLength(RDosCCMonitorExt::TStringParam &)
// returns string length, or -1 if error
// we should replace all 0x0D with 0x0A, maybe not
int CCMonGetString(int strnum, char *string)
{
	char buf[6];
	char reply[0x1006];

	PutHeader(buf, IPC_GROUP_CCMON, strnum, 6);
	if (Request(buf, reply))
		return -1;
	return UnpackString(reply, string);
}

/*-----------------------------------------------------------------*/
// 2000 CDsyExtension::DevMenuGetDevMenuEnabled(int &)
// returns -1 error, 0 - disabled, 1 - enabled
int DevMenuGetDevMenuEnabled(void)
{
	char buf[6];
	char reply[7];

	PutHeader(buf, IPC_GROUP_DEVMENU, DEVMENU_Enabled, 6);
	if (Request(buf, reply))
		return -1;
	return (reply[6] == 0) ? 0 : 1;
}

/*-----------------------------------------------------------------*/
// 2001 CDsyExtension::DevMenuGetStringLength(RDosDevMenuExt::TStringParam &)
// strnum used: 1..10,12,20..27
int DevMenuGetString(int strnum, char *string)
{
	char buf[6];
	char reply[0x1006];

	if (strnum > 27)
		return -1;
	PutHeader(buf, IPC_GROUP_CCMON, (char)strnum, 6);
	if (Request(buf, reply))
		return -1;
	return UnpackString(reply, string);
}

/*-----------------------------------------------------------------*/
// 2003 CDsyExtension::DevMenuGetResetExits(int &)
int DevMenuGetResetExits(void)
{
	char buf[6];
	char reply[7];

	PutHeader(buf, IPC_GROUP_CCMON, DEVMENU_ResetExits, 6);
	if (Request(buf, reply))
		return -1;
	return (int)reply[6];
}

/*-----------------------------------------------------------------*/
// 3000 CDsyExtension::SwitchPowerOff(void)
int SwitchPowerOff(void)
{
	char buf[6];
	char reply[6];

	PutHeader(buf, IPC_GROUP_POWEROFF, PWROFF_SWITCHOFF, 6);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 3001 CDsyExtension::PartialModeReq(unsigned char)
int PartialModeReq(unsigned char mode)
{
	char buf[7];
	char reply[6];

	PutHeader(buf, IPC_GROUP_POWEROFF, PWROFF_OMAPPARTIALMODE, 7);
	buf[6] = mode;
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 4000 CDsyExtension::BTAudioGetBdDataFromEElite(unsigned char *)
int BTAudioGetBdDataFromEElite(unsigned char *btdata)
{
	char buf[6];
	char reply[66];

	PutHeader(buf, IPC_GROUP_BTBD, BTBD_DataFromEElite, 6);
	if (Request(buf, reply))
		return -1;
	memcpy(btdata, reply + 6, 60);
	return 0;
}

/*-----------------------------------------------------------------*/
// 6000 CDsyExtension::SimGetActivePendingLockType(RDosSimExt::TGetActivePendingLockTypeParam &)
int SimGetActivePendingLockType(char *c1, char *c2)
{
	char buf[6], reply[8];

	do {
		PutHeader(buf, IPC_GROUP_SIM, SIM_ActivePendingLockType, 6);
		if (Request(buf, reply))
			return -1;
		*c1 = reply[6];
		*c2 = reply[7];
		if (*c1 != 6)
			break;
		usleep(10000);
	} while (1);
//      if(c1 == 5) {
	// CDosEventManager::SimState(TDosSimState) (7)
//              return 0;
//      }
//      if(c2 == 0) {
//              // CDosEventManager::SimLockStatus(TSASimLockStatus) (1)
//              return 0;
//      }
	return 2;
}

/*-----------------------------------------------------------------*/
// 6001 CDsyExtension::SimGetDomesticLanguage(unsigned char &)
// returns -1 = error, or language
int SimGetDomesticLanguage(char *cc)
{
	char buf[6], reply[7];

	PutHeader(buf, IPC_GROUP_SIM, SIM_GetDomesticLanguage, 6);
	if (Request(buf, reply))
		return -1;
	*cc = reply[6];
//      if( (c1==18)||(c1==37)||(c1==49)||(c1==60)||(c1==63)||(c1==81)||(c1==84) )
//              * c1 = 0xFF;
	return 1;
}

/*-----------------------------------------------------------------*/
// 6002 CDsyExtension::SimSetDomesticLanguage(unsigned char)
int SimSetDomesticLanguage(unsigned char lang)
{
	char buf[7], reply[6];

	PutHeader(buf, IPC_GROUP_SIM, SIM_SetDomesticLanguage, 7);
	buf[6] = lang;
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 6004 CDsyExtension::SimGetSimlockStatus(unsigned short &)
int SimGetSimlockStatus(unsigned short *c1)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_SIM, SIM_GetSimlockStatus, 6);
	if (Request(buf, reply))
		return -1;
	*c1 = *(unsigned short *)(reply + 6);
	return 1;
}

/*-----------------------------------------------------------------*/
int RtcCheck(void)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_RTC_CHECK, 6);
	if (Request(buf, reply))
		return -1;
	return (int)reply[6];
}

/*-----------------------------------------------------------------*/
int RtcTransfer(struct tm *time)
{
	char buf[6], reply[22];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_RTC_TRANSFER, 6);
	if (Request(buf, reply))
		return -1;
	time->tm_sec = *(unsigned short *)(reply + 6);
//      SYSLOG("sec= %d", time -> tm_sec);
	time->tm_min = *(unsigned short *)(reply + 8);
//      SYSLOG("min= %d", time -> tm_min);
	time->tm_hour = *(unsigned short *)(reply + 10);
//      syslog(LOG_DEBUG, "hour= %d", time -> tm_hour);
	time->tm_mday = *(unsigned short *)(reply + 12);
	time->tm_mon = *(unsigned short *)(reply + 14) - 1;
//      SYSLOG("mon= %d", time -> tm_mon);
	time->tm_year = *(unsigned short *)(reply + 16) - 1900;
//      SYSLOG("year= %d", time -> tm_year);
	time->tm_wday = *(unsigned short *)(reply + 18);
//      buf[20] = 0; // ???
	return 0;
}

/*-----------------------------------------------------------------*/
int StartupReason(void)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_STARTUP_REASON, 6);
	if (Request(buf, reply))
		return -1;
	return *(unsigned short *)(reply + 6);
}

/*-----------------------------------------------------------------*/
int SWStartupReason(void)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_GETSWSTARTUPREASON, 6);
	if (Request(buf, reply))
		return -1;
	return *(unsigned short *)(reply + 6);
}

/*-----------------------------------------------------------------*/
int SelfTest(unsigned short test)
{
	char buf[8], reply[6];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_SELFTEST, 8);
	*(unsigned short *)(buf + 6) = test;
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
int PowerUpHiddenReset(void)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_HIDDENRESET, 6);
	if (Request(buf, reply))
		return -1;
	return (int)reply[6];
}

/*-----------------------------------------------------------------*/
int IndicationObserverOk(void)
{
	char buf[6], reply[8];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_INDICATIONOBSERVEROK, 6);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 7000 CDsyExtension::TimeSetModemTime(TTime)
int TimeSetModemTime(struct tm *time)
{
	char buf[21], reply[6];

	PutHeader(buf, IPC_GROUP_POWERUP, PWRUP_RTCSETUP, 21);
	*(unsigned short *)(buf + 6) = time->tm_sec;
	*(unsigned short *)(buf + 8) = time->tm_min;
	*(unsigned short *)(buf + 10) = time->tm_hour;
	*(unsigned short *)(buf + 12) = time->tm_mday;
	*(unsigned short *)(buf + 14) = time->tm_mon + 1;
	*(unsigned short *)(buf + 16) = time->tm_year + 1900;
	*(unsigned short *)(buf + 18) = time->tm_wday;
	buf[20] = 0;		// ???
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 8000 CDsyExtension::LightsGetDisplayConstrast(signed char &, signed char &)
int LightsGetDisplayConstrast(char *contr1, char *contr2)
{
	char buf[6], reply[8];
	PutHeader(buf, IPC_GROUP_RAGBAG, RAG_GetDisplayConstrast, 6);
	if (Request(buf, reply))
		return -1;
	*contr1 = reply[6];
	*contr2 = reply[7];
	return 0;
}

/*-----------------------------------------------------------------*/
// 8001 CDsyExtension::LightsSetDisplayConstrast(signed char)
int LightsSetDisplayConstrast(char contrast)
{
	char buf[7], reply[6];
	PutHeader(buf, IPC_GROUP_RAGBAG, RAG_SetDisplayConstrast, 7);
	buf[6] = contrast;
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 9000 CDsyExtension::TransitionCoProMode(void)
int TransitionCoProMode(void)
{
	char buf[6], reply[6];
	PutHeader(buf, IPC_GROUP_POWEROFF, PWROFF_TRANSITIONCOPROMODE, 6);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// 9001 CDsyExtension::TransitionToNormalMode(void)
int TransitionToNormalMode(void)
{
	char buf[6], reply[6];
	PutHeader(buf, IPC_GROUP_POWEROFF, PWROFF_TRANSITIONTONORMAL, 6);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// A000 CDsyExtension::PanicTransfer(RPanicSupportExt::TPanicListType)
int PanicTransfer(char *panic)
{
	char buf[22], reply[6];
	PutHeader(buf, IPC_GROUP_RAGBAG, RAG_PanicTransfer, 22);
	memcpy(buf + 6, panic, 16);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/

// Send general message with no args and results
int ipc_message(char group, char cmd)
{
	char buf[6], reply[6];
	PutHeader(buf, group, cmd, 6);
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// Send general message with 1 char arg and no results
int ipc_message_char(char group, char cmd, char arg)
{
	char buf[7], reply[6];
	PutHeader(buf, group, cmd, 7);
	buf[6] = arg;
	return Request(buf, reply);
}

/*-----------------------------------------------------------------*/
// Handles RAGBAG messages
// returns answer message size
int handle_ragbag(char *msg, char *answer)
{
	char cmd;
	char *data;
	int ret = -1;
	cmd = *(unsigned char *)(msg + 1);
	data = msg + 2;

	switch (cmd) {
	case RAG_UsbSessionEnd:
		ret = ipc_message(IPC_GROUP_RAGBAG, RAG_UsbSessionEnd);
		break;
	case RAG_RestoreFactorySettings:
//                      ret =
		break;
	case RAG_SetDosAlarm:
		break;
	case RAG_SetState:
		ret = ipc_message_char(IPC_GROUP_RAGBAG, RAG_SetState, data[0]);
		break;
	case RAG_ModeTransition:
		ret =
		    ipc_message_char(IPC_GROUP_RAGBAG, RAG_ModeTransition,
				     data[0]);
		break;
	case RAG_AudioLinkOpenResReq:
		ret = ipc_message(IPC_GROUP_RAGBAG, RAG_AudioLinkOpenResReq);
		break;
	case RAG_AudioLinkCloseResReq:
		ret = ipc_message(IPC_GROUP_RAGBAG, RAG_AudioLinkCloseResReq);
		break;
	case RAG_EgoldWakeUp:
		ret = ipc_message(IPC_GROUP_RAGBAG, RAG_EgoldWakeUp);
		break;
	}
	return ret;
}

/*-----------------------------------------------------------------*/
// Handles SIM messages
// returns answer message size
int handle_sim(char *msg, char *answer)
{
	char cmd;
	char *data;
	int ret = -1;
	cmd = *(unsigned char *)(msg + 1);
	data = msg + 2;

	switch (cmd) {
	case SIM_GetLanguage:
//                      ret = sim_getlang();
		break;
	case SIM_ActivePendingLockType:
		ret = SimGetActivePendingLockType(answer, answer + 1);
		break;
	case SIM_GetDomesticLanguage:
		ret = SimGetDomesticLanguage(answer);
		break;
	case SIM_SetDomesticLanguage:
		ret = SimSetDomesticLanguage(data[0]);
		break;
	case SIM_GetSimlockStatus:
		ret = SimGetSimlockStatus((unsigned short *)(answer));
		break;
	}
	return ret;
}

/*-----------------------------------------------------------------*/
// Decodes message
// Returns answer message length, or -1 if error
int decode_message(char *msg, char *answer)
{
	char group, cmd;
	char *data;
	struct tm *modem_time;
	time_t loc_time;
	int ret = 0;
	group = *(unsigned char *)(msg + 0);
	cmd = *(unsigned char *)(msg + 1);
	data = msg + 2;

	SYSLOG(" got message: %02X %02X", group, cmd);

	switch (group) {
	case IPC_GROUP_CCMON:
		ret = CCMonGetString(cmd, answer);
		break;
	case IPC_GROUP_DEVMENU:
		if (cmd == DEVMENU_Enabled) {
			ret = DevMenuGetDevMenuEnabled();
			*answer = (char)ret;
			ret = 1;
		} else if (cmd == DEVMENU_ResetExits) {
			ret = DevMenuGetResetExits();
			*answer = (char)ret;
			ret = 1;
		} else
			ret = DevMenuGetString(cmd, answer);
		break;
	case IPC_GROUP_RAGBAG:
		ret = handle_ragbag(msg, answer);
		break;
	case IPC_GROUP_SIM:
		ret = handle_sim(msg, answer);
		break;
	case IPC_GROUP_POWERUP:
		if (cmd == PWRUP_RTCSETUP) {
			/* save system time to egold */
			loc_time = time(NULL);
			modem_time = localtime(&loc_time);
			ret = TimeSetModemTime(modem_time);
			syslog(LOG_DEBUG, " modem time set");
			break;
		}
		if (cmd == PWRUP_SYNCREQ) {
		}
		ret = ipc_message(group, cmd);
		break;
	case IPC_GROUP_POWEROFF:
		ret = ipc_message(group, cmd);
		break;
	}
	return ret;
}

/*-----------------------------------------------------------------*/
/* processes messages from user programs
 * message is (ushort group)(ushort cmd)[char data...]
 * response is the same
 */
int process_client(const int fd)
{
	char buffer_in[MAXMSG];
	char buffer_out[0x1006];
	int nbytes;
	int length;		/* message length */
	int answer_length;

	/* read message length */
	if ((nbytes = read(fd, &length, sizeof(length))) == 0) {
		SYSLOG(" socket closed");
		return 0;	/* connection closed */
	}
	if (length > MAXMSG) {
		SYSLOG("message too big");
		return -1;
	}
	/* read the message body */
	nbytes = read(fd, buffer_in, length);
	if (nbytes <= 0) {
		/* Read error. */
		syslog(LOG_DEBUG, "message read error");
		return -1;
	} else {
		/* Data read. */
		if (nbytes >= 2)
			answer_length = decode_message(buffer_in, buffer_out);
		else {
			syslog(LOG_DEBUG, " message size is < 2");
			return -1;
		}
	}
	if (answer_length < 0)
		syslog(LOG_DEBUG, " error decoding message= %d", answer_length);
	else {
		/* send response */
		answer_length += 2;	// add header
		/* send size of message */
		nbytes = write(fd, &answer_length, sizeof(answer_length));
		/* send header back */
		nbytes = write(fd, buffer_in, 2);
		/* send the rest of message */
		if (answer_length > 2)
			nbytes = write(fd, buffer_out, answer_length - 2);
	}
	return 0;
}

/*-----------------------------------------------------------------*/
static int extension_init(void)
{
	/* TODO handle errors here */

	extension_init_serial();

	ipc_fd = ipc_start("sx1_dsy");

	shdata = ShmMap(SHARED_SYSTEM);

	/* Subscribe to different groups */
	 /*TODO*/
	    /* ClSubscribeToGroup(MSG_GROUP_PHONE); */
}

/*-----------------------------------------------------------------*/
/*
 * Read time from Egold RTC and set local time and system RTC
 */
static int extension_set_rtc(void)
{
	int res = 0, rtc_ready = 1;
	struct tm modem_time;
	time_t loc_time;

	/* open RTC device */
	rtc_fd = open(default_rtc, O_RDONLY);

	if (rtc_fd == -1) {
		perror(default_rtc);
		rtc_ready = 0;
	}


	/* read RTC time from Egold and set local time */
	startup.rtccheck = RtcCheck();	/* check RTC status */
	if (!startup.rtccheck) {
		res = RtcTransfer(&modem_time);
		if (!res) {
			loc_time = mktime(&modem_time);
			if (loc_time == -1)
				perror("mktime:");
			else
			/* set system time to localtime from modem */
			if (stime(&loc_time) < 0)
				perror("stime:");
			syslog(LOG_DEBUG, "local time set = %s",
			       asctime(&modem_time));
		}
	}
	
	/* set the RTC time */
	if (rtc_ready) {
		res = ioctl(rtc_fd, RTC_SET_TIME, modem_time);
		if (res == -1) {
			perror("RTC_SET_TIME ioctl");
		}
		close(rtc_fd);
	}

}

/*-----------------------------------------------------------------*/
static int extension_powerup(void)
{
	char cc;

	/* Startup Init IPC */
	shdata->powerup.egold_ping_ok = PingIpcL();	/* ping connection with EGold */

	shdata->powerup.reason = StartupReason();	/* Get startup reason */

	if (SimGetDomesticLanguage(&cc) >= 0) {
		shdata->sim.domesticlang = cc;
		SimSetDomesticLanguage(cc);
		syslog(LOG_DEBUG, "SimSetDomesticLanguage = %d", cc);
	}

	startup.hiddenreset = PowerUpHiddenReset();
	syslog(LOG_DEBUG, "PowerUpHiddenReset = %d", startup.hiddenreset);

	shdata->powerup.swreason = SWStartupReason();	//PowerUpGetSwStartupReasonReq;
	syslog(LOG_DEBUG, "SWStartupReason = %d", startup.sw_reason);

	/* PowerUpIndicationObserverOkReq, enable indication observer */
	IndicationObserverOk();

	shdata->powerup.selftest = SelfTest(0x0B);	/* 0x0B = unknown constant */
	syslog(LOG_DEBUG, "SelfTest = %d", startup.selftest);

	/*TODO RagbagSetDosAlarmReq(00000);*/

	TransitionToNormalMode();	// PowerOffTransitionToNormalReq
	syslog(LOG_DEBUG, "TransitionToNormalMode");

	/*TODO RagbagSetStateReq(3)*/
	/* 5069A8EC ; CDsyMtc::PowerOnL(void) */
	res = ipc_message_char(IPC_GROUP_RAGBAG, RAG_SetState, 3);

	return 0;
}
/*-----------------------------------------------------------------*/
/* Handle IPC message
 * This message only tells indicator that its value is changed
 * Actual value is stored in sharedmem
*/
int ipc_handle(int fd)
{
	int ack = 0, size = 64;
	unsigned short src = 0;
	unsigned char msg_buf[64];

	DBGMSG("ipc_ext: ipc_handle\n");

	if ((ack = ClGetMessage(&msg_buf, &size, &src)) < 0)
		return ack;

	if (ack == CL_CLIENT_BROADCAST) {
		/* handle broadcast message */
	}

	/*      if (IS_GROUP_MSG(src))
	   ipc_group_message(src, msg_buf); */
}
/*-----------------------------------------------------------------*/
/* shows how to use this program
 */
void usage(char *_name)
{
	fprintf(stderr, "\nUsage: %s [options] \n", _name);
	fprintf(stderr, "options:\n");
	fprintf(stderr, " -d : Debug mode, don't fork\n");
	fprintf(stderr, " -w : Wait for deamon startup success/failure\n");
	fprintf(stderr, " -h : Show this help message\n");
}

/* Wait for child process to kill the parent.
 */
void parent_signal_treatment(int param)
{
	fprintf(stderr, "ipc_extension started\n");
	exit(0);
}

/*-----------------------------------------------------------------*/
/*
 * Daemonize process, this process  create the daemon
 */
int daemonize(void)
{
#ifndef DEBUG
	signal(SIGHUP, parent_signal_treatment);
	if ((the_pid = fork()) < 0) {
		wait_for_daemon_status = 0;
		return (-1);
	} else if (the_pid != 0) {
		if (wait_for_daemon_status) {
			wait(NULL);
			fprintf(stderr,
				"ipc_extension startup failed. See syslog for details.\n");
			exit(1);
		}
		exit(0);	//parent goes bye-bye
	}
	//child continues
	setsid();		//become session leader
	//signal(SIGHUP, SIG_IGN);
	if (wait_for_daemon_status == 0 && (the_pid = fork()) != 0)
		exit(0);
	chdir("/tmp");		//change working directory
	umask(0);		// clear our file mode creation mask

// Close out the standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
#endif
	//daemonize process stop here
	return 0;
}

/*-----------------------------------------------------------------*/
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

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	int res, i, opt;
	fd_set active_fd_set, read_fd_set;
	struct timeval timeout;
	char *programName = argv[0];
	pid_t parent_pid;

	while ((opt = getopt(argc, argv, "h?")) > 0) {
		switch (opt) {
		case '?':
		case 'h':
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

	/* SIGNALS treatment */
	signal(SIGHUP, signal_treatment);
	signal(SIGPIPE, signal_treatment);
	signal(SIGKILL, signal_treatment);
	signal(SIGINT, signal_treatment);
	signal(SIGUSR1, signal_treatment);
	signal(SIGTERM, signal_treatment);

	programName = argv[0];
#ifdef DEBUG
	openlog(programName, LOG_NDELAY | LOG_PID | LOG_PERROR, LOG_LOCAL0);
	_priority = LOG_DEBUG;
#else
	openlog(programName, LOG_NDELAY | LOG_PID, LOG_LOCAL0);
	_priority = LOG_INFO;
#endif

#ifdef DEBUG
	syslog(LOG_INFO,
	       "You can quit the ipc_ext daemon with SIGKILL or SIGTERM");
#else
	if (wait_for_daemon_status) {
		kill(parent_pid, SIGHUP);
	}
#endif

/*-----------------------------------------------------------------*/
	extension_init();

	extension_set_rtc();

	extension_powerup();
/*-----------------------------------------------------------------*/
	/* Initialize the timeout data structure. */
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	/* Initialize the file descriptor set. */
	FD_ZERO(&active_fd_set);
	FD_SET(ipc_fd, &active_fd_set);

	/*  ---  Socket processing loop --- */
	while (!terminate) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			syslog(LOG_DEBUG, "select error = %d", errno);
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(ipc_fd, &read_fd_set)) {
			handle_ipc(ipc_fd);
		}
	}

	closelog();
	close(fd_mux);
	close(sock_loc);
	return 0;
}
