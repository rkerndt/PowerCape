/* Rickie Kerndt <rkerndt@cs.uoregon.edu>
 * power.c
 */


#include <getopt.h>
#include "powercape.h"

typedef enum {
    OP_NONE,
    OP_BOOT,
    OP_QUERY,
    OP_READ_RTC,
    OP_SET_SYSTIME,
    OP_WRITE_RTC,
    OP_INFO,
    OP_CHARGE,
    OP_CHARGE_TIME,
    OP_POWER_DOWN,
    OP_POWER_ON,
} op_type;

static op_type operation = OP_NONE;
static int operation_arg = 0;

void show_usage( char *progname )
{
    fprintf( stderr, "Usage: %s [OPTION] \n", progname );
    fprintf( stderr, "   Options:\n" );
    fprintf( stderr, "      -h --help           Show usage.\n" );
    fprintf( stderr, "      -i --info           Show PowerCape info.\n" );
    fprintf( stderr, "      -b --boot           Enter bootloader.\n" );
    fprintf( stderr, "      -q --query          Query reason for power-on.\n" );
    fprintf( stderr, "                          Output can be TIMEOUT, PGOOD, BUTTON, or OPTO.\n" );
    fprintf( stderr, "      -r --read           Read and display cape RTC value.\n" );
    fprintf( stderr, "      -s --set            Set system time from cape RTC.\n" );
    fprintf( stderr, "      -w --write          Write cape RTC from system time.\n" );
    fprintf( stderr, "      -cn --charge n      Set charge rate where n= 1, 2, or 3\n");
    fprintf( stderr, "      -tn --charge-time n Set charge time where n = 3-10 hours\n");
    fprintf( stderr, "      -pn --power-down n  Power down after n seconds where n=0-255\n");
    fprintf( stderr, "      -Pn --power-on n    Power on after n seconds where n=0-22047555 (~255 days)\n");
    exit( 1 );
}

void parse( int argc, char *argv[] )
{
    while( 1 )
    {
        static const struct option lopts[] =
        {
            { "help",        0, 0, 'h' },
            { "boot",        0, 0, 'b' },
            { "info",        0, 0, 'i' },
            { "query",       0, 0, 'q' },
            { "read",        0, 0, 'r' },
            { "set",         0, 0, 's' },
            { "write",       0, 0, 'w' },
            { "charge",      1, 0, 'c' },
            { "charge-time", 1, 0, 't' },
            { "power-down",  1, 0, 'p' },
            { "power-on",    1, 0, 'P' },
            { NULL,          0, 0, 0 },
        };
        int c;

        c = getopt_long( argc, argv, "ihbqrswc:t:p::P:", lopts, NULL );

        if( c == -1 )
            break;

        switch( c )
        {
            case 'b':
            {
                operation = OP_BOOT;
                break;
            }

            case 'i':
            {
                operation = OP_INFO;
                break;
            }

            case 'q':
            {
                operation = OP_QUERY;
                break;
            }

            case 'r':
            {
                operation = OP_READ_RTC;
                break;
            }

            case 's':
            {
                operation = OP_SET_SYSTIME;
                break;
            }

            case 'w':
            {
                operation = OP_WRITE_RTC;
                break;
            }

            case 'h':
            {
                operation = OP_NONE;
                show_usage( argv[ 0 ] );
                break;
            }
            case 'c':
            {
                operation = OP_CHARGE;
                operation_arg = atoi(optarg);
                if ((operation_arg < CHARGE_RATE_LOW) || (operation_arg > CHARGE_RATE_HIGH))
                {
                    operation = OP_NONE;
                    show_usage( argv[ 0 ] );
                }
                break;
            }
            case 't':
            {
                operation = OP_CHARGE_TIME;
                operation_arg = atoi(optarg);
                if ((operation_arg < CHARGE_TIME_MIN) || (operation_arg > CHARGE_TIME_MAX))
                {
                    operation = OP_NONE;
                    show_usage( argv[ 0] );
                }
                break;
            }
            case 'p':
            {
                operation = OP_POWER_DOWN;
                if (optarg != NULL)
                {
                    operation_arg = atoi(optarg);
                    if ((operation_arg < POWER_DOWN_MIN_SEC) || (operation_arg > POWER_DOWN_MAX_SEC))
                    {
                        operation = OP_NONE;
                        show_usage(argv[0]);
                    }
                }
                else
                {
                    operation_arg = POWER_DOWN_DEFAULT_SEC;
                }
                break;
            }
            case 'P':
            {
                operation = OP_POWER_ON;
                operation_arg = atoi(optarg);
                if ((operation_arg < POWER_ON_MIN_SEC) || (operation_arg > POWER_DOWN_MAX_SEC))
                {
                    operation = OP_NONE;
                    show_usage(argv[0]);
                }
                break;
            }
        }
    }
}


int main( int argc, char *argv[] )
{
    int rc = 0;
    char filename[ 20 ];

    if ( argc == 1 )
    {
        show_usage( argv[ 0 ] );
    }

    parse( argc, argv );

    if (cape_initialize(CAPE_I2C_BUS, AVR_ADDRESS) < 0)
    {
        exit(1);
    }

    switch ( operation )
    {
        case OP_INFO:
        {
            rc = cape_show_cape_info();
            break;
        }

        case OP_QUERY:
        {
            rc = cape_query_reason_power_on();
            break;
        }

        case OP_BOOT:
        {
            rc = cape_enter_bootloader();
            break;
        }

        case OP_READ_RTC:
        {
            rc = cape_read_rtc( NULL );
            break;
        }

        case OP_SET_SYSTIME:
        {
            struct timeval t;

            rc = cape_read_rtc( &t.tv_sec );
            if ( rc == 0 )
            {
                t.tv_usec = 0;
                rc = settimeofday( &t, NULL);
                if ( rc != 0 )
                {
                    fprintf( stderr, "Error: %s\n", strerror( errno ) );
                }
            }
            break;
        }

        case OP_WRITE_RTC:
        {
            rc = cape_write_rtc();
            break;
        }

        case OP_CHARGE:
        {
            rc = cape_charge_rate(operation_arg);
            break;
        }
        case OP_CHARGE_TIME:
        {
            rc = cape_charge_time(operation_arg);
            break;
        }
        case OP_POWER_DOWN:
        {
            rc = cape_power_down(operation_arg);
            break;
        }
        case OP_POWER_ON:
        {
            rc = cape_power_on(operation_arg);
            break;
        }
        default:
        case OP_NONE:
        {
            break;
        }
    }

    cape_close();
    return rc;
}
