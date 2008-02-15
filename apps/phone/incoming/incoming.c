/***************************************************************************
 *   Copyright (C) 2008 by Denys Gatsenko   *
 *   orb1t@orb1t   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libgsmd/libgsmd.h>
#include <gsmd/usock.h>

#include <nano-X.h>

#define STDIN_BUF_SIZE	1024

static struct lgsm_handle *lgsmh;
int pending_responses = 0;

GR_WINDOW_ID dialer_window;
GR_GC_ID gc;
GR_EVENT event;

static int call_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	struct gsmd_call_status *gcs;
	struct gsmd_call_fwd_stat *gcfs;
	int *ret;

	switch (gmh->msg_subtype) {
	case GSMD_VOICECALL_GET_STAT:
		gcs = (struct  gsmd_call_status*) ((char *)gmh + sizeof(*gmh));
		
		if (gcs->idx > 0)
			printf("%d, %d, %d, %d, %d, %s, %d\n",
					gcs->idx, gcs->dir,
					gcs->stat, gcs->mode,
					gcs->mpty, gcs->number,
					gcs->type);
		else if (gcs->idx < 0)
			/* If index < 0, error happens */
			printf("+CME ERROR %d\n", (0-(gcs->idx)));
		else
			/* No existing call */
			printf("Doesn't exist\n");

		if (gcs->is_last)
			pending_responses --;
		break;
	case GSMD_VOICECALL_CTRL:
		ret = (int*)((char *)gmh + sizeof(*gmh));
		 (*ret)? printf("+CME ERROR %d\n", *ret) : printf("OK\n");
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_DIS:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_EN:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_STAT:
		gcfs = (struct  gsmd_call_fwd_stat*) ((char *)gmh + sizeof(*gmh));
		
		printf("+CCFC:%d, %d, %s\n",gcfs->status, gcfs->classx, gcfs->addr.number);

		if (gcfs->is_last)
			pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_REG:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_ERAS:
		pending_responses --;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int incall_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Incoming call type = %u\n", aux->u.call.type);

	GrMapWindow(dialer_window);
	GrSetFocus(dialer_window);
	GrText(dialer_window, gc, 10, 10, "Incoming call", 0, 0);


	return 0;
}

static int error_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	if(aux->u.cme_err.number)
		printf("cme error: %u\n", aux->u.cme_err.number);
	else if(aux->u.cms_err.number)
		printf("cms error: %u\n", aux->u.cms_err.number);
		
	return 0;
}

int event_init(struct lgsm_handle *lh)
{
	int rc;

	rc  = lgsm_evt_handler_register(lh, GSMD_EVT_IN_CALL, &incall_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_IN_ERROR, &error_handler);
	return rc;
}

int main(int argc, char *argv[]){

	if ( GrOpen() < 0){
		//error("Couldn't connect to Nano-X server!\n");
		fprintf ( stderr, "Couldn't connect to Nano-X server!\n");
		exit(-1);
	}

	lgsmh = lgsm_init(LGSMD_DEVICE_GSMD);
	if (!lgsmh) {
		fprintf(stderr, "Can't connect to gsmd\n");
		exit(1);
	}

	//pin_init(lgsmh, pin);
	event_init(lgsmh);

	int gsm_fd = lgsm_fd(lgsmh);
	lgsm_register_handler(lgsmh, GSMD_MSG_VOICECALL, call_msghandler);

	dialer_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 120, 176, 180, 0, MWNOCOLOR, 0);
	GrSelectEvents(dialer_window, GR_EVENT_MASK_BUTTON_DOWN );


	int rc;
	char buf[STDIN_BUF_SIZE+1];
	while ( 1 ){
		fd_set readset;
		FD_SET(0, &readset);
		FD_SET(gsm_fd, &readset);

		rc = select(gsm_fd+1, &readset, NULL, NULL, NULL);
		if (rc <= 0)
			break;
		if (FD_ISSET(gsm_fd, &readset)) {
			/* we've received something on the gsmd socket, pass it
			 * on to the library */
			rc = read(gsm_fd, buf, sizeof(buf));
			if (rc <= 0) {
				printf("ERROR reading from gsm_fd\n");
				break;
			}
			rc = lgsm_handle_packet(lgsmh, buf, rc);
		}
		
		GrGetNextEvent(&event);
		switch(event.type) {
		case GR_EVENT_TYPE_KEY_DOWN:
                    	printf ( "!got keyboard event, event.keystroke.ch = %i\n", event.keystroke.ch ); 
			int k;
			switch(event.keystroke.ch) {
			case  	MWKEY_F3:
				printf("Answer\n");
				lgsm_voice_in_accept(lgsmh);
                        	break;
                	case  	MWKEY_F4:
				printf("Hangup\n");
				lgsm_voice_hangup(lgsmh);
                		break;
                	default:
                		break;
			}
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
		case GR_EVENT_TYPE_SCREENSAVER:
		case GR_EVENT_TYPE_TIMER:
		case GR_EVENT_TYPE_NONE:
			break;
		default:
			fprintf(stderr, "Got unknown event type %d\n", event.type);
			break;
	}
	}

}
