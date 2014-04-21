//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	  pal_config.h
//
//  Description:  This header file contains configuration parameters for the SiFLEX02
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


/* Prevent double inclusion */
#ifndef PAL_CONFIG_H
#define PAL_CONFIG_H

// === Includes =============================================================

#include "pal_boardtypes.h"

// This header file is required since a function with
// return type retval_t is declared
#include "return_val.h"

// === Types ================================================================


// === Externals ============================================================


// === Macros ===============================================================

// IRQ macros for ATxmega256A3

// Number of used TRX IRQs in this implementation
#define NO_OF_TRX_IRQS                  (2)

// Mapping of TRX interrupts to ISR vectors
#define TRX_MAIN_ISR_VECTOR             (PORTD_INT0_vect)
#define TRX_TSTAMP_ISR_VECTOR           (PORTD_INT1_vect)

// Enables the transceiver interrupts
#define ENABLE_TRX_IRQ(trx_irq_num)     do {                    \
    if (trx_irq_num == TRX_MAIN_IRQ_HDLR_IDX)                   \
    {                                                           \
        /* Enable PORTD interrupt 0 with high priority */       \
        PORTD.INTCTRL |= PORT_INT0LVL_gm;                       \
    }                                                           \
    else if (trx_irq_num == TRX_TSTAMP_IRQ_HDLR_IDX)            \
    {                                                           \
        /* Enable PORTD interrupt 1 with high priority */       \
        PORTD.INTCTRL |= PORT_INT1LVL_gm;                       \
    }                                                           \
} while (0)

// Disables the transceiver interrupts
#define DISABLE_TRX_IRQ(trx_irq_num)    do {                    \
    if (trx_irq_num == TRX_MAIN_IRQ_HDLR_IDX)                   \
    {                                                           \
        /* Disable PORTD interrupt 0 with high priority */      \
        PORTD.INTCTRL &= ~PORT_INT0LVL_gm;                      \
    }                                                           \
    else if (trx_irq_num == TRX_TSTAMP_IRQ_HDLR_IDX)            \
    {                                                           \
        /* Disable PORTD interrupt 1 with high priority */      \
        PORTD.INTCTRL &= ~PORT_INT1LVL_gm;                      \
    }                                                           \
} while (0)

// Clears the transceiver interrupts
#define CLEAR_TRX_IRQ(trx_irq_num)     do {                     \
    if (trx_irq_num == TRX_MAIN_IRQ_HDLR_IDX)                   \
    {                                                           \
        PORTD.INTFLAGS = PORT_INT0IF_bm;                        \
    }                                                           \
    else if (trx_irq_num == TRX_TSTAMP_IRQ_HDLR_IDX)            \
    {                                                           \
        PORTD.INTFLAGS = PORT_INT1IF_bm;                        \
    }                                                           \
} while (0)

// Enables the global interrupt
#define ENABLE_GLOBAL_IRQ()             sei()

// Disables the global interrupt
#define DISABLE_GLOBAL_IRQ()            cli()

// This macro saves the global interrupt status
#define ENTER_CRITICAL_REGION()         {uint8_t sreg = SREG; cli()

// This macro restores the global interrupt status
#define LEAVE_CRITICAL_REGION()         SREG = sreg;}

// This macro saves the trx interrupt status and disables the trx interrupt.
#define ENTER_TRX_REGION()      { uint8_t irq_mask = PORTD.INTCTRL; PORTD.INTCTRL &= ~PORT_INT0LVL_gm

// This macro restores the transceiver interrupt status
#define LEAVE_TRX_REGION()      PORTD.INTCTRL = irq_mask; }

// GPIO macros for ATxmega256A3

// This board uses an SPI-attached transceiver.
#define PAL_USE_SPI_TRX                 (1)

// Actual Ports
// The data direction register for the transceiver
#define TRX_PORT1_DDR                   (PORTD.DIR)

// The transceiver port
#define TRX_PORT1                       (PORTD)

// RESET pin of transceiver
#define TRX_RST                         (0)

// Sleep Transceiver pin
#define SLP_TR                          (3)

// Slave select pin
#define SEL                             (4)

// SPI Bus Master Output/Slave Input pin
#define MOSI                            (5)

// SPI Bus Master Input/Slave Output pin
#define MISO                            (6)

// SPI serial clock pin
#define SCK                             (7)

// Value of an external PA gain
// If no external PA is available, value is 0.
#define EXTERN_PA_GAIN                  (0)

// Set TRX GPIO pins.
#define RST_HIGH()                      (TRX_PORT1.OUTSET = _BV(TRX_RST))
#define RST_LOW()                       (TRX_PORT1.OUTCLR = _BV(TRX_RST))
#define SLP_TR_HIGH()                   (TRX_PORT1.OUTSET = _BV(SLP_TR))
#define SLP_TR_LOW()                    (TRX_PORT1.OUTCLR = _BV(SLP_TR))

// Timer macros for ATxmega256A3

// These macros are placeholders for delay functions for high speed processors.
//
// The following delay are not reasonbly implemented via delay functions,
// but rather via a certain number of NOP operations.
// The actual number of NOPs for each delay is fully MCU and frequency
// dependent, so it needs to be updated for each board configuration.
//
// ATxmega256a3 @ 16MHz

// Wait for 500 ns.
#define PAL_WAIT_500_NS()               {nop(); nop(); nop(); nop(); \
                                         nop(); nop(); nop(); nop();}
// Wait for 1 us.
#define PAL_WAIT_1_US()                 {nop(); nop(); nop(); nop(); \
                                         nop(); nop(); nop(); nop(); \
                                         nop(); nop(); nop(); nop(); \
                                         nop(); nop(); nop(); nop();}

// The smallest timeout in microseconds
#define MIN_TIMEOUT                     (0x80)

// The largest timeout in microseconds
#define MAX_TIMEOUT                     (0x7FFFFFFF)

// Minimum time in microseconds, accepted as a delay request
#define MIN_DELAY_VAL                   (5)

// Settings to give clocking to timer when radio is awake
#define TIMER_SRC_DURING_TRX_AWAKE()

// Settings to give clocking to timer when radio is sleeping
//
// T1 & T0:
// clk source is event channel 0 triggered by system clock (16MHz) with prescaler 16
#define TIMER_SRC_DURING_TRX_SLEEP() {TCC0_CTRLA = TC0_CLKSEL3_bm; TCC1_CTRLA = TC1_CLKSEL3_bm;}

// Maximum numbers of software timers running at a time
#define MAX_NO_OF_TIMERS                (25)

// Hardware register that holds Rx timestamp
#define TIME_STAMP_REGISTER             (TCC1_CCA)


// TRX Access macros for ATxmega128A1
// Bit mask for slave select
#define SS_BIT_MASK                     (1 << SEL)

// Slave select made low
#define SS_LOW()                        (TRX_PORT1.OUTCLR = SS_BIT_MASK)

// Slave select made high
#define SS_HIGH()                       (TRX_PORT1.OUTSET = SS_BIT_MASK)

// Mask for SPIF bit in status register
#define SPIF_MASK                       (SPI_IF_bm)

// SPI Data Register
#define SPI_DATA_REG    (SPID.DATA)

// Wait for SPI interrupt flag
#define SPI_WAIT()                      do {                        \
    while ((SPID.STATUS & SPIF_MASK) == 0) { ; }                    \
} while (0)

// Dummy value written in SPDR to retrieve data form it
#define SPI_DUMMY_VALUE                 (0x00)

// TRX Initialization
#define TRX_INIT()                      do {                        \
    /* Enable the SPI and make the microcontroller as SPI master */ \
    SPID.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | (0 << SPI_MODE0_bp);\
    /* Set SEL pin to high */                                       \
    TRX_PORT1.OUTSET = _BV(SEL);                                    \
} while (0)

// Storage location for crystal trim value - within external EEPROM
#define EE_XTAL_TRIM_ADDR                  (21)

// Alert initialization
#define ALERT_INIT()                    do {    \
        PORTE.OUT    = 0;                       \
        PORTE.DIRSET = 0xFF;                    \
} while (0)

// Alert indication
#define ALERT_INDICATE()                (PORTE.OUTTGL = 0xFF)


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
} // extern "C"
#endif

#endif  //end PAL_CONFIG_H
