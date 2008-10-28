/* mach_sx1.c
 *
 */

#include "t-hal-mach.h"

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define SOFIA_MAX_LIGHT_VAL     0x30    // (was 0x2B)

#define SOFIA_I2C_ADDR          0x32
// Sofia reg 3 bits masks
#define SOFIA_POWER1_REG        0x03

#define SOFIA_USB_POWER         0x01
#define SOFIA_MMC_POWER         0x04
#define SOFIA_BLUETOOTH_POWER   0x08
#define SOFIA_MMILIGHT_POWER    0x20

#define SOFIA_POWER2_REG        0x04
#define SOFIA_BACKLIGHT_REG     0x06
#define SOFIA_KEYLIGHT_REG      0x07
#define SOFIA_DIMMING_REG       0x09


static int set_brightness(int file, int br)
{
	uint8_t buf[2];

	/* Using I2C Write, equivalent of i2c_smbus_write_word_data(file,register,0x6543) */
	buf[0] = SOFIA_BACKLIGHT_REG;
	buf[1] = br;
	if ( write(file, buf, 2) != 2 ) {
		return -1;
	}

	/* Using I2C Write, equivalent of i2c_smbus_write_word_data(file,register,0x6543) */
	buf[0] = SOFIA_KEYLIGHT_REG;
	buf[1] = br;
	if ( write(file, buf, 2) != 2 ) {
		return -1;
	}
	return 0;
}

 /* set display brightness 0..5 */
int mach_set_display_brightness(int n)
{
	int file;
	uint8_t s_br;

	if (n < 0 || n > 5)
		return -EINVAL;

	shdata->HAL.DisplayBrightness = n;
	s_br = SOFIA_MAX_LIGHT_VAL * n / 5;

	if ((file = open("/dev/i2c-0",O_RDWR)) < 0) {
		return -1;
	}

	int addr = SOFIA_I2C_ADDR; /* The I2C address of Sofia */
	if (ioctl(file, I2C_SLAVE, addr) < 0) {
		return -1;
	}

	set_brightness(file, s_br);

	return 0;
}

int mach_connect_signals()
{
	int err;

	err = tbus_connect_signal("sx1_dsy", "BatteryChargeLevel");
	err |= tbus_connect_signal("sx1_dsy", "BatteryCharging");
	err |= tbus_connect_signal("PhoneServer", "BatteryChargeLevel");

	return err;
}

/* returns 1-message was handled here, 0 - not handled */
int mach_handle_signal(struct tbus_message *msg)
{
	if (!strcmp(msg->service_dest, "PhoneServer")) {
		if (!strcmp(msg->object, "BatteryChargeLevel")) {
			int value;
			tbus_get_message_args(msg, "i", &value);
			tbus_emit_signal("BatteryCharge", "i", &value);
		}
	} else
	if (!strcmp(msg->service_dest, "sx1_dsy")) {
		if (!strcmp(msg->object, "BatteryChargeLevel")) {
			int value;
			tbus_get_message_args(msg, "i", &value);
			tbus_emit_signal("BatteryCharge", "i", &value);
		} else
		if (!strcmp(msg->object, "BatteryCharging")) {
			int state;
			tbus_get_message_args(msg, "i", &state);
			tbus_emit_signal("BatteryCharging", "i", &state);
			mach_set_display_brightness(1);
			/*TODO play a sound here on start and stop charging */
		}
	} else
		return 0;

	return 1;
}
