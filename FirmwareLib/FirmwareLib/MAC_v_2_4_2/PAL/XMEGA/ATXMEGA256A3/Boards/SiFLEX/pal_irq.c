//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	  pal_irq.c
//
//  Description:  PAL IRQ functionality
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

#include <stdint.h>
#include "pal.h"
#include "pal_boardtypes.h"

/* === Types ============================================================== */

/**
 * This is a typedef of the function which is called from the transceiver ISR
 */
typedef void (*irq_handler_t)(void);

/* === Globals ============================================================= */

/*
 * Function pointers to store the callback function of
 * the transceiver interrupt
 */
static irq_handler_t irq_handler[NO_OF_TRX_IRQS];

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/**
 * @brief Initializes the transceiver interrupts
 *
 * This function sets the microcontroller specific registers
 * responsible for handling the transceiver interrupts
 *
 * @param trx_irq_num Transceiver interrupt line to be initialized
 * @param trx_irq_cb Callback function for the given transceiver
 * interrupt
 */
void pal_trx_irq_init(trx_irq_hdlr_idx_t trx_irq_num, void *trx_irq_cb)
{
    /*
     * Set the handler function.
     * The handler is set before enabling the interrupt to prepare for spurious
     * interrupts, that can pop up the moment they are enabled
     */
    irq_handler[trx_irq_num] = (irq_handler_t)trx_irq_cb;

    if (trx_irq_num == TRX_MAIN_IRQ_HDLR_IDX)
    {
        // Rising edge on IRQ pin used to trigger IRQ
        PORTD.PIN2CTRL = PORT_ISC0_bm;

        // Set pin 2 as source for port interrupt 0
        PORTD.INT0MASK = PIN2_bm;

        // Clear pending interrupts
        PORTD.INTFLAGS = PORT_INT0IF_bm;
    }
    else if (trx_irq_num == TRX_TSTAMP_IRQ_HDLR_IDX)
    {
        // Rising edge on DIG2 pin used to trigger IRQ
        PORTD.PIN1CTRL = PORT_ISC0_bm;

        // Set pin 1 as source for port interrupt 1
        PORTD.INT1MASK = PIN1_bm;

        // Clear pending interrupts
        PORTD.INTFLAGS = PORT_INT1IF_bm;
    }
}	//end pal_trx_irq_init


/**
 * @brief ISR for transceiver's main interrupt
 */
ISR(TRX_MAIN_ISR_VECTOR)
{
    irq_handler[TRX_MAIN_IRQ_HDLR_IDX]();
}


/**
 * @brief ISR for transceiver's RX TIME STAMP interrupt
 */
ISR(TRX_TSTAMP_ISR_VECTOR)
{
    // Clear capture interrupt.
    TCD1_INTFLAGS |= TC1_CCAIF_bm;
    irq_handler[TRX_TSTAMP_IRQ_HDLR_IDX]();
}
