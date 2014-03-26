#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "board.h"
#include "registers.h"
#include "twi_slave.h"

uint8_t mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr( void )
{
    mcusr = MCUSR;
    MCUSR = 0;
    wdt_enable( WDTO_2S );
}


enum state_type {
    STATE_INIT,
    STATE_CLEAR_MASK,
    STATE_OFF,
    STATE_OFF_PGOOD,
    STATE_POWER_ON,
    STATE_POWER_OFF,
    STATE_ON,
};

volatile uint8_t power_state = STATE_INIT;


void power_down( void )
{
    if ( power_state == STATE_ON )
    {
        power_state = STATE_POWER_OFF;
    }
}


void power_event( uint8_t reason )
{
    if ( ( power_state == STATE_OFF ) || ( power_state == STATE_OFF_PGOOD ) )
    {
        if ( registers_get( REG_START_ENABLE ) & reason )
        {
            power_state = STATE_POWER_ON;
            registers_set_mask( REG_START_REASON, reason );
        }
    }
}


void state_machine( void )
{
    switch( power_state )
    {
        default:
        case STATE_INIT:
        {
            if ( board_3v3() )
            {
                power_state = STATE_POWER_ON;
            }
            else
            {
                power_state = STATE_POWER_OFF;
            }
            break;
        }
        
        case STATE_CLEAR_MASK:
        {
            uint8_t reg = registers_get( REG_START_ENABLE ); 
            
            board_enable_interrupt( reg );
            board_begin_countdown();
            
            if ( board_pgood() )
            {
                power_state = STATE_OFF_PGOOD;
            }
            else
            {
                power_state = STATE_OFF;
            }
            
            registers_clear_mask( REG_START_REASON, 0xFF );

            break;
        }
        
        case STATE_OFF:
        {
            if ( board_pgood() )
            {
                power_state = STATE_OFF_PGOOD;
                power_event( START_PWRGOOD );
            }
            break;
        }
        
        case STATE_OFF_PGOOD:
        {
            if ( !board_pgood() )
            {
                power_state = STATE_OFF;
            }
            break;
        }
        
        case STATE_POWER_ON:
        {
            board_power_on();
            if ( board_3v3() != 0 )
            {
                power_state = STATE_ON;
                twi_slave_init();
                board_disable_interrupt( START_ALL );
            }
            break;
        }
        
        case STATE_ON:
        {
            // Check for countdown value
            if ( board_3v3() == 0 )
            {
                power_state = STATE_POWER_OFF;
            }
            break;
        }
        
        case STATE_POWER_OFF:
        {
            twi_slave_stop();
            board_power_off();
            power_state = STATE_CLEAR_MASK;
            break;
        }
    }
}


int main( void )
{
    // Platform setup
    board_init();
    registers_init();
    registers_set( REG_MCUSR, mcusr );
    
    set_sleep_mode( SLEEP_MODE_PWR_SAVE );
    sei();

    // Main loop
    while ( 1 )
    {
        state_machine();
        if ( power_state != STATE_ON )
        {
            sleep_enable();
            sleep_cpu();
            sleep_disable();
        }
        wdt_reset();
    }
}

