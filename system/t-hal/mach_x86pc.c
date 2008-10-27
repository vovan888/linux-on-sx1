/* mach_x86pc.c
 *
 */

#include "t-hal-mach.h"

 /* set display brightness 0..5 */
int mach_set_display_brightness(int n)
{
	shdata->HAL.DisplayBrightness = n;

	return 0;
}

int mach_connect_signals()
{
}

int mach_handle_signal(struct tbus_message *msg)
{
}