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



 /* set display brightness 0..5 */
int mach_set_display_brightness(int n)
{
	shdata->HAL.DisplayBrightness = n;
	int file;
	uint8_t s_br;

	s_br = SOFIA_MAX_LIGHT_VAL * n / 5;

	if ((file = open("/dev/i2c-0",O_RDWR)) < 0) {
		return -1;
	}

	int addr = SOFIA_I2C_ADDR; /* The I2C address of Sofia */
	if (ioctl(file, I2C_SLAVE, addr) < 0) {
		return -1;
	}
	uint8_t buf[2];

	/* Using I2C Write, equivalent of i2c_smbus_write_word_data(file,register,0x6543) */
	buf[0] = SOFIA_BACKLIGHT_REG;
	buf[1] = s_br;
	if ( write(file, buf, 2) != 2 ) {
		return -1;
	}

	/* Using I2C Write, equivalent of i2c_smbus_write_word_data(file,register,0x6543) */
	buf[0] = SOFIA_KEYLIGHT_REG;
	buf[1] = s_br;
	if ( write(file, buf, 2) != 2 ) {
		return -1;
	}

	return 0;
}

