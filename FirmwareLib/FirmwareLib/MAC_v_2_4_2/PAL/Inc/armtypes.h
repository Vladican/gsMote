/**
 * @file armtypes.h
 *
 * @brief Compatibility definitions for compilers (IAR, GCC)
 *
 * This file contains ARM type definitions that enable Atmel's 802.15.4
 * stack implementation to build using multiple compilers.
 *
 * $Id: armtypes.h 20082 2010-01-29 11:50:27Z sschneid $
 *
 */
/**
 *  @author
 *      Atmel Corporation: http://www.atmel.com
 *      Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel’s Limited License Agreement --> EULA.txt
 */

/* Prevent double inclusion */
#ifndef ARMTYPES_H
#define ARMTYPES_H

/* === Includes ============================================================= */

/*
 * The if defined(__ICCARM__) section below applies to the IAR compiler.
 */
#if defined(__ICCARM__)

#include <intrinsics.h>

/*
 * The elif defined(__GNUC__) section below applies to the GNUC compiler.
 */
#elif defined(__GNUC__)

/*
 * The else section below applies to unsupported compiler. This is where support
 * for compilers other than IAR and GNUC would be placed.
 */
#else
#error Unsupported compiler
#endif /* compiler selection */

/* === Externals ============================================================ */


/* === Macros =============================================================== */

#ifndef _BV
/**
 * Bit value -- compute the bitmask for a bit position
 */
#define _BV(x) (1 << (x))
#endif

/*
 * This block contains just the documentation for all
 * compiler-dependent macros.
 */
#if defined(DOXYGEN)

/**
 * Globally enable interrupts.
 */
#define sei()

/**
 * Globally disable interrupts.
 */
#define cli()

/**
 * Read contents of EEPROM cell \c addr into variable \c var.
 */
#define EEGET(var, addr)

/**
 * Null operation: just waste one CPU cycle.
 */
#define nop()

/**
 * Attribute to apply to struct tags in order to force it into an 8-bit
 * alignment.
 */
#define ALIGN8BIT

/**
 * Attribute to apply to an enum declaration to force it into the smallest
 * type size required to represent all values.
 */
#define SHORTENUM

#endif /* defined(DOXYGEN) */

#if defined(__ICCARM__)

/* Intrinsic functions provided by IAR to enable and disable IRQ interrupts. */
#define sei()               __enable_irq()
#define cli()               __disable_irq()

/* Intrinsic functions provided by IAR to get the current program status word */
#define GET_CPSR(irq)       (irq = __get_CPSR())

/* Intrinsic functions provided by IAR to set the current program status word */
#define SET_CPSR(irq)       __set_CPSR(irq)

#define nop()               __no_operation()

#define EEGET(var, addr)    nop()

/* program memory space abstraction */
#define FLASH_EXTERN(x) extern const x
#define FLASH_DECLARE(x)  const x
#define FUNC_PTR void *
#define FLASH_STRING(x) ((const char *)(x))
#define FLASH_STRING_T char const
#define PGM_READ_BYTE(x) *(x)
#define PGM_READ_WORD(x) *(x)
#define PGM_READ_BLOCK(dst, src, len) memcpy((dst), (src), (len))
#define PGM_STRLEN(x) strlen(x)
#define PGM_STRCPY(dst, src) strcpy((dst), (src))

#define PUTS(s) { static const char c[] = s; printf(c); }
#define PRINTF(fmt, ...) { static const char c[] = fmt; printf(c, __VA_ARGS__); }

#define ALIGN8BIT /**/

/*
 * Extended keyword provided by IAR to set the alignment of members of
 * structure and union to 1.
 */
#define SHORTENUM           __packed

#endif /* defined(__ICCARM__) */

#if defined(__GNUC__)

#define asm                 __asm__

#define volatile            __volatile__

/*
 * No intrinsic function is provided by GCC to enable and disable interrupt,
 * hence they are implemented in assembly language
 */
#define sei()   do                          \
{                                           \
    asm volatile ("MRS R0, CPSR");          \
    asm volatile ("BIC R0, R0, #0x80");     \
    asm volatile ("MSR CPSR_c, R0");        \
}while (0);

#define cli()   do                          \
{                                           \
    asm volatile ("MRS R0, CPSR");          \
    asm volatile ("ORR R0, R0, #0x80");     \
    asm volatile ("MSR CPSR_c, R0");        \
}while (0);

/* Gets the current program status word */
#define GET_CPSR(sreg)  asm volatile ("MRS %0, CPSR" : "=r" (sreg) :);

/* Sets the current program status word */
#define SET_CPSR(sreg)  asm volatile ("MSR CPSR_c, %0" : : "r" (sreg));

#define nop()               do { asm volatile ("nop"); } while (0)

#define EEGET(var, addr)    nop()

#define ALIGN8BIT /**/

/*
 * Provided by GCC to set the alignment of members of structure and union to 1.
 */
#define SHORTENUM           __attribute__((packed))

#endif /* defined(__GNUC__) */

#define ADDR_COPY_DST_SRC_16(dst, src)  ((dst) = (src))
#define ADDR_COPY_DST_SRC_64(dst, src)  ((dst) = (src))

/* === Types ================================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef cplusplus
} /* extern "C" */
#endif

#endif /* ARMTYPES_H */
/* EOF */

