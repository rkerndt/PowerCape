/* Rickie Kerndt <rkerndt@cs.uoregon.edu>
 * powercape.h
 */

#ifndef __POWER_CAPE_H__
#define __POWER_CAPE_H__
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <endian.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include "../avr/registers.h"


#define AVR_ADDRESS         0x21
#define INA_ADDRESS         0x40

// avr defined battery charge rates
#define CHARGE_RATE_ZERO    0x00     // disables battery charging
#define CHARGE_RATE_LOW     0x01     // maximum 1/3 amp
#define CHARGE_RATE_MED     0x02     // maximum 2/3 amp
#define CHARGE_RATE_HIGH    0x03     // maximum 1 amp

// avr battery max charge time
#define CHARGE_TIME_MIN     0x03     // charge stops after 3 hours
#define CHARGE_TIME_MAX     0x0A     // charge stops after 10 hours

// status codes
#define CAPE_INIT           0x00
#define CAPE_OK             0x01
#define CAPE_FAIL           0x02
#define CAPE_ERROR          0x03


// structure to hold data fields needed by powercape routines
typedef struct _powercape {
    int i2c_bus;
    int handler;
    int status;
} powercape;


int cape_initialize(int i2c_bus);

int cape_close(void);

int cape_enter_bootloader(void);

int cape_read_rtc(time_t *iptr);

int cape_write_rtc(void);

int cape_query_reason_power_on(void);

int cape_show_cape_info(void);

int set_charge_rate(int rate);

int set_charge_time(int time);

#endif