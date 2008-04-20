/********************************************************************
* Description: Group management routines
* Author: Vladimir Ananiev (vovan888 at gmail com)
* Created at: Fri Jan 25 00:31:38 MSK 2008
*
* Copyright (c) 2008 Vladimir Ananiev  All rights reserved.
*
********************************************************************/
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <malloc.h>

#include <ipc/colosseum.h>
#include <common/linux_list.h>

#include "server.h"

struct subscription {
	struct llist_head list;

	unsigned short	group_id;
	cl_app_struct	*client;
};

static LLIST_HEAD(subscribers);

/* add subscription to list */
int subscr_add(unsigned short group_id, cl_app_struct * client)
{
	struct subscription *sub = malloc(sizeof(struct subscription));

	if (!sub)
		return -1;
	/* FIXME check if the subscription already exist */
	DPRINT("subscr_add: group %x, client %s[%d]\n",group_id, client->cl_name, client->cl_id);
	/* fill the struct */
	sub->group_id = group_id;
	sub->client = client;

	/* add data to the list */
	llist_add_tail(&sub->list, &subscribers);

	return 0;
}

/* delete subscription from list */
int subscr_del(unsigned short group_id, cl_app_struct * client)
{
	struct subscription *cur, *cur2;

	DPRINT("subscr_del: group %x, client %d\n",group_id, client->cl_id);
	llist_for_each_entry_safe(cur, cur2, &subscribers, list) {
		if ( (cur->group_id == group_id) && (cur->client == client) ) {
			/* delete from the list of subscriptions */
			llist_del(&cur->list);
			free(&cur->list);
		}
	}

	return 0;
}

/* Send message pkt to the group_id */
int subscr_send(unsigned short group_id, cl_app_struct * client, cl_pkt_buff *pkt)
{
	struct subscription *cur;
	cl_app_struct *dest;
	int stored = 0;

	if (llist_empty(&subscribers))
		return 0;

	DPRINT("subscr_send: to %x\n",group_id);
	
	cl_pkt_message *msg = pkt->data;
	/* set source ipc ID to the group ID */
	msg->src = group_id;
	
	llist_for_each_entry(cur, &subscribers, list) {
		if (cur->group_id == group_id) {
			/* target group found */
			dest = cur->client;
			if (dest != client) {
//			if (dest != client && ISSET_FLAG(dest->cl_flags, CL_F_ACTIVE)) {
				DPRINT("subscr_send: message from id=%x to %s[%x]\n",
				       client->cl_id, dest->cl_name, dest->cl_id);
				if (deliver_message(dest, pkt) == 1) {
					stored++;
					PKT_BUFF_ADD_OWNER(pkt);
				}
			}
		}
		
	}
	return stored;
}
