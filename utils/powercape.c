#include "powercape.h"

// misc constants
#define I2C_MAX_DEVICE_NAME 0x0C     // maximum length of i2c filename

// global struct to hold needed powercap data
static powercape pc;

void msleep( int msecs )
{
    usleep( msecs * 1000 );
}

int i2c_read( void *buf, int len )
{
    int rc = 0;

    if ( read( pc.handle, buf, len ) != len )
    {
        fprintf(stderr, "I2C read failed: %s\n", strerror( errno ) );
        rc = -1;
    }

    return rc;
}


int i2c_write( void *buf, int len )
{
    int rc = 0;
    
    if ( write( pc.handle, buf, len ) != len )
    {
        fprintf(stderr, "I2C write failed: %s\n", strerror( errno ) );
        rc = -1;
    }
    
    return rc;
}


int register_read( unsigned char reg, unsigned char *data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    if ( i2c_write( bite, 1 ) == 0 )
    {
        if ( i2c_read( bite, 1 ) == 0 )
        {
            *data = bite[ 0 ];
            rc = 0;
        }
    }
    
    return rc;
}


int register32_read( unsigned char reg, unsigned int *data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    if ( i2c_write( bite, 1 ) == 0 )
    {
        if ( i2c_read( data, 4 ) == 0 )
        {
            rc = 0;
        }
    }
    
    return rc;
}


int register_write( unsigned char reg, unsigned char data )
{
    int rc = -1;
    unsigned char bite[ 4 ];
    
    bite[ 0 ] = reg;
    bite[ 1 ] = data;

    if ( i2c_write( bite, 2 ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}


int register32_write( unsigned char reg, unsigned int data )
{
    int rc = -1;
    unsigned char bite[ 6 ];
    
    bite[ 0 ] = reg;
    bite[ 1 ] = data & 0xFF;
    bite[ 2 ] = ( data >> 8 ) & 0xFF;
    bite[ 3 ] = ( data >> 16 ) & 0xFF;
    bite[ 4 ] = ( data >> 24 ) & 0xFF;

    if ( i2c_write( bite, 5 ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}

int cape_initialize(int i2c_bus, int avr_address)
{
    int rc = 0;
    pc.i2c_bus = i2c_bus;
    pc.handle = 0;
    pc.status = CAPE_OK;
    char filename[I2C_MAX_DEVICE_NAME];
    snprintf(filename, I2C_MAX_DEVICE_NAME, "/dev/i2c-%d", i2c_bus);
    pc.handle = open(filename, O_RDWR);
    if (pc.handle == -1)
    {
        fprintf(stderr, "Failed to open %s: (%d) %s", filename, errno, strerror(errno));
        rc = -1;
        pc.status = CAPE_FAIL;
    }

    if (ioctl(pc.handle, I2C_SLAVE, avr_address) < 0)
    {
        fprintf(stderr, "IOCTL Error: %s\n", strerror(errno));
        rc = -1;
    }

    return rc;
}

int cape_close(void)
{
    int rc = 0;
    if (pc.handle != 0)
    {
        rc = close(pc.handle);
        if (rc == -1)
        {
            fprintf(stderr, "Error closing handler: (%d) %s", errno, strerror(errno));
        }
    }
    return rc;
}

int cape_enter_bootloader( void )
{
    unsigned char b;
    int rc = 2;
    
    if ( register_write( REG_CONTROL, CONTROL_BOOTLOAD ) == 0 )
    {
        if ( register_read( REG_CONTROL, &b ) == 0 )
        {
            fprintf( stderr, "Unable to switch to cape bootloader\n" );
            rc = 3;
        }
        else
        {
            rc = 0;
        }
    }
    
    return rc;
}


int cape_read_rtc( time_t *iptr )
{
    int rc = 1;
    unsigned int seconds;
    
    if ( register32_read( REG_SECONDS_0, &seconds ) == 0 )
    {
        //printf( "Cape RTC seconds %08X (%d)\n", seconds, seconds );
        printf( ctime( (time_t*)&seconds ) );
        
        if ( iptr != NULL )
        {
            *iptr = seconds;
        }
        rc = 0;
    }
    
    return rc;
}


int cape_write_rtc( void )
{
    int rc = 1;
    unsigned int seconds = time( NULL );
    
    //printf( "System seconds %08X (%d)\n", seconds, seconds );
    printf( ctime( (time_t*)&seconds ) );

    if ( register32_write( REG_SECONDS_0, seconds ) == 0 )
    {
        rc = 0;
    }
    
    return rc;
}


int cape_query_reason_power_on( void )
{
    int rc = 1;
    unsigned char reason;

    if ( register_read( REG_START_REASON, &reason) == 0 )
    {
        switch ( reason ) {
        case 1: printf("BUTTON\n"); break;
        case 2: printf("OPTO\n"); break;
        case 4: printf("PGOOD\n"); break;
        case 8: printf("TIMEOUT\n"); break;
        default: printf("CODE %d\n", reason); break;
        }
        rc = 0;
    }

    return rc;
}


int cape_show_cape_info( void )
{
    unsigned char c;
    unsigned char c1, c2, c3, c4;
    unsigned char revision, stepping, type;
    char capability = -1;

    if ( register_read(REG_EXTENDED, &c) == 0 && c == 0x69 )
    {
        if ( register_read(REG_CAPABILITY, &capability) != 0 )
	{
	    capability = -1;
	}
    }

    if ( register_read(REG_CONTROL, &c) == 0 )
    {
	if ( ! (c & CONTROL_CE) ) printf("Charger is not enabled!\n");
	if ( c & CONTROL_BOOTLOAD ) printf("Bootloader is enabled!\n");
	printf("LED 1 %s, LED 2 %s\n",
		c & CONTROL_LED0 ? "on" : "off",
		c & CONTROL_LED1 ? "on" : "off");
    }

    if ( register_read(REG_START_REASON, &c) == 0 )
    {
        printf("Powered on triggered by ");
	if ( c & START_BUTTON ) printf("button press ");
	if ( c & START_EXTERNAL ) printf("external event ");
	if ( c & START_PWRGOOD ) printf("power good ");
	if ( c & START_TIMEOUT ) printf("timer");
        printf("\n");
    }

    if ( capability >= CAPABILITY_WDT )
    {
        if ( register_read( REG_BOARD_TYPE, &type) == 0 && 
             register_read( REG_BOARD_REV, &revision ) == 0 && 
             register_read( REG_BOARD_STEP, &stepping ) == 0 )
        {
            if ( revision <= 32 || revision >= 127 ) revision = '?';
            if ( stepping <= 32 || stepping >= 127 ) stepping = '?';
            printf("%s PowerCape %c%c\n", 
		type == BOARD_TYPE_BONE ? "BeagleBone" : 
		    type == BOARD_TYPE_PI ? "Raspberry Pi" : "Unknown", 
		revision, 
		stepping);
        }

        if ( register_read(REG_WDT_RESET, &c1) == 0 && 
             register_read(REG_WDT_POWER, &c2) == 0 && 
             register_read(REG_WDT_STOP, &c3) == 0 && 
             register_read(REG_WDT_START, &c4) == 0 )
        {
            printf("Watchdog: power cycle @ %d, power down @ %d, start within @ %d, reset for %d\n", c2, c3, c4, c1);
        }
    }

    if ( capability >= CAPABILITY_RTC ) 
    {
	unsigned int seconds;
	if ( register32_read(REG_SECONDS_0, &seconds) == 0 )
	{
	    printf("RTC: %s", ctime((time_t*)&seconds));
	}
    }

    if ( register_read(REG_START_ENABLE, &c) == 0 )
    {
        printf("Allow power on by ");
	if ( c & START_BUTTON ) printf("button press; ");
	if ( c & START_EXTERNAL ) printf("external event; ");
	if ( c & START_PWRGOOD ) printf("power good signal; ");
	if ( c & START_TIMEOUT ) 
	{
            unsigned char hours, minutes, seconds;
            if ( register_read(REG_RESTART_HOURS , &hours) == 0 && 
                 register_read(REG_RESTART_MINUTES , &minutes) == 0 && 
                 register_read(REG_RESTART_SECONDS , &seconds) == 0 )
            {
                if ( seconds > 0 ) {
                    printf("%d seconds power off", hours * 3600 + minutes * 60 + seconds);
                }
                else if ( minutes > 0 )
                {
                    printf("%d minutes power off", hours * 60 + minutes);
                }
                else
                {
                    printf("%d hours power off", hours);
                }
            }

        }
	printf("\n");
    }

    if ( register_read(REG_STATUS, &c) == 0 )
    {
	if ( c & STATUS_BUTTON ) printf("Button PRESSED\n");
	if ( c & STATUS_OPTO ) printf("Opto ACTIVE\n");
	// if ( c & STATUS_POWER_GOOD ) printf("Power good\n");
    }

    if ( capability >= CAPABILITY_ADDR )
    {
        if ( register_read(REG_I2C_ADDRESS, &c) == 0 )
        {
            printf("AVR I2C address: 0x%02x\n", c);
        }
    }

    // if ( register_read(REG_MCUSR, &c1) == 0 && 
    //      register_read(REG_OSCCAL, &c2) == 0 )
    // {
    //     printf("AVR MCURS: 0x%02x, OSCCAL: 0x%02x\n", c1, c2);
    // }

    if ( capability >= CAPABILITY_CHARGE && (revision == 'A' && stepping >= '2' || revision > 'A') )
    {
        if ( register_read(REG_I2C_ICHARGE, &c1) == 0 && register_read(REG_I2C_TCHARGE, &c2) == 0 )
        {
	    printf("Charge current: %d mA\n", c1 * 1000 / 3);
	    printf("Charge timer: %d hours\n", c2);
        }
    }

    return 0;
}

int cape_charge_rate(unsigned char rate)
{
    //TODO: add in capability checks as done in show info
     int rc = 0;
     if ((rate == CHARGE_RATE_LOW) ||
         (rate == CHARGE_RATE_MED) ||
         (rate == CHARGE_RATE_HIGH))
     {
         rc = register_write(REG_I2C_ICHARGE, rate);
     }
     else
     {
        fprintf(stderr, "Rate %d is out of range\n", rate);
        rc = -1;
     }
     return rc;
}

int cape_charge_time(unsigned char time)
{
    int rc = 0;
    if ((time >= CHARGE_RATE_LOW) && (time <= CHARGE_TIME_MAX))
    {
        rc = register_read(REG_I2C_TCHARGE, time);
    }
    else
    {
        fprintf(stderr, "Charge time %d is out of range\n", time);
    }
    return rc;
}

int cape_power_down(unsigned char seconds)
{
    int rc = 0;
    if ((seconds >= POWER_DOWN_MIN_SEC) && (seconds <= POWER_DOWN_MAX_SEC))
    {
        rc = register_write(REG_WDT_STOP, seconds);
    }
    else
    {
        fprintf(stderr, "Power down seconds %d is out of range\n", seconds);
    }
    return rc;
}

int cape_power_on(int seconds)
{
    int rc = 0;
    if ((seconds >= POWER_ON_MIN_SEC) && (seconds <= POWER_ON_MAX_SEC))
    {
        // convert to hours, minutes, seconds
        unsigned char hour = (unsigned char) seconds / 3600;
        unsigned char min = (unsigned char) (seconds % 3600) / 60;
        unsigned char sec = (unsigned char) (seconds % 60);

        rc = register_write(REG_RESTART_HOURS, hour);
        if (rc != -1)
        {
            rc = register_write(REG_RESTART_MINUTES, min);
        }
        if (rc != -1)
        {
            rc = register_write(REG_RESTART_SECONDS, sec);
        }
    }
    else
    {
        fprintf(stderr, "Power on seconds %d is out of range\n", seconds);
    }
    return rc;
}

