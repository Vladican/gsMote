/**
 * @file tal_rx.h
 *
 * @brief File contains macros and modules used while processing
 * a received frame.
 *
 * $Id: tal_rx.h 16323 2009-06-23 16:38:09Z sschneid $
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
#ifndef TAL_RX_H
#define TAL_RX_H

/* === INCLUDES ============================================================ */


/* === EXTERNALS =========================================================== */


/* === TYPES =============================================================== */

/*
 * Structure to store the received frame length, timestamp and RSSI
 */
typedef struct rx_frame_info_tag
{
    /** Received frame length */
    uint8_t  frame_length;
    /** Received frame energy level */
    uint8_t  ed_level;
    /** Timestamp of the received frame */
    uint32_t timestamp;
} rx_frame_info_t;

/* === MACROS ============================================================== */


/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

void handle_received_frame_irq(void);

void process_incoming_frame(buffer_t *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TAL_RX_H */
