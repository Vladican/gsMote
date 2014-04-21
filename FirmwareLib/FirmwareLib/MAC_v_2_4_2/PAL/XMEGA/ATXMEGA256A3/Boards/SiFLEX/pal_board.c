//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	  pal_board.c
//
//  Description:  PAL board specific functionality
//
//  Micro:
//  Compiler:
//
//  Written by:
//
//  Copyright (c)   2010 LS Research, LLC
//                  www.lsr.com
//
//  Version         rev 0.1 - prerelease
//
//////////////////////////////////////////////////////////////////////////////////


/* === Includes ============================================================ */

#include <stdbool.h>
#include <stdlib.h>
#include "pal.h"
#include "pal_boardtypes.h"
#include "pal_config.h"

/*
 * 'sys_time' is a entity of timer module which is given to other modules
 * through the interface file below. 'sys_time' is required to obtain the
 * frame timestamp
 */
#include "pal_timer.h"


/* === Macros ============================================================== */


/* === Types =============================================================== */

/**
 * Encoding of the board family in the board_family configuration
 * record member.
 */
enum boardfamilycode
{
    CFG_BFAMILY_RADIO_EXTENDER, /**< Radio Extender boards */
    CFG_BFAMILY_RCB             /**< Radio Controller boards */
} SHORTENUM;


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */


/* === Implementation ======================================================= */

/**
 * @brief Provides timestamp of the last received frame
 *
 * This function provides the timestamp (in microseconds)
 * of the last received frame.
 *
 * @param[out] Timestamp in microseconds
 */
void pal_trx_read_timestamp(uint32_t *timestamp)
{
    /*
     * Everytime a transceiver interrupt is triggred, input capture register of
     * the AVR is latched. The 'sys_time' is concatenated to the ICR to
     * generate the time stamp of the received frame.
     * 'sys_time'   'ICR'
     *  ---------|--------- => 32 bit timestamp
     *   16 bits   16 bits
     */
    *timestamp = (uint32_t)sys_time << (uint32_t)16;
    *timestamp |= (uint32_t)TIME_STAMP_REGISTER;
}	//end pal_trx_read_timestamp


/**
 * @brief Calibrates the internal RC oscillator
 *
 * This function calibrates the internal RC oscillator.
 *
 * @return True since the RC oscillator is always calibrated
 *         automatically at startup by hardware itself
 */
bool pal_calibrate_rc_osc(void)
{
    return (true);
}	//end pal_calibrate_rc_osc


/**
 * @brief Initialize the event system of the ATxmega
 */
void event_system_init(void)
{
    // Route system clock (16MHz) with prescaler 16 through event channel 0.
    EVSYS_CH0MUX = EVSYS_CHMUX_PRESCALER_16_gc;

    // Route port D pin 1 through event channel 1.
    EVSYS_CH1MUX = EVSYS_CHMUX_PORTD_PIN1_gc;
}	//end event_system_init


/**
 * @brief Initialize the interrupt system of the ATxmega
 */
void interrupt_system_init(void)
{
    /* Enable high priority interrupts */
    PMIC.CTRL |= PMIC_HILVLEN_bm;
}	//end interrupt_system_init


// LSR - 11/17/09
void clock_init(void)
{
	OSC.XOSCCTRL = (OSC_X32KLPM_bm + OSC_XOSCSEL1_bm);		// Enable external 32kHz crystal and low power mode for the 32kHz.

	// Enable 32MHz internal oscillator (and by thus disable the 2 MHz internal oscillator).
	OSC.CTRL = (OSC_XOSCEN_bm + OSC_RC32MEN_bm);

	// The ATxmega shall run from its internal 32MHz Oscillator.
	// Set the clock speed to 16MHz. Use internal 32MHz and DFLL.
    while (0 == (OSC.STATUS & OSC_RC32MRDY_bm))
    {
        // Hang until the internal 32MHz Oscillator is stable.
        ;
    }

    CCP = 0xD8;						// Enable change of protected IO register.
    CLK.PSCTRL = CLK_PSADIV0_bm;	// Use Prescaler A to divide 32MHz clock by 2 to 16MHz system clock.

    CCP = 0xD8;						// Enable change of protected IO register.
    CLK.CTRL = CLK_SCLKSEL0_bm;		// Set internal 32MHz Oscillator as system clock.

    // Enable DFLL for the external oscillator.
    OSC.DFLLCTRL = OSC_RC32MCREF_bm;
    DFLLRC32M.CTRL |= DFLL_ENABLE_bm;
}  //end clock_init


/**
 * @brief Initializes the GPIO pins
 *
 * This function is used to initialize the port pins used to connect
 * the microcontroller to transceiver.
 */
void gpio_init(void)
{
    /* The following pins are output pins.  */
    TRX_PORT1_DDR |= _BV(SEL);
    TRX_PORT1_DDR |= _BV(SCK);
    TRX_PORT1_DDR |= _BV(TRX_RST);
    TRX_PORT1_DDR |= _BV(MOSI);
    TRX_PORT1_DDR |= _BV(SLP_TR);

    /* The following pins are input pins.  */
    TRX_PORT1_DDR &= ~_BV(MISO);
}	//end gpio_init


/*
 * This function is called by timer_init() to perform the non-generic portion
 * of the initialization of the timer module.
 *
 * sys_clk = 16MHz -> Will be used as source for Event Channel 0 with Prescaler 16
 *
 * Timer usage
 * TCC0_CCA: Systime (software timer based on compare match)
 * TCC1_CCA: Input capture for time stamping (only if Antenna diversity is not used)
 */
void timer_init_non_generic(void)
{
    /* Select proper clock as timer clock source when radio is sleeping */
    TIMER_SRC_DURING_TRX_SLEEP();

    /*
     * Clear pending output compare matches of all.
     */
    TCC0_INTFLAGS = TC0_CCAIF_bm | TC0_CCBIF_bm | TC0_CCCIF_bm | TC0_CCDIF_bm;
    TCC1_INTFLAGS = TC1_CCAIF_bm | TC1_CCBIF_bm;

    /* Enable the timer overflow interrupt for TCC0 used for systime overflow. */
    TCC0_INTCTRLA =  TC_OVFINTLVL_HI_gc;

    /* Assign event channel 1 as input capture to TCC1_CCA */
    TCC1_CTRLB |= TC1_CCAEN_bm;
    TCC1_CTRLD = TC1_EVACT0_bm | TC1_EVSEL3_bm | TC1_EVSEL0_bm;
}	//end timer_init_non_generic
