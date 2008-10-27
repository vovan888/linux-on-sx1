/* t-hal-mach.h
 * Definitions for the low-level interfaces
 *
 */

#include <ipc/tbus.h>
#include <ipc/shareddata.h>

void handle_signal(struct tbus_message *msg);
void handle_method(struct tbus_message *msg);

extern struct SharedSystem *shdata;	/* shared memory segment */

/* set display brightness 0..100 */
int mach_set_display_brightness(int n);
int mach_connect_signals();
int mach_handle_signal(struct tbus_message *msg);
