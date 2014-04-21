/**
 * @file indirect_data_structures.h
 *
 * @brief This module contains structures used internally for indirect data
 *
 * $Id: indirect_data_structures.h 19330 2009-11-24 12:47:54Z sschneid $
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
#ifndef INDIRECT_DATA_STRUCTURES_H
#define INDIRECT_DATA_STRUCTURES_H

/* === Includes ============================================================= */


/* === Macros =============================================================== */


/* === Types ================================================================ */

/**
 * @brief This is the Indirect data message structure.
 */
typedef struct indirect_data_tag
{
    frame_info_t *data;
    uint8_t msduHandle;
    bool in_transit;
} indirect_data_t;



typedef struct indirect_data_pers_timer_tag
{
    uint8_t *buf_ptr;
    uint16_t persistence_time;
    bool active;
} indirect_data_pers_timer_t;

/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* INDIRECT_DATA_STRUCTURES_H */
/* EOF */
