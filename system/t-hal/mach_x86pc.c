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
