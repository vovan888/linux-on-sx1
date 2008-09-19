#ifndef __GSMD__
#define __GSMD__

#include <gsmd/usock.h>
#include <gsmd/gsmd.h>
#include <common/linux_list.h>

struct gsmd_user;

struct gsmd_ucmd {
	struct llist_head list;
	struct gsmd_msg_hdr hdr;
	char buf[];
} __attribute__ ((packed));

extern struct gsmd_ucmd *ucmd_alloc(int extra_size);
extern int usock_init(struct gsmd *g);
extern void usock_cmd_enqueue(struct gsmd_ucmd *ucmd, struct gsmd_user *gu);
extern struct gsmd_ucmd *usock_build_event(u_int8_t type, u_int8_t subtype, u_int16_t len);
extern int usock_evt_send(struct gsmd *gsmd, struct gsmd_ucmd *ucmd, u_int32_t evt);
extern int gsmd_ucmd_submit(struct gsmd_user *gu, u_int8_t msg_type,
			    u_int8_t msg_subtype, u_int16_t id, int len, const void *data);
extern int gsmd_opname_init(struct gsmd *g);
extern int gsmd_opname_add(struct gsmd *g, const char *numeric_bcd_string,
			   const char *alnum_long);
int pin_name_to_type(char *pin_name);
int gsmd_initsettings2(struct gsmd *gsmd);
int gsmd_initsettings_after_pin(struct gsmd *gsmd);

#endif
