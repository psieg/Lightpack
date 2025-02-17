/*
 * Lightpack.c
 *
 *  Created on: 11.01.2011
 *      Author: Mike Shatohin (brunql)
 *     Project: Lightpack
 *
 *  Lightpack is a content-appropriate ambient lighting system for any computer
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Lightpack.h"
#include "LedDriver.h"
#include "LedManager.h"
#include "LightpackUSB.h"
#include<avr/sleep.h>

volatile uint8_t g_Flags = 0;

Images_t g_Images = { };

Settings_t g_Settings =
{
        .isSmoothEnabled = true,

        // Number of intermediate colors between old and new
        .smoothSlowdown = 100,

        .brightness = 50,

        // Maximum number of different colors for each channel (Red, Green and Blue)
        .maxPwmValue = 128,

        // Timer OCR value
        .timerOutputCompareRegValue = 100,

        // The enabled state of the blue USB LED on the back of the device
        .isUsbLedEnabled = true,
};

/*
 *  Interrupts of the timer that generates PWM
 */
ISR( TIMER1_COMPA_vect )
{
    LedManager_UpdateColors();

    // Clear timer interrupt flag
    TIFR1 = _BV(OCF1A);
}

// Watchdog interrupt
ISR ( WDT_vect )
{
    _BlinkUsbLed(3, 40);
    for(;;);
}

static inline void Timer_Init(void)
{
    TCCR1A = 0x00;
    TCCR1C = 0x00;
    TCCR1B = 0x00;
//    TCCR0A = 0x00;
//    TCCR0B = 0x00;

    // Setup default value
    OCR1A = g_Settings.timerOutputCompareRegValue;

    TIMSK1 = _BV(OCIE1A); // Main timer
//    TIMSK0 = _BV(TOIE0); // Usb led timer

    // Start timer
    TCCR1B = _BV(CS10); // div1
//    TCCR0B = _BV(CS02); // div by 8

    TCNT1 = 0x0000;
//    TCNT0 = 0x0000;
}

static inline void SetupHardware(void)
{
    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    DDRB  = 0b11111000;
    PORTB = 0b00000000;

    DDRC  = 0b11111100;
    PORTC = 0b00000000;

    DDRD  = 0b11111111;
    PORTD = 0b00000000;

    OUTPUT(USBLED);
    CLR(USBLED);

//    set_sleep_mode(SLEEP_MODE_IDLE);
}

static inline void enableWatchdog(void)
{
    wdt_reset();
    // Watchdog configuration: interrupt after 500ms and reset after 1000ms;
    wdt_enable(WDTO_500MS);
    // Enable watchdog interrupt
    WDTCSR |= _BV(WDIE);
}

static inline void _ProcessFlags(void)
{
    /* if (_FlagProcess(Flag_HaveNewColors)) */

    if (USB_DeviceState!=DEVICE_STATE_Configured || _FlagProcess(Flag_LedsOffAll))
        LedManager_FillImages(0, 0, 0);

    if (_FlagProcess(Flag_TimerOptionsChanged))
    {
        // Pause timer
        TIMSK1 &= (uint8_t)~_BV(OCIE1A);

        OCR1A = g_Settings.timerOutputCompareRegValue;

        // Restart timer
        TCNT1 = 0x0000;
        TIMSK1 = _BV(OCIE1A);
    }
}

/*
 *  Main program entry point
 */
int main(void)
{
    enableWatchdog();
    SetupHardware();
    _WaveSwitchOnUsbLed(100, 100);
    // Led driver ports initialization
    LedDriver_Init();

    // Initialize timer for update LedDriver-s
    Timer_Init();

    // Initialize USB
    USB_Init();

    sei();

    for (;;)
    {
        wdt_reset();
        ProcessUsbTasks();
        _ProcessFlags();
//	sleep_mode();
    }

    // Avoid annoying warning
    return 0;
}

