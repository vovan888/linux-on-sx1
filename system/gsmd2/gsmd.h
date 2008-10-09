#ifndef __GSMD__
#define __GSMD__

#include <gsmd/usock.h>
#include <gsmd/gsmd.h>
#include <common/linux_list.h>

/* if defined - use second mux channel for "slow" commands */
#define GSMD_SLOW_MUX_DEVICE	1

struct gsmd_user;
extern struct gsmd *g_slow;

extern int usock_init(struct gsmd *g);
extern struct gsmd_ucmd *usock_build_event(u_int8_t type, u_int8_t subtype, u_int16_t len);
extern int gsmd_opname_init(struct gsmd *g);
extern int gsmd_opname_add(struct gsmd *g, const char *numeric_bcd_string,
			   const char *alnum_long);
int pin_name_to_type(char *pin_name);
int gsmd_initsettings2(struct gsmd *gsmd);
int gsmd_initsettings_after_pin(struct gsmd *gsmd);

/* machine plugin */
extern struct gsmd_machine_plugin gsmd_machine_plugin;
/* vendor plugin */
extern struct gsmd_vendor_plugin gsmd_vendor_plugin;

#endif
