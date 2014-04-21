/**
 * @file sio_handler.h
 *
 * @brief This file contains the prototypes for UART related functions.
 *
 * $Id: sio_handler.h 16867 2009-07-31 12:11:57Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* Prevent double inclusion */
#ifndef SIO_HANDLER_H
#define SIO_HANDLER_H

/* === Includes ============================================================= */

#include <stdio.h>

/* === Macros =============================================================== */

#ifdef UART0
#define SIO_CHANNEL     SIO_0
#endif
#ifdef UART1
#define SIO_CHANNEL     SIO_1
#endif
#ifdef USB0
#define SIO_CHANNEL     SIO_2
#endif

/* Function aliases allowing IAR and GCC functions use the same way */
#if ((defined __ICCAVR__) || (defined __ICCARM__))
#define sio_putchar(data)       _sio_putchar(data)
#define sio_getchar()           _sio_getchar()
#define sio_getchar_nowait()    _sio_getchar_nowait()
#else
#define sio_putchar(data)       _sio_putchar(data, NULL)
#define sio_getchar()           _sio_getchar(NULL)
#define sio_getchar_nowait()    _sio_getchar_nowait(NULL)
#endif

/* === Types ================================================================ */


/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif


#if ((defined __ICCAVR__) || (defined __ICCARM__))
int _sio_putchar(char data);
int _sio_getchar(void);
int _sio_getchar_nowait(void);
#else
int _sio_putchar(char data, FILE *dummy);
int _sio_getchar(FILE *dummy);
int _sio_getchar_nowait(FILE *dummy);
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SIO_HANDLER_H */
/* EOF */
