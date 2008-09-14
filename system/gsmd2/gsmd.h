#ifndef __GSMD__
#define __GSMD__

#include <gsmd/gsmd.h>

int pin_name_to_type(char *pin_name);
int gsmd_initsettings2(struct gsmd *gsmd);
int gsmd_initsettings_after_pin(struct gsmd *gsmd);

#endif
